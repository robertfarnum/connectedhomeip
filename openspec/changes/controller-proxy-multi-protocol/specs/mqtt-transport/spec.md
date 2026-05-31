## ADDED Requirements

### Requirement: MQTT adapter implements TransportProtocol as an IPC bridge

`MqttTransportAdapter` SHALL implement all methods of `TransportProtocol`
(`Connect`, `Disconnect`, `Send`, and `MessageHandler` registration) by
delegating to an external MQTT connection manager over a Unix domain socket IPC
channel. The adapter SHALL NOT contain an embedded MQTT client library.

#### Scenario: Adapter satisfies TransportProtocol contract

- **WHEN** `MqttTransportAdapter` is used wherever a `TransportProtocol*` is
  accepted
- **THEN** all `TransportProtocol` requirements SHALL be satisfied

### Requirement: MQTT adapter connects to the external connection manager

`MqttTransportAdapter::Connect` SHALL open a Unix domain socket connection to
the external MQTT connection manager. If the socket is unavailable or the
manager rejects the connection, the `Connect` future SHALL resolve to
`CHIP_ERROR_CONNECTION_ABORTED`.

#### Scenario: Manager available at connect time

- **WHEN** `Connect` is called and the external manager is listening on the
  configured socket path
- **THEN** the future SHALL resolve to `CHIP_NO_ERROR` once the IPC handshake
  completes

#### Scenario: Manager unavailable at connect time

- **WHEN** `Connect` is called and no process is listening on the socket path
- **THEN** the future SHALL resolve to `CHIP_ERROR_CONNECTION_ABORTED` within a
  configurable timeout

### Requirement: MQTT adapter uses per-session publish/subscribe topics

The adapter SHALL map each session to exactly two MQTT topics:

- `chip/proxy/<session-id>/cmd` — outbound (publish)
- `chip/proxy/<session-id>/rsp` — inbound (subscribe)

`<session-id>` SHALL be a 128-bit random value encoded as 32 lowercase hex
characters, independent of the Matter session ID.

#### Scenario: Outbound message published to correct topic

- **WHEN** `Send` is called with a payload and session ID `S`
- **THEN** the adapter SHALL publish the payload to `chip/proxy/<S>/cmd`

#### Scenario: Inbound subscription covers all sessions

- **WHEN** `Connect` completes successfully
- **THEN** the adapter SHALL subscribe to the wildcard topic `chip/proxy/+/rsp`
  so a single MQTT connection multiplexes all sessions

### Requirement: Broker configuration is owned by the external connection manager

Broker hostname, port, TLS settings, credentials, and reconnect policy SHALL be
configured in the external connection manager, not in `MqttTransportAdapter`.
The C++ adapter SHALL only require the socket path used to reach the manager.

#### Scenario: Broker config absent from adapter construction

- **WHEN** `MqttTransportAdapter` is constructed with only the IPC socket path
- **THEN** the adapter SHALL compile and operate correctly without any broker
  hostname, port, or TLS parameters

### Requirement: MQTT adapter supports configurable QoS

`MqttTransportAdapter` SHALL accept a QoS level (0, 1, or 2) at construction and
apply it to all outbound publishes and inbound subscriptions.

#### Scenario: QoS 1 delivery acknowledgement

- **WHEN** QoS level 1 is configured and `Send` is called
- **THEN** the future SHALL not resolve until the broker sends a PUBACK, at
  which point it SHALL resolve to `CHIP_NO_ERROR`

#### Scenario: QoS 0 fire-and-forget

- **WHEN** QoS level 0 is configured and `Send` is called
- **THEN** the future SHALL resolve to `CHIP_NO_ERROR` immediately after the
  payload is handed to the MQTT client, without waiting for any broker
  acknowledgement

### Requirement: External connection manager owns MQTT reconnect policy

Reconnection to the MQTT broker after connection loss SHALL be handled entirely
by the external connection manager. The C++ adapter SHALL NOT implement broker
reconnect logic.

#### Scenario: Broker reconnect transparent to adapter

- **WHEN** the external manager reconnects to the broker after a transient
  failure without dropping the IPC socket
- **THEN** the adapter SHALL continue sending and receiving messages without
  requiring a new `Connect` call

### Requirement: Adapter handles IPC channel loss

If the IPC socket to the external manager closes unexpectedly, the adapter SHALL
resolve all in-flight `Send` futures with `CHIP_ERROR_CONNECTION_ABORTED` and
SHALL invoke the registered `MessageHandler` with a sentinel error payload (or
omit invocation) to indicate the channel is down. The adapter SHALL transition
to a disconnected state requiring a new `Connect` call before sends can proceed.

#### Scenario: IPC channel closed mid-send

- **WHEN** the IPC socket closes while one or more `Send` futures are pending
- **THEN** each pending future SHALL resolve to `CHIP_ERROR_CONNECTION_ABORTED`

#### Scenario: Adapter rejects sends after IPC loss

- **WHEN** `Send` is called after the IPC channel has been lost and before
  `Connect` is called again
- **THEN** the future SHALL resolve immediately to `CHIP_ERROR_INCORRECT_STATE`

### Requirement: MQTT adapter is disabled by default

The MQTT adapter and its dependencies SHALL be excluded from the build unless
the GN argument `chip_enable_mqtt_transport = true` is set.

#### Scenario: Default build excludes MQTT adapter

- **WHEN** a target is built without setting `chip_enable_mqtt_transport`
- **THEN** no object code from `MqttTransportAdapter` or its MQTT client library
  SHALL appear in the final binary
