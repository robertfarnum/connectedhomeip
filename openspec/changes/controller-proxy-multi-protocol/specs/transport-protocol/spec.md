## ADDED Requirements

### Requirement: Adapter exposes a future-based Send operation

Every `TransportProtocol` implementation SHALL provide a `Send` method that
accepts an opaque byte payload (`chip::Span<const uint8_t>`) and a session ID,
and returns a `pw::async::Future<CHIP_ERROR>` that resolves when the transport
has accepted the payload for delivery.

#### Scenario: Send completes successfully

- **WHEN** `TransportProtocol::Send` is called with a non-empty payload
- **THEN** the returned future SHALL resolve to `CHIP_NO_ERROR` once the
  underlying transport has accepted the message

#### Scenario: Send fails due to transport error

- **WHEN** `TransportProtocol::Send` is called but the underlying transport
  reports a connection error
- **THEN** the returned future SHALL resolve to a non-`CHIP_NO_ERROR` value
  describing the failure (e.g., `CHIP_ERROR_CONNECTION_ABORTED`)

### Requirement: Adapter exposes a callback-based Receive registration

Every `TransportProtocol` implementation SHALL accept a `MessageHandler`
callback at or before the first call to `Connect`. The implementation SHALL
invoke this callback on the chip task for every inbound message.

#### Scenario: Inbound message triggers callback

- **WHEN** the transport receives a message addressed to a registered session
- **THEN** the adapter SHALL invoke the registered `MessageHandler` with the
  payload span and session ID on the chip task thread

#### Scenario: Callback invoked before Connect returns

- **WHEN** a message arrives after `Connect` completes but before the caller
  processes any futures
- **THEN** the adapter SHALL queue the callback invocation and deliver it on the
  chip task without dropping it

### Requirement: Adapter exposes Connect and Disconnect lifecycle methods

Every `TransportProtocol` implementation SHALL provide:

- `Connect() -> pw::async::Future<CHIP_ERROR>`: establish the underlying
  connection. Future resolves to `CHIP_NO_ERROR` on success.
- `Disconnect()`: synchronously initiate teardown. In-flight `Send` futures
  SHALL resolve to `CHIP_ERROR_CONNECTION_ABORTED`.

#### Scenario: Connect succeeds

- **WHEN** `TransportProtocol::Connect` is called on an unconnected adapter with
  valid configuration
- **THEN** the future SHALL resolve to `CHIP_NO_ERROR` and the adapter SHALL be
  ready to send and receive

#### Scenario: Disconnect completes in-flight sends

- **WHEN** `TransportProtocol::Disconnect` is called while one or more `Send`
  futures are pending
- **THEN** all pending futures SHALL resolve to `CHIP_ERROR_CONNECTION_ABORTED`
  before the adapter releases resources

### Requirement: Adapter implementations SHALL be called only on the chip task

All methods of `TransportProtocol` (`Send`, `Connect`, `Disconnect`, and the
`MessageHandler` callback) SHALL be invoked exclusively on the chip stack task.
Implementations are not required to be thread-safe.

#### Scenario: Enforcement in debug builds

- **WHEN** any `TransportProtocol` method is called from a thread other than the
  chip task in a debug build
- **THEN** an assertion SHALL fire via `assertChipStackLockedByCurrentThread()`
