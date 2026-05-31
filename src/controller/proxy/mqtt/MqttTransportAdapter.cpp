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

#include <controller/proxy/mqtt/MqttTransportAdapter.h>

#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/LockTracker.h>

#include <cstdio>
#include <cstring>

namespace chip {
namespace Controller {
namespace Mqtt {

MqttTransportAdapter::MqttTransportAdapter(const char * socketPath, QosLevel qos) : mSocketPath(socketPath), mQos(qos) {}

MqttTransportAdapter::~MqttTransportAdapter()
{
    Disconnect();
}

void MqttTransportAdapter::SetMessageHandler(MessageHandler handler)
{
    assertChipStackLockedByCurrentThread();
    mMessageHandler = std::move(handler);
}

void MqttTransportAdapter::Connect(TransportCompletionCallback callback)
{
    assertChipStackLockedByCurrentThread();

    mIpcClient.SetFrameCallback([this](IpcMessageType type, chip::ByteSpan sessionId, chip::ByteSpan payload) {
        OnIpcFrame(type, sessionId, payload);
    });

    CHIP_ERROR err = mIpcClient.Open(mSocketPath, chip::DeviceLayer::SystemLayer());
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Controller, "MqttTransportAdapter: failed to open IPC socket: %" CHIP_ERROR_FORMAT, err.Format());
        if (callback)
        {
            callback(err);
        }
        return;
    }

    // Send a Connect frame to the external manager so it initiates the broker
    // connection.  The manager replies with a Status frame.
    uint8_t sessionId[kIpcSessionIdLen] = {};
    uint8_t qosByte                     = static_cast<uint8_t>(mQos);
    CHIP_ERROR writeErr = mIpcClient.WriteFrame(IpcMessageType::kConnect, chip::ByteSpan(sessionId), chip::ByteSpan(&qosByte, 1));
    if (writeErr != CHIP_NO_ERROR)
    {
        ChipLogError(Controller, "MqttTransportAdapter: Connect frame write failed: %" CHIP_ERROR_FORMAT, writeErr.Format());
        mIpcClient.Close();
        if (callback)
        {
            callback(writeErr);
        }
        return;
    }

    // Stash callback; resolved when Status frame arrives.
    mConnectCallback = std::move(callback);
}

void MqttTransportAdapter::Disconnect()
{
    assertChipStackLockedByCurrentThread();

    // Abort all pending Send callbacks.
    for (auto & cb : mPendingSends)
    {
        if (cb)
        {
            cb(CHIP_ERROR_CONNECTION_ABORTED);
        }
    }
    mPendingSends.clear();

    // Abort pending Connect callback if still outstanding.
    if (mConnectCallback)
    {
        auto cb      = std::move(mConnectCallback);
        mConnectCallback = nullptr;
        cb(CHIP_ERROR_CONNECTION_ABORTED);
    }

    mIpcClient.Close();
}

void MqttTransportAdapter::Send(chip::ByteSpan payload, uint64_t sessionId, TransportCompletionCallback callback)
{
    assertChipStackLockedByCurrentThread();

    if (!mIpcClient.IsOpen())
    {
        if (callback)
        {
            callback(CHIP_ERROR_INCORRECT_STATE);
        }
        return;
    }

    uint8_t sessionBytes[kIpcSessionIdLen];
    SessionIdToBytes(sessionId, sessionBytes);

    CHIP_ERROR err = mIpcClient.WriteFrame(IpcMessageType::kSend, chip::ByteSpan(sessionBytes), payload);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Controller, "MqttTransportAdapter: Send frame write failed: %" CHIP_ERROR_FORMAT, err.Format());
        if (callback)
        {
            callback(err);
        }
        return;
    }

    // Queue the callback; it will be resolved when the matching Status frame arrives.
    mPendingSends.push_back(std::move(callback));
}

void MqttTransportAdapter::OnIpcFrame(IpcMessageType type, chip::ByteSpan sessionId, chip::ByteSpan payload)
{
    switch (type)
    {
    case IpcMessageType::kStatus: {
        // A status frame may resolve either the Connect callback or the oldest
        // pending Send callback (they arrive in order).
        CHIP_ERROR result = CHIP_NO_ERROR;
        if (!payload.empty() && payload.data()[0] != 0x00)
        {
            result = CHIP_ERROR_INTERNAL;
        }

        if (mConnectCallback)
        {
            auto cb          = std::move(mConnectCallback);
            mConnectCallback = nullptr;
            cb(result);
        }
        else if (!mPendingSends.empty())
        {
            auto cb = std::move(mPendingSends.front());
            mPendingSends.erase(mPendingSends.begin());
            if (cb)
            {
                cb(result);
            }
        }
        break;
    }

    case IpcMessageType::kReceive: {
        if (mMessageHandler)
        {
            // Decode session ID back to uint64.
            uint64_t id = 0;
            if (sessionId.size() >= sizeof(uint64_t))
            {
                ::memcpy(&id, sessionId.data(), sizeof(uint64_t));
            }
            mMessageHandler(payload, id);
        }
        break;
    }

    default:
        ChipLogDetail(Controller, "MqttTransportAdapter: unexpected IPC frame type 0x%02x", static_cast<unsigned>(type));
        break;
    }
}

// static
void MqttTransportAdapter::SessionIdToBytes(uint64_t sessionId, uint8_t out[kIpcSessionIdLen])
{
    ::memset(out, 0, kIpcSessionIdLen);
    ::memcpy(out, &sessionId, sizeof(sessionId));
}

} // namespace Mqtt
} // namespace Controller
} // namespace chip
