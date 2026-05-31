# Controller Proxy

The controller proxy provides a **protocol-agnostic** bridge between the Matter
controller layer and one or more cloud communication back-ends. Callers (device
controller code, commissioning flows, etc.) send and receive opaque byte
payloads with no knowledge of which transport protocol is active.

## Architecture

```
ControllerProxy          TransportProtocol (abstract)
      |                         |
      | SetAdapter()            |-- MqttTransportAdapter  (IPC bridge to external manager)
      | SendMessage()           |-- (future: WebSocketAdapter, AMQPAdapter, …)
      | MessageHandler callback |
```

The three key components are:

| Component           | Location                                      | Purpose                                                           |
| ------------------- | --------------------------------------------- | ----------------------------------------------------------------- |
| `TransportProtocol` | `src/controller/proxy/TransportProtocol.h`    | Abstract interface — Connect, Disconnect, Send, SetMessageHandler |
| `ControllerProxy`   | `src/controller/proxy/ControllerProxy.h/.cpp` | Protocol-neutral proxy; owns one adapter                          |
| `ProtocolRegistry`  | `src/controller/proxy/ProtocolRegistry.h`     | Allocation-free lookup of adapters by index                       |

The MQTT adapter lives under `src/controller/proxy/mqtt/` and is disabled by
default (see [MQTT Transport](#mqtt-transport)).

## Using ControllerProxy

```cpp
#include <controller/proxy/ControllerProxy.h>

// 1. Create a proxy with an inbound message handler.
chip::Controller::ControllerProxy proxy(
    [](chip::ByteSpan payload, uint64_t sessionId) {
        // Handle inbound message on chip task.
    });

// 2. Attach a transport adapter and connect.
MyTransportAdapter adapter;
proxy.SetAdapter(&adapter, [](CHIP_ERROR err) {
    if (err != CHIP_NO_ERROR) {
        ChipLogError(Controller, "Connect failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
});

// 3. Send a message.
const uint8_t data[] = { 0x01, 0x02, 0x03 };
proxy.SendMessage(chip::ByteSpan(data), sessionId, [](CHIP_ERROR err) {
    // Called when the transport has accepted or rejected the payload.
});
```

All methods must be called on the chip stack task.

## Implementing a Custom Transport

Subclass `chip::Controller::TransportProtocol`:

```cpp
#include <controller/proxy/TransportProtocol.h>
#include <platform/LockTracker.h>

class MyTransport : public chip::Controller::TransportProtocol
{
public:
    void SetMessageHandler(chip::Controller::MessageHandler handler) override
    {
        assertChipStackLockedByCurrentThread();
        mHandler = std::move(handler);
    }

    void Connect(chip::Controller::TransportCompletionCallback callback) override
    {
        assertChipStackLockedByCurrentThread();
        // ... initiate connection ...
        callback(CHIP_NO_ERROR);
    }

    void Disconnect() override
    {
        assertChipStackLockedByCurrentThread();
        // Abort all in-flight Sends before returning.
    }

    void Send(chip::ByteSpan payload, uint64_t sessionId,
              chip::Controller::TransportCompletionCallback callback) override
    {
        assertChipStackLockedByCurrentThread();
        // ... transmit payload, invoke callback on completion ...
    }

private:
    chip::Controller::MessageHandler mHandler;
};
```

## Protocol Registry

`ProtocolRegistry` allows selecting adapters by a compact numeric index, useful
when multiple transports are registered at startup:

```cpp
MyTransport a, b;
chip::Controller::TransportProtocol * adapters[] = { &a, &b };
chip::Controller::ProtocolRegistry reg(chip::Span<chip::Controller::TransportProtocol *>(adapters));

chip::Controller::TransportProtocol * chosen = reg.Lookup(0); // returns &a
```

## MQTT Transport

The MQTT adapter (`MqttTransportAdapter`) bridges the proxy to an **external
MQTT connection manager** over a Unix domain socket. The external manager owns
all broker connectivity, TLS, reconnect logic, and credentials — the C++ adapter
contains no embedded MQTT library.

### Enabling

Add the build arg `chip_enable_mqtt_transport=true` to your GN invocation:

```sh
scripts/run_in_build_env.sh \
  "./scripts/build/build_examples.py \
    --target linux-x64-tests-clang \
    --args 'chip_enable_mqtt_transport=true' \
    build"
```

### Usage

```cpp
#include <controller/proxy/mqtt/MqttTransportAdapter.h>

using namespace chip::Controller::Mqtt;

MqttTransportAdapter adapter("/run/chip/mqtt-manager.sock", QosLevel::kAtLeastOnce);
proxy.SetAdapter(&adapter, connectCb);
```

### IPC Wire Format

Each IPC frame uses the following layout (all integers little-endian):

```
+----------+-----------------+-------------------------------+---------+
| msg_type | payload_len (4B)| session_id (16B)              | payload |
+----------+-----------------+-------------------------------+---------+
   1 byte       4 bytes              16 bytes                 variable
```

Message types:

| Type          | Byte   | Direction | Description                           |
| ------------- | ------ | --------- | ------------------------------------- |
| `kConnect`    | `0x01` | C→M       | Request broker connection             |
| `kDisconnect` | `0x02` | C→M       | Request broker disconnection          |
| `kSend`       | `0x03` | C→M       | Publish payload on session topic      |
| `kReceive`    | `0x04` | M→C       | Inbound message on session topic      |
| `kStatus`     | `0x05` | M→C       | Result of Connect or Send (0x00 = ok) |

### MQTT Topic Layout

The external manager uses the following topic scheme for per-session routing:

- Outbound: `chip/proxy/<session-id>/cmd`
- Inbound: `chip/proxy/<session-id>/rsp`

Session IDs are 128-bit values (16 bytes) rendered as 32 lowercase hex
characters.

### External Manager

The external manager may be implemented in any language. It must:

1. Listen on the configured Unix socket path.
2. Parse IPC frames per the wire format above.
3. On `kConnect`: connect to the configured MQTT broker and reply with
   `kStatus`.
4. On `kSend`: publish the payload to `chip/proxy/<session-id>/cmd` and reply
   with `kStatus` when the broker acknowledges (QoS ≥ 1) or immediately (QoS 0).
5. On inbound MQTT messages from `chip/proxy/+/rsp`: send a `kReceive` frame to
   the C++ adapter.
6. On broker reconnect: no special action required — the C++ side does not need
   to call `Connect` again.

## Testing

Run the core proxy tests:

```sh
scripts/run_in_build_env.sh \
  "ninja -C out/linux-x64-tests-clang \
   src/controller/proxy/tests:TestControllerProxy.run"
```

Run the MQTT adapter tests (requires `chip_enable_mqtt_transport=true`):

```sh
scripts/run_in_build_env.sh \
  "ninja -C out/linux-x64-tests-clang \
   src/controller/proxy/mqtt/tests:TestMqttTransportAdapter.run"
```
