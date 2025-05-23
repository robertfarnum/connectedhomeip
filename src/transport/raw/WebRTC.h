/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
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

#include <transport/raw/Base.h>
#include <transport/raw/MessageHeader.h>

#include <lib/core/CHIPError.h>
#include <lib/support/DLLUtil.h>
#include <system/SystemLayer.h>
#include <system/SystemPacketBuffer.h>

#include <memory>

namespace chip {
namespace Transport {

class DLL_EXPORT WebRTC : public Base
{
public:
    WebRTC(const WebRTC &) = delete;
    WebRTC & operator=(const WebRTC &) = delete;

    WebRTC(const char * peerConnectionConfig = nullptr);
    ~WebRTC() override;

    // Implementation of Transport::Base
    CHIP_ERROR Init(const Transport::InitializeParams & params) override;
    CHIP_ERROR SendMessage(const Transport::PeerAddress & address, System::PacketBufferHandle && msgBuf) override;
    bool CanSendToPeer(const Transport::PeerAddress & address) override;

    // Handle received data from WebRTC
    void HandleDataChannelMessage(const uint8_t * data, size_t len);

    // Connection management methods
    CHIP_ERROR Connect(const char * peerIp, uint16_t port);
    CHIP_ERROR Disconnect();
    bool IsConnected() const;

    // Methods to handle WebRTC signaling
    CHIP_ERROR SetRemoteDescription(const char * sdp);
    CHIP_ERROR GetLocalDescription(char * sdp, size_t maxLen);
    CHIP_ERROR AddIceCandidate(const char * candidate);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace Transport
} // namespace chip