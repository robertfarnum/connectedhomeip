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

/// Tests for MqttTransportAdapter state machine.
///
/// Uses a test-double subclass that overrides Connect/Send/Disconnect to skip
/// actual Unix socket I/O, and injects IPC frames via OnIpcFrame (protected).

#include <pw_unit_test/framework.h>

#include <controller/proxy/mqtt/IpcProtocol.h>
#include <controller/proxy/mqtt/MqttTransportAdapter.h>

#include <cstring>
#include <vector>

namespace {

using namespace chip::Controller;
using namespace chip::Controller::Mqtt;

// ---------------------------------------------------------------------------
// IPC protocol constant tests
// ---------------------------------------------------------------------------

TEST(IpcProtocol, HeaderLenIs21)
{
    // 1 (msgtype) + 4 (payload_len) + 16 (session_id) = 21
    EXPECT_EQ(kIpcHeaderLen, static_cast<uint8_t>(21));
}

TEST(IpcProtocol, SessionIdLenIs16)
{
    EXPECT_EQ(kIpcSessionIdLen, static_cast<uint8_t>(16));
}

TEST(IpcProtocol, MessageTypeValues)
{
    EXPECT_EQ(static_cast<uint8_t>(IpcMessageType::kConnect), 0x01u);
    EXPECT_EQ(static_cast<uint8_t>(IpcMessageType::kDisconnect), 0x02u);
    EXPECT_EQ(static_cast<uint8_t>(IpcMessageType::kSend), 0x03u);
    EXPECT_EQ(static_cast<uint8_t>(IpcMessageType::kReceive), 0x04u);
    EXPECT_EQ(static_cast<uint8_t>(IpcMessageType::kStatus), 0x05u);
}

TEST(IpcProtocol, MaxPayloadIs1MiB)
{
    EXPECT_EQ(kIpcMaxPayloadLen, 1u * 1024u * 1024u);
}

TEST(QosLevel, Values)
{
    EXPECT_EQ(static_cast<uint8_t>(QosLevel::kAtMostOnce), 0u);
    EXPECT_EQ(static_cast<uint8_t>(QosLevel::kAtLeastOnce), 1u);
    EXPECT_EQ(static_cast<uint8_t>(QosLevel::kExactlyOnce), 2u);
}

// ---------------------------------------------------------------------------
// StubMqttAdapter: bypasses socket I/O; drives state machine directly.
// ---------------------------------------------------------------------------

class StubMqttAdapter : public MqttTransportAdapter
{
public:
    explicit StubMqttAdapter(QosLevel qos = QosLevel::kAtLeastOnce) :
        MqttTransportAdapter(kFakeSocketPath, qos) {}

    // Override to skip socket I/O; just store the callback.
    void Connect(TransportCompletionCallback callback) override { mConnectCb = std::move(callback); }

    void Send(chip::ByteSpan payload, uint64_t sessionId, TransportCompletionCallback callback) override
    {
        mSendPayloads.emplace_back(payload.begin(), payload.end());
        mSendSessionIds.push_back(sessionId);
        mSendCbs.push_back(std::move(callback));
    }

    void Disconnect() override
    {
        mDisconnectCount++;
        for (auto & cb : mSendCbs)
        {
            if (cb)
            {
                cb(CHIP_ERROR_CONNECTION_ABORTED);
            }
        }
        mSendCbs.clear();
        if (mConnectCb)
        {
            auto cb    = std::move(mConnectCb);
            mConnectCb = nullptr;
            cb(CHIP_ERROR_CONNECTION_ABORTED);
        }
    }

    /// Inject a simulated Status frame from the external manager.
    void SimulateStatus(bool success)
    {
        uint8_t statusPayload             = success ? 0x00u : 0x01u;
        uint8_t zeroSession[kIpcSessionIdLen] = {};
        OnIpcFrame(IpcMessageType::kStatus, chip::ByteSpan(zeroSession), chip::ByteSpan(&statusPayload, 1));
    }

    /// Inject a simulated Receive frame (inbound MQTT message).
    void SimulateReceive(chip::ByteSpan sessionId, chip::ByteSpan payload)
    {
        OnIpcFrame(IpcMessageType::kReceive, sessionId, payload);
    }

