## Context

Matter controllers (`DeviceCommissioner`, `InteractionModelEngine`) are today
tightly bound to the Matter UDP/TCP transport stack. There is no hook point for
routing commands through a message broker or any non-Matter transport. This
blocks deployments where the controller lives in a cloud service, behind a NAT,
or must interoperate with MQTT-first IoT infrastructure.

This design introduces a thin proxy layer in `src/controller/proxy/` that sits
between callers of the controller API and the wire, delegating message I/O to a
registered `TransportProtocol` adapter. The first adapter is MQTT; future
adapters (WebSocket, AMQP, …) plug in without changes to the core.

The SDK uses C++17 and Pigweed's `pw::async` for async operations. Heap
allocation must be minimised; adapters own their own buffers.

## Goals / Non-Goals

**Goals:**

- Expose a **clean, protocol-agnostic controller interface** (`ControllerProxy`)
  that hides all transport, session, and routing details from the caller.
- Define a stable `TransportProtocol` C++ interface that any bidirectional cloud
  communication adapter can implement.
- Support **one or more active adapters** behind a single proxy, with the
  registry selecting the appropriate adapter per call.
- Implement `MqttTransportAdapter` as the first concrete adapter, acting as an
  IPC bridge to a separately managed external MQTT connection.
- Gate the MQTT adapter behind `chip_enable_mqtt_transport` so it contributes
  zero code size when disabled.
- No breaking changes to `DeviceCommissioner` or existing controller APIs.

**Non-Goals:**

- Re-encoding Matter payloads — the proxy forwards opaque byte payloads as
  produced by the existing IM engine.
- Implementing MQTT, WebSocket, AMQP, or any cloud protocol directly in C++;
  protocol implementations live in the external connection manager.
- Additional adapters beyond MQTT in this change (protocol registry supports
  them; implementations are deferred).
- Thread-safety guarantees beyond what Pigweed's task runner already provides;
  callers must dispatch on the chip task.

## Decisions

### D0: Clean controller interface — proxy as the sole API boundary

**Decision**: `ControllerProxy` exposes the **only** API the Matter controller
calls. Its methods are named in controller terms (`SendMessage`, `SetAdapter`,
etc.) and carry no mention of MQTT, IPC, topics, QoS, or any other protocol
concept. All protocol selection, session mapping, and connection dispatch are
internal to the proxy and its adapters.

**Rationale**: The controller must be insulated from protocol churn. If a
deployment switches from MQTT to WebSocket, or adds a second parallel transport,
the controller call-sites must not change. Keeping the interface clean also
enables unit-testing the controller against a stub `ControllerProxy` with no
real transport dependency. Any protocol detail that leaks into the controller
interface becomes a breaking API change the moment that detail changes.

### D1: Interface style — virtual dispatch vs. static polymorphism

**Decision**: Pure virtual `TransportProtocol` base class (runtime
polymorphism).

**Rationale**: The number of concurrently registered protocols is small (1–3)
and selected at startup, so the vtable overhead is negligible. Static
polymorphism (CRTP) would require the proxy to be templated on the adapter type,
propagating that template parameter throughout the call stack and significantly
increasing code complexity and binary size through template instantiation.

**Alternative considered**: Function-pointer table (C-style). Rejected — C++17
idioms (e.g., `std::optional`, `chip::Span`) used elsewhere in the SDK make a
class hierarchy more consistent.

### D2: Future/async model — `pw::async::Future` vs. callbacks vs. `CHIP_ERROR` + callback

**Decision**: Use `pw::async::Future<CHIP_ERROR>` for `Send`, and a registered
`MessageHandler` callback (not a future) for `Receive`.

**Rationale**: `Send` is a one-shot operation with a single completion; a future
models this cleanly and composes with `pw::async::Task`. `Receive` is a
continuous stream; modelling it as a future would require re-arming on every
message. A lightweight `MessageHandler` callback registered once at proxy
construction avoids allocating a future per message and matches how the existing
IM engine handles inbound data.

**Alternative considered**: Full Rx streams (pw::async::Channel). Provides
cleaner composition but adds significant dependency weight; deferred to a future
refactor.

### D3: MQTT client implementation — external connection manager

**Decision**: The MQTT broker connection is managed by an external process or
runtime, implemented in one or more non-C++ languages (language(s) TBD). The C++
`MqttTransportAdapter` acts as a bridge: it does not embed an MQTT library but
instead communicates with the external manager over an IPC channel.

