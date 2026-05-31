## ADDED Requirements

### Requirement: Proxy exposes a protocol-agnostic controller interface

The `ControllerProxy` public API SHALL contain no references to any specific
cloud communication protocol (MQTT, WebSocket, AMQP, etc.), IPC mechanism, topic
names, QoS levels, or connection manager details. Callers SHALL interact with
the proxy exclusively through protocol-neutral methods.

#### Scenario: Controller API contains no protocol identifiers

- **WHEN** the `ControllerProxy` header is compiled without any
  `chip_enable_*_transport` flag set
- **THEN** it SHALL compile successfully and expose the same public method
  signatures regardless of which adapters are available at link time

#### Scenario: Switching adapters requires no controller call-site changes

- **WHEN** the active `TransportProtocol` adapter is replaced via `SetAdapter`
- **THEN** all existing callers of `ControllerProxy::SendMessage` and the
  inbound `MessageHandler` SHALL continue to operate without modification

### Requirement: Proxy forwards commands to a registered transport adapter

The `ControllerProxy` SHALL accept encoded Matter IM messages from the
controller and forward them to the currently active `TransportProtocol` adapter.
It SHALL NOT re-encode or inspect the payload content.

#### Scenario: Command forwarded successfully

- **WHEN** `ControllerProxy::SendMessage` is called with a valid encoded payload
  and a session ID
- **THEN** the proxy SHALL invoke `TransportProtocol::Send` with the payload and
  SHALL return a future that resolves to `CHIP_NO_ERROR` once the adapter
  acknowledges delivery

#### Scenario: No adapter registered

- **WHEN** `ControllerProxy::SendMessage` is called and no adapter has been
  registered
- **THEN** the proxy SHALL return a future that resolves immediately to
  `CHIP_ERROR_INCORRECT_STATE`

### Requirement: Proxy delivers inbound messages to the controller

The `ControllerProxy` SHALL register a `MessageHandler` with the active adapter
at construction and SHALL invoke the registered inbound handler with every
message received from the adapter.

#### Scenario: Inbound message delivered

- **WHEN** the registered adapter calls the proxy's inbound callback with a
  received payload
- **THEN** the proxy SHALL invoke the `MessageHandler` supplied at construction
  with the payload and the originating session ID without copying the payload
  data

#### Scenario: No inbound handler registered

- **WHEN** the adapter delivers a message and no `MessageHandler` was supplied
  at construction
- **THEN** the proxy SHALL discard the message and log a warning via
  `ChipLogError`

### Requirement: Proxy manages adapter lifecycle

The `ControllerProxy` SHALL own the lifetime of its registered adapter. When the
proxy is destroyed, it SHALL call `TransportProtocol::Disconnect` on the adapter
before releasing it.

#### Scenario: Proxy shutdown tears down adapter

- **WHEN** `ControllerProxy` destructor runs
- **THEN** `TransportProtocol::Disconnect` SHALL be called on the registered
  adapter before any proxy state is released

### Requirement: Only one adapter active at a time

The `ControllerProxy` SHALL support exactly one active `TransportProtocol`
adapter at a time. Registering a second adapter SHALL replace the first after
disconnecting it.

#### Scenario: Adapter replacement disconnects prior adapter

- **WHEN** `ControllerProxy::SetAdapter` is called while an adapter is already
  registered
- **THEN** the proxy SHALL call `Disconnect` on the existing adapter, then
  register the new adapter and call `Connect` on it
