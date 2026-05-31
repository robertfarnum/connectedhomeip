/*
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <controller/proxy/TransportProtocol.h>
#include <controller/proxy/mqtt/IpcProtocol.h>
#include <controller/proxy/mqtt/MqttIpcClient.h>
#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace chip {
namespace Controller {
namespace Mqtt {

/// QoS levels forwarded to the external MQTT connection manager.
enum class QosLevel : uint8_t
{
    kAtMostOnce  = 0, ///< QoS 0 — fire and forget
    kAtLeastOnce = 1, ///< QoS 1 — acknowledged delivery
    kExactlyOnce = 2, ///< QoS 2 — assured delivery
};

/// MQTT IPC transport adapter.
///
/// MqttTransportAdapter implements TransportProtocol by bridging the Matter
/// controller to an external MQTT connection manager over a Unix domain socket.
/// The adapter contains no embedded MQTT library — all broker connectivity,
/// reconnect logic, TLS, and credentials are owned by the external manager.
///
/// Per-session MQTT topic layout (managed by the external process):
///   Outbound: chip/proxy/<session-id>/cmd
///   Inbound:  chip/proxy/<session-id>/rsp
///
/// Session IDs are 128-bit values (16 bytes) rendered as 32 lowercase hex
/// characters on the wire, assigned independently of Matter session IDs.
///
/// Thread safety: All methods MUST be called on the chip stack task.
class MqttTransportAdapter : public TransportProtocol
{
public:
    /// Construct an adapter.
    ///
    /// @param socketPath  Path to the Unix domain socket served by the external
    ///                    MQTT connection manager.  Must remain valid for the
    ///                    lifetime of this object.
    /// @param qos         QoS level to request for published messages.
    MqttTransportAdapter(const char * socketPath, QosLevel qos);
    ~MqttTransportAdapter() override;

    // Non-copyable.
    MqttTransportAdapter(const MqttTransportAdapter &)             = delete;
    MqttTransportAdapter & operator=(const MqttTransportAdapter &) = delete;

    // TransportProtocol interface.
    void SetMessageHandler(MessageHandler handler) override;
    void Connect(TransportCompletionCallback callback) override;
    void Disconnect() override;
    void Send(chip::ByteSpan payload, uint64_t sessionId, TransportCompletionCallback callback) override;

protected:
    /// Dispatch an IPC frame to the adapter state machine.
    ///
    /// Exposed as protected to allow test subclasses to inject frames without
    /// requiring a live socket.
    void OnIpcFrame(IpcMessageType type, chip::ByteSpan sessionId, chip::ByteSpan payload);

private:
    /// Convert a 64-bit sessionId to the 16-byte wire representation stored in
    /// the IPC session ID field.
    static void SessionIdToBytes(uint64_t sessionId, uint8_t out[kIpcSessionIdLen]);

    const char *  mSocketPath;
    QosLevel      mQos;
    MqttIpcClient mIpcClient;
    MessageHandler mMessageHandler;

    /// Pending Connect callback (cleared on receipt of Status frame).
    TransportCompletionCallback mConnectCallback;

    /// Pending Send callbacks keyed by sequence number.
    /// Simple FIFO list; Status frames arrive in send order.
    std::vector<TransportCompletionCallback> mPendingSends;
};

} // namespace Mqtt
} // namespace Controller
} // namespace chip