    // Inspection.
    TransportCompletionCallback              mConnectCb;
    std::vector<TransportCompletionCallback> mSendCbs;
    std::vector<std::vector<uint8_t>>        mSendPayloads;
    std::vector<uint64_t>                    mSendSessionIds;
    int                                      mDisconnectCount = 0;

private:
    static constexpr const char * kFakeSocketPath = "/tmp/chip-mqtt-test.sock";
};

// ---------------------------------------------------------------------------
// State machine tests
// ---------------------------------------------------------------------------

TEST(MqttTransportAdapter, ConnectCallbackResolvedBySuccessStatus)
{
    StubMqttAdapter adapter;
    CHIP_ERROR connectResult = CHIP_ERROR_INTERNAL;
    adapter.Connect([&](CHIP_ERROR e) { connectResult = e; });
    EXPECT_TRUE(adapter.mConnectCb); // pending

    adapter.SimulateStatus(true);
    EXPECT_EQ(connectResult, CHIP_NO_ERROR);
}

TEST(MqttTransportAdapter, ConnectCallbackResolvedByFailStatus)
{
    StubMqttAdapter adapter;
    CHIP_ERROR connectResult = CHIP_NO_ERROR;
    adapter.Connect([&](CHIP_ERROR e) { connectResult = e; });

    adapter.SimulateStatus(false);
    EXPECT_NE(connectResult, CHIP_NO_ERROR);
}

TEST(MqttTransportAdapter, SendCallbackQueuedAndResolvedByStatus)
{
    StubMqttAdapter adapter;
    adapter.Connect(nullptr);
    adapter.SimulateStatus(true); // connect resolved

    const uint8_t data[]   = { 0xAB, 0xCD };
    CHIP_ERROR sendResult  = CHIP_ERROR_INTERNAL;
    adapter.Send(chip::ByteSpan(data), 42u, [&](CHIP_ERROR e) { sendResult = e; });
    EXPECT_EQ(adapter.mSendCbs.size(), 1u);

    adapter.SimulateStatus(true);
    EXPECT_EQ(sendResult, CHIP_NO_ERROR);
}

TEST(MqttTransportAdapter, MultipleSendCallbacksResolvedInOrder)
{
    StubMqttAdapter adapter;
    adapter.Connect(nullptr);
    adapter.SimulateStatus(true);

    const uint8_t d[] = { 1 };
    int resolved = 0;
    adapter.Send(chip::ByteSpan(d), 1u, [&](CHIP_ERROR) { resolved = 1; });
    adapter.Send(chip::ByteSpan(d), 2u, [&](CHIP_ERROR) { resolved = 2; });

    adapter.SimulateStatus(true); // resolves first Send
    EXPECT_EQ(resolved, 1);

    adapter.SimulateStatus(true); // resolves second Send
    EXPECT_EQ(resolved, 2);
}

TEST(MqttTransportAdapter, DisconnectAbortsAllPendingSends)
{
    StubMqttAdapter adapter;
    adapter.Connect(nullptr);
    adapter.SimulateStatus(true);

    const uint8_t d[]       = { 0xFF };
    int abortedCount        = 0;
    adapter.Send(chip::ByteSpan(d), 10u, [&](CHIP_ERROR e) {
        if (e == CHIP_ERROR_CONNECTION_ABORTED)
        {
            abortedCount++;
        }
    });
    adapter.Send(chip::ByteSpan(d), 11u, [&](CHIP_ERROR e) {
        if (e == CHIP_ERROR_CONNECTION_ABORTED)
        {
            abortedCount++;
        }
    });

    adapter.Disconnect();
    EXPECT_EQ(abortedCount, 2);
}

TEST(MqttTransportAdapter, ReceiveFrameInvokesMessageHandler)
{
    StubMqttAdapter adapter;

    bool           called    = false;
    uint64_t       gotSid    = 0;
    std::vector<uint8_t> gotPayload;

    adapter.SetMessageHandler([&](chip::ByteSpan payload, uint64_t sessionId) {
        called     = true;
        gotSid     = sessionId;
        gotPayload.assign(payload.begin(), payload.end());
    });

    uint8_t sessionBytes[kIpcSessionIdLen] = {};
    const uint64_t kTestSession            = 77u;
    ::memcpy(sessionBytes, &kTestSession, sizeof(kTestSession));

    const uint8_t msg[] = { 0xDE, 0xAD };
    adapter.SimulateReceive(chip::ByteSpan(sessionBytes), chip::ByteSpan(msg));

    EXPECT_TRUE(called);
    EXPECT_EQ(gotSid, kTestSession);
    ASSERT_EQ(gotPayload.size(), 2u);
    EXPECT_EQ(gotPayload[0], 0xDEu);
}

} // namespace
