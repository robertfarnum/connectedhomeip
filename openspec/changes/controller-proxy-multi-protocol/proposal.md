## Why

Matter controllers today require direct protocol connectivity to devices,
limiting deployment in environments where the controller cannot maintain
persistent connections or must integrate with existing IoT infrastructure (e.g.,
cloud-hosted MQTT brokers, industrial message buses). A controller proxy layer
with a pluggable, bidirectional multi-future protocol abstraction allows
controllers to operate over heterogeneous transports without coupling core logic
to any single protocol.

## What Changes

- Introduce a `ControllerProxy` component that decouples the Matter controller
  from its transport, forwarding commands and receiving responses over pluggable
  protocol adapters.
- Define a `TransportProtocol` abstraction (bidirectional, future-based): each
  adapter exposes `Send` (returns a future for delivery acknowledgement) and
  `Receive` (returns an async stream of inbound messages).
- Implement the first concrete adapter: `MqttTransportAdapter` — bidirectional
  MQTT transport using publish/subscribe topics mapped to controller proxy
  channels.
- Provide a protocol registry so additional transports (e.g., WebSocket, AMQP)
  can be registered and selected at runtime.

## Capabilities

### New Capabilities

- `controller-proxy`: Core proxy component — routes controller commands to a
  registered transport adapter and delivers responses back to the controller.
  Manages session lifetime and adapter lifecycle.
- `transport-protocol`: Abstract bidirectional protocol interface with
  future-based send and async receive. Defines the contract all transport
  adapters must satisfy.
- `mqtt-transport`: MQTT adapter implementing `transport-protocol`. Supports
  configurable broker, topic namespacing, QoS levels, TLS, and reconnect policy.

### Modified Capabilities

## Impact

- **New code**: `src/controller/proxy/` (proxy core, protocol registry);
  `src/controller/proxy/mqtt/` (MQTT adapter).
- **Dependencies**: Requires an MQTT client library (e.g., Eclipse Paho C++ or
  similar); to be evaluated in design.
- **APIs**: New public C++ interfaces — `ControllerProxy`, `TransportProtocol`,
  `MqttTransportAdapter`.
- **No breaking changes** to existing `DeviceCommissioner` or
  `InteractionModelEngine` APIs; proxy is an optional layer.
- **Build**: New GN/CMake targets; MQTT adapter gated behind a feature flag
  (`chip_enable_mqtt_transport`).
