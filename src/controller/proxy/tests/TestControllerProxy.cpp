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

#include <pw_unit_test/framework.h>

#include <controller/proxy/ControllerProxy.h>
#include <controller/proxy/TransportProtocol.h>

namespace {

using namespace chip::Controller;

/// Stub transport that records calls and lets tests drive outcomes.
class StubTransport : public TransportProtocol
{
public:
    void SetMessageHandler(MessageHandler handler) override { mHandler = std::move(handler); }

    void Connect(TransportCompletionCallback callback) override
    {
        mConnectCallCount++;
        if (callback)
        {
            callback(mNextConnectError);
        }
    }

    void Disconnect() override { mDisconnectCallCount++; }

    void Send(chip::ByteSpan payload, uint64_t sessionId, TransportCompletionCallback callback) override
    {
        mLastPayload.assign(payload.begin(), payload.end());
        mLastSessionId = sessionId;
        mSendCallCount++;
        if (callback)
        {
            callback(mNextSendError);
        }
    }

    /// Simulate receiving a message from the remote.
    void SimulateInbound(chip::ByteSpan payload, uint64_t sessionId)
    {
        if (mHandler)
        {
            mHandler(payload, sessionId);
        }
    }

    MessageHandler          mHandler;
    CHIP_ERROR              mNextConnectError = CHIP_NO_ERROR;
    CHIP_ERROR              mNextSendError    = CHIP_NO_ERROR;
    int                     mConnectCallCount    = 0;
    int                     mDisconnectCallCount = 0;
    int                     mSendCallCount       = 0;
    std::vector<uint8_t>    mLastPayload;
    uint64_t                mLastSessionId = 0;
};

// ---------------------------------------------------------------------------
// TransportProtocol interface contract tests
// ---------------------------------------------------------------------------

TEST(TransportProtocol, StubImplementsInterface)
{
    // Compilation test: StubTransport satisfies the abstract interface.
    StubTransport t;
    TransportProtocol & proto = t;
    (void) proto;
}

TEST(TransportProtocol, SetMessageHandlerStoredAndInvoked)
{
    StubTransport t;
    bool called = false;
    t.SetMessageHandler([&](chip::ByteSpan, uint64_t) { called = true; });

    const uint8_t data[] = { 0xAB };
    t.SimulateInbound(chip::ByteSpan(data), 42u);
    EXPECT_TRUE(called);
}

// ---------------------------------------------------------------------------
// ControllerProxy tests
// ---------------------------------------------------------------------------

TEST(ControllerProxy, SendMessageFailsWithNoAdapter)
{
    ControllerProxy proxy(nullptr);
    CHIP_ERROR result = CHIP_NO_ERROR;
    const uint8_t data[] = { 1, 2 };
    proxy.SendMessage(chip::ByteSpan(data), 1u, [&](CHIP_ERROR e) { result = e; });
    EXPECT_EQ(result, CHIP_ERROR_INCORRECT_STATE);
}

TEST(ControllerProxy, SetAdapterConnectsAndForwards)
{
    StubTransport transport;
    ControllerProxy proxy(nullptr);

    CHIP_ERROR connectResult = CHIP_ERROR_INTERNAL;
    proxy.SetAdapter(&transport, [&](CHIP_ERROR e) { connectResult = e; });

    EXPECT_EQ(connectResult, CHIP_NO_ERROR);
    EXPECT_EQ(transport.mConnectCallCount, 1);

    const uint8_t data[] = { 0x01, 0x02, 0x03 };
    CHIP_ERROR sendResult = CHIP_ERROR_INTERNAL;
    proxy.SendMessage(chip::ByteSpan(data), 99u, [&](CHIP_ERROR e) { sendResult = e; });

    EXPECT_EQ(sendResult, CHIP_NO_ERROR);
    EXPECT_EQ(transport.mSendCallCount, 1);
    EXPECT_EQ(transport.mLastSessionId, 99u);
    ASSERT_EQ(transport.mLastPayload.size(), sizeof(data));
    EXPECT_EQ(transport.mLastPayload[0], 0x01u);
}

TEST(ControllerProxy, SetAdapterDisconnectsPriorAdapter)
{
    StubTransport first;
    StubTransport second;
    ControllerProxy proxy(nullptr);

    proxy.SetAdapter(&first, nullptr);
    EXPECT_EQ(first.mConnectCallCount, 1);
    EXPECT_EQ(first.mDisconnectCallCount, 0);

    proxy.SetAdapter(&second, nullptr);
    EXPECT_EQ(first.mDisconnectCallCount, 1);
    EXPECT_EQ(second.mConnectCallCount, 1);
}

TEST(ControllerProxy, SetAdapterNullptrDisconnectsOnly)
{
    StubTransport transport;
    ControllerProxy proxy(nullptr);

    proxy.SetAdapter(&transport, nullptr);
    EXPECT_EQ(transport.mConnectCallCount, 1);

    CHIP_ERROR nullResult = CHIP_ERROR_INTERNAL;
    proxy.SetAdapter(nullptr, [&](CHIP_ERROR e) { nullResult = e; });
    EXPECT_EQ(transport.mDisconnectCallCount, 1);
    EXPECT_EQ(nullResult, CHIP_NO_ERROR);

    // After nullptr, SendMessage should fail with INCORRECT_STATE.
    CHIP_ERROR sendResult = CHIP_NO_ERROR;
    const uint8_t data[]  = { 0xFF };
    proxy.SendMessage(chip::ByteSpan(data), 1u, [&](CHIP_ERROR e) { sendResult = e; });
    EXPECT_EQ(sendResult, CHIP_ERROR_INCORRECT_STATE);
}

TEST(ControllerProxy, DestructorDisconnectsAdapter)
{
    StubTransport transport;
    {
        ControllerProxy proxy(nullptr);
        proxy.SetAdapter(&transport, nullptr);
        EXPECT_EQ(transport.mDisconnectCallCount, 0);
    }
    EXPECT_EQ(transport.mDisconnectCallCount, 1);
}

TEST(ControllerProxy, InboundMessageForwardedToHandler)
{
    StubTransport transport;
    bool called          = false;
    uint64_t gotSession  = 0;
    std::vector<uint8_t> gotPayload;

    ControllerProxy proxy([&](chip::ByteSpan payload, uint64_t sessionId) {
        called     = true;
        gotSession = sessionId;
        gotPayload.assign(payload.begin(), payload.end());
    });

    proxy.SetAdapter(&transport, nullptr);

    const uint8_t data[] = { 0xDE, 0xAD };
    transport.SimulateInbound(chip::ByteSpan(data), 77u);

    EXPECT_TRUE(called);
    EXPECT_EQ(gotSession, 77u);
    ASSERT_EQ(gotPayload.size(), 2u);
    EXPECT_EQ(gotPayload[0], 0xDEu);
}

TEST(ControllerProxy, AdapterReplacementUpdatesMessageHandler)
{
    StubTransport first;
    StubTransport second;
    int callCount = 0;

    ControllerProxy proxy([&](chip::ByteSpan, uint64_t) { callCount++; });

    proxy.SetAdapter(&first, nullptr);
    const uint8_t data[] = { 1 };
    first.SimulateInbound(chip::ByteSpan(data), 1u);
    EXPECT_EQ(callCount, 1);

    // Replace adapter: first is disconnected, second is connected with a fresh handler.
    proxy.SetAdapter(&second, nullptr);
    EXPECT_EQ(first.mDisconnectCallCount, 1);
    EXPECT_EQ(second.mConnectCallCount, 1);
    EXPECT_TRUE(second.mHandler); // new adapter has a handler installed

    second.SimulateInbound(chip::ByteSpan(data), 2u);
    EXPECT_EQ(callCount, 2);
}

} // namespace
