## 1. Prerequisites

- [x] 1.1 Confirm `pw::async::Future` availability in the SDK's current Pigweed
      pin; update pigweed.json if needed

## 2. TransportProtocol Interface

- [x] 2.1 Create `src/controller/proxy/TransportProtocol.h` — pure virtual base
      class with `Connect`, `Disconnect`, `Send`, and `SetMessageHandler`
- [x] 2.2 Define `MessageHandler` as
      `chip::Callback::Callback<void(chip::Span<const uint8_t>, uint64_t sessionId)>`
      (or equivalent on-chip-task callback type)
- [x] 2.3 Add `assertChipStackLockedByCurrentThread()` guard macros in debug
      builds for all interface entry points
- [x] 2.4 Add `TransportProtocol.h` to `src/controller/proxy/BUILD.gn` and
      `app_config_dependent_sources.cmake`

## 3. ControllerProxy Core

- [x] 3.1 Create `src/controller/proxy/ControllerProxy.h` and
      `ControllerProxy.cpp` — owns a `TransportProtocol*`, exposes `SendMessage`
      returning `pw::async::Future<CHIP_ERROR>`, and `SetAdapter` with
      disconnect-then-connect logic
- [x] 3.2 Implement `SetAdapter`: call `Disconnect` on the existing adapter (if
      any), register the new adapter, call `Connect`, register the inbound
      `MessageHandler`
- [x] 3.3 Implement `SendMessage`: delegate to `TransportProtocol::Send`; return
      `CHIP_ERROR_INCORRECT_STATE` future if no adapter registered
- [x] 3.4 Implement inbound path: `MessageHandler` callback invokes the
      `ControllerProxy`-level handler supplied at construction
- [x] 3.5 Implement destructor: call `Disconnect` on the adapter before
      releasing it
- [x] 3.6 Add `ControllerProxy.h/.cpp` to `src/controller/proxy/BUILD.gn`

## 4. Protocol Registry

- [x] 4.1 Create `src/controller/proxy/ProtocolRegistry.h` — holds a
      `chip::Span<TransportProtocol *>` provided at init; exposes
      `Lookup(protocolId)` returning `TransportProtocol*`
- [x] 4.2 Add `ProtocolRegistry.h` to `BUILD.gn`

## 5. Unit Tests — TransportProtocol and ControllerProxy

- [x] 5.1 Create `src/controller/proxy/tests/` directory and `BUILD.gn`
- [x] 5.2 Write `TestTransportProtocol.cpp`: stub adapter satisfying the
      interface; verify `Connect`/`Disconnect` lifecycle, Send future
      resolution, and MessageHandler invocation
- [x] 5.3 Write `TestControllerProxy.cpp`: verify forwarding to adapter,
      INCORRECT_STATE when no adapter, adapter replacement (prior adapter
      disconnected), destructor teardown
- [x] 5.4 Add tests to `src/controller/proxy/tests/BUILD.gn` and register in the
      top-level test target

## 6. MQTT Transport Adapter (IPC bridge)

- [x] 6.1 Define the IPC wire format: length-prefixed binary framing with a
      1-byte message type (Connect, Send, Receive, Status), 16-byte session ID,
      and variable-length payload; document in
      `src/controller/proxy/mqtt/IpcProtocol.h`
- [x] 6.2 Implement `MqttIpcClient`: async Unix domain socket client that reads
      and writes IPC frames on the chip task; exposes `Open(socketPath)`,
      `Close()`, `WriteFrame()`, and an `OnFrameReceived` callback
- [x] 6.3 Create `src/controller/proxy/mqtt/MqttTransportAdapter.h` and
      `MqttTransportAdapter.cpp` — implements `TransportProtocol`; constructor
      takes only the IPC socket path and QoS level
- [x] 6.4 Implement `Connect`: open `MqttIpcClient`, send Connect frame, resolve
      future on Status-OK response from the external manager
- [x] 6.5 Implement `Send`: write a Send frame carrying the session ID and
      payload; resolve future on QoS 0 (immediate) or on Ack frame (QoS 1/2)
- [x] 6.6 Implement inbound dispatch: on Receive frame from IPC client, extract
      session ID and payload, invoke registered `MessageHandler` on chip task
- [x] 6.7 Implement `Disconnect`: send Disconnect frame, close IPC socket,
      resolve all in-flight Send futures with `CHIP_ERROR_CONNECTION_ABORTED`
- [x] 6.8 Implement IPC loss handling: if the socket closes unexpectedly,
      resolve in-flight futures with `CHIP_ERROR_CONNECTION_ABORTED` and
      transition to disconnected state
- [x] 6.9 Add `src/controller/proxy/mqtt/BUILD.gn` with
      `chip_enable_mqtt_transport` guard (no MQTT library dependency — IPC only)

## 7. Unit Tests — MqttTransportAdapter

- [x] 7.1 Write `TestMqttIpcClient.cpp`: mock Unix socket server; verify frame
      encoding/decoding, Open/Close lifecycle, and `OnFrameReceived` delivery on
      chip task
- [x] 7.2 Write `TestMqttTransportAdapter.cpp`: mock `MqttIpcClient`; verify
      Connect frame sent on `Connect`, QoS 0 immediate resolution, QoS 1 waits
      for Ack frame, IPC loss resolves in-flight futures with
      `CHIP_ERROR_CONNECTION_ABORTED`, sends rejected in disconnected state
- [x] 7.3 Add tests to `src/controller/proxy/mqtt/tests/BUILD.gn` guarded by
      `chip_enable_mqtt_transport`

## 8. Build Integration

- [x] 8.1 Add `src/controller/proxy/` to the Linux test target
      (`linux-x64-tests-clang`) so proxy core tests run in CI
- [x] 8.2 Add an opt-in `linux-x64-tests-clang-mqtt` target (or build arg) that
      sets `chip_enable_mqtt_transport=true` and includes the MQTT adapter tests
- [x] 8.3 Verify `chip_enable_mqtt_transport = false` (default) produces zero
      MQTT object code in a stripped binary (check with `nm`)

## 9. Documentation

- [x] 9.1 Add `docs/guides/controller_proxy.md` covering: purpose, the
      protocol-agnostic controller interface, how to register an adapter, the
      IPC protocol format, how to implement a new adapter, and how to build the
      external connection manager
- [x] 9.2 Update `docs/guides/writing_clusters.md` or the relevant controller
      docs to reference the proxy as an optional cloud transport layer