**Rationale**: Embedding a full MQTT client (including TLS, reconnect logic, and
buffering) in C++ adds significant complexity and binary size, and introduces
licensing constraints. Delegating connection management to a separate process
means the MQTT implementation can use mature, battle-tested MQTT libraries
available in higher-level languages (Python, Go, Rust, etc.) without any of
those dependencies entering the C++ build. It also allows the connection
lifecycle to be controlled independently of the Matter controller process (e.g.,
shared by multiple controllers, restarted independently on failure).

**Alternative considered**: Embedding Paho embedded-c directly in C++. Rejected
due to EPL-2.0 licensing friction and the maintenance burden of owning TLS and
reconnect logic in C++.

### D4: Topic namespace layout for MQTT

**Decision**: Use a fixed two-topic-per-session layout:

- `chip/proxy/<session-id>/cmd` — controller → device (commands published here)
- `chip/proxy/<session-id>/rsp` — device → controller (responses subscribed
  here)

Session IDs are 128-bit random values encoded as lowercase hex.

**Rationale**: A single command topic and a single response topic per logical
session is sufficient for the request/response pattern the IM engine uses.
Separating topics by direction allows fine-grained ACL rules at the broker.
Wildcard subscriptions (`chip/proxy/+/rsp`) allow a single MQTT connection to
multiplex many sessions.

### D5: Protocol registry

**Decision**: A simple `ProtocolRegistry` singleton holding a
`chip::Span<TransportProtocol *>` provided at initialisation time (no dynamic
registration after startup).

**Rationale**: Dynamic registration (e.g., `Register()`/`Unregister()`) adds
complexity and potential use-after-free risk. In practice, the set of protocols
is known at build time. Providing the span at init keeps the registry
allocation-free and lock-free.

### D6: IPC channel between C++ adapter and external connection manager

**Decision**: Use a Unix domain socket with a simple length-prefixed binary
framing. Each frame carries a message type byte, a session-ID field, and a
payload. The C++ adapter is the client; the external manager is the server.

**Rationale**: Unix sockets are available on all Linux/macOS targets, have
near-zero latency for local IPC, and do not require a network stack. Framing is
minimal — a 4-byte little-endian length prefix is sufficient. More complex
schemes (gRPC, Cap'n Proto) were considered but add heavy dependencies for what
is essentially a local message pipe. The protocol is intentionally small: only
four message types are needed — Connect, Send, Receive, and Status.

**Alternative considered**: stdin/stdout pipes. Rejected — does not support
bidirectional async messaging without additional multiplexing.

## Risks / Trade-offs

- **MQTT latency** → Round-trip adds broker hop latency. Mitigation: document
  expected latency range; expose `QoS` configuration so callers can trade
  reliability for speed.
- **Session correlation under reconnect** → If the MQTT connection drops mid-
  command, in-flight futures may never complete. Mitigation:
  `MqttTransportAdapter` tracks in-flight sends and completes them with
  `CHIP_ERROR_CONNECTION_ABORTED` on disconnect; upper layers already handle
  retransmission.
- **IPC channel failure** → If the external connection manager crashes or the
  Unix socket is unavailable, `Connect` fails and in-flight `Send` futures
  resolve with `CHIP_ERROR_CONNECTION_ABORTED`. Mitigation: the external manager
  should be supervised (e.g., systemd); the adapter exposes connection-state
  notifications so callers can retry.
- **Binary size on constrained targets** → The IPC bridge adds minimal flash vs.
  embedding a full MQTT client. Mitigation: `chip_enable_mqtt_transport = false`
  by default; platforms opt in explicitly.
- **Single-threaded assumption** → Proxy and adapter must be called from the
  chip task. Mitigation: add `assertChipStackLockedByCurrentThread()` assertions
  in debug builds.

## Open Questions

- [ ] Which language(s) will implement the external connection manager? (Python,
      Go, Rust, Node.js are all candidates.) This determines packaging,
      deployment, and the test harness.
- [ ] Should the IPC socket path be configurable at runtime, compile time, or
      both?
- [ ] Should `session-id` in MQTT topics be the Matter session ID or an
      independent UUID? (Independence avoids leaking Matter internals to the
      broker; Matter session IDs can change on reconnect.)
- [ ] Do we need a `Disconnect` future on `TransportProtocol`, or is
      destructor-based teardown sufficient for the current use cases?
- [ ] Who is responsible for launching and supervising the external manager
      process — the application, a platform service, or the adapter itself?
