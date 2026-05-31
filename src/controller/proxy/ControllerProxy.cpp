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

#include <controller/proxy/ControllerProxy.h>

#include <lib/support/logging/CHIPLogging.h>
#include <platform/LockTracker.h>

namespace chip {
namespace Controller {

ControllerProxy::ControllerProxy(MessageHandler inboundHandler) : mInboundHandler(std::move(inboundHandler)) {}

ControllerProxy::~ControllerProxy()
{
    if (mAdapter != nullptr)
    {
        mAdapter->Disconnect();
        mAdapter = nullptr;
    }
}

void ControllerProxy::SetAdapter(TransportProtocol * adapter, TransportCompletionCallback callback)
{
    assertChipStackLockedByCurrentThread();

    if (mAdapter != nullptr)
    {
        mAdapter->Disconnect();
        mAdapter = nullptr;
    }

    if (adapter == nullptr)
    {
        if (callback)
        {
            callback(CHIP_NO_ERROR);
        }
        return;
    }

    mAdapter = adapter;
    mAdapter->SetMessageHandler([this](chip::ByteSpan payload, uint64_t sessionId) {
        if (mInboundHandler)
        {
            mInboundHandler(payload, sessionId);
        }
    });
    mAdapter->Connect(std::move(callback));
}

void ControllerProxy::SendMessage(chip::ByteSpan payload, uint64_t sessionId, TransportCompletionCallback callback)
{
    assertChipStackLockedByCurrentThread();

    if (mAdapter == nullptr)
    {
        if (callback)
        {
            callback(CHIP_ERROR_INCORRECT_STATE);
        }
        return;
    }

    mAdapter->Send(payload, sessionId, std::move(callback));
}

} // namespace Controller
} // namespace chip
