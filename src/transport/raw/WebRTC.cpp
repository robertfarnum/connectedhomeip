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

#include <transport/raw/WebRTC.h>

#include <lib/core/CHIPError.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

#include <rtc/rtc.hpp>

namespace chip {
namespace Transport {

class WebRTC::Impl
{
public:
    Impl(WebRTC & parent, const char * peerConnectionConfig) : mParent(parent)
    {
        rtc::InitLogger(rtc::LogLevel::Warning);
        
        rtc::Configuration config;
        if (peerConnectionConfig == nullptr)
        {
            // Use default STUN server if none provided
            config.iceServers.emplace_back("stun:stun.l.google.com:19302");
        }
        else
        {
            // Use provided configuration (parsing would go here)
            config.iceServers.emplace_back(peerConnectionConfig);
        }
        
        // Create peer connection
        mPeerConnection = std::make_shared<rtc::PeerConnection>(config);
        SetupPeerConnection();
    }
    
    ~Impl()
    {
        mDataChannel.reset();
        mPeerConnection.reset();
    }
    
    void SetupPeerConnection()
    {
        // Set up callbacks
        mPeerConnection->onStateChange([](rtc::PeerConnection::State state) {
            ChipLogProgress(NotSpecified, "WebRTC: Peer connection state changed to %d", static_cast<int>(state));
        });
        
        mPeerConnection->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
            ChipLogProgress(NotSpecified, "WebRTC: Gathering state changed to %d", static_cast<int>(state));
            
            if (state == rtc::PeerConnection::GatheringState::Complete)
            {
                ChipLogProgress(NotSpecified, "WebRTC: ICE gathering complete");
                // ICE gathering is complete, local description is ready
            }
        });
        
        mPeerConnection->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dataChannel) {
            ChipLogProgress(NotSpecified, "WebRTC: Data channel received");
            mDataChannel = dataChannel;
            SetupDataChannel();
        });
    }
    
    void SetupDataChannel()
    {
        if (!mDataChannel)
            return;
            
        mDataChannel->onOpen([this]() {
            ChipLogProgress(NotSpecified, "WebRTC: Data channel open");
            mIsConnected = true;
        });
        
        mDataChannel->onClosed([this]() {
            ChipLogProgress(NotSpecified, "WebRTC: Data channel closed");
            mIsConnected = false;
        });
        
        mDataChannel->onMessage([this](std::variant<rtc::binary, std::string> message) {
            if (std::holds_alternative<rtc::binary>(message))
            {
                const auto & binary = std::get<rtc::binary>(message);
                mParent.HandleDataChannelMessage(binary.data(), binary.size());
            }
        });
    }
    
    CHIP_ERROR Init(const Transport::InitializeParams & params)
    {
        return CHIP_NO_ERROR;
    }
    
    CHIP_ERROR SendMessage(System::PacketBufferHandle && msgBuf)
    {
        VerifyOrReturnError(mIsConnected, CHIP_ERROR_INCORRECT_STATE);
        VerifyOrReturnError(mDataChannel, CHIP_ERROR_INCORRECT_STATE);
        
        if (!msgBuf.IsNull())
        {
            rtc::binary data(msgBuf->Start(), msgBuf->Start() + msgBuf->DataLength());
            mDataChannel->send(data);
            return CHIP_NO_ERROR;
        }
        
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    
    CHIP_ERROR Connect(const char * peerIp, uint16_t port)
    {
        // In WebRTC, actual connection is established through signaling
        // This would be handled through a signaling server in a real implementation
        
        // Create data channel for communication
        mDataChannel = mPeerConnection->createDataChannel("matter");
        SetupDataChannel();
        
        // Create offer
        mPeerConnection->setLocalDescription();
        
        return CHIP_NO_ERROR;
    }
    
    CHIP_ERROR Disconnect()
    {
        if (mDataChannel)
        {
            mDataChannel->close();
            mDataChannel.reset();
        }
        
        mIsConnected = false;
        return CHIP_NO_ERROR;
    }
    
    bool IsConnected() const
    {
        return mIsConnected;
    }
    
    CHIP_ERROR SetRemoteDescription(const char * sdp)
    {
        try
        {
            mPeerConnection->setRemoteDescription(sdp);
            return CHIP_NO_ERROR;
        }
        catch (const std::exception & e)
        {
            ChipLogError(NotSpecified, "WebRTC: Error setting remote description: %s", e.what());
            return CHIP_ERROR_INTERNAL;
        }
    }
    
    CHIP_ERROR GetLocalDescription(char * sdp, size_t maxLen)
    {
        try
        {
            std::string description = mPeerConnection->localDescription();
            if (description.size() >= maxLen)
            {
                return CHIP_ERROR_BUFFER_TOO_SMALL;
            }
            
            strncpy(sdp, description.c_str(), maxLen);
            return CHIP_NO_ERROR;
        }
        catch (const std::exception & e)
        {
            ChipLogError(NotSpecified, "WebRTC: Error getting local description: %s", e.what());
            return CHIP_ERROR_INTERNAL;
        }
    }
    
    CHIP_ERROR AddIceCandidate(const char * candidate)
    {
        try
        {
            mPeerConnection->addRemoteCandidate(rtc::Candidate(candidate, ""));
            return CHIP_NO_ERROR;
        }
        catch (const std::exception & e)
        {
            ChipLogError(NotSpecified, "WebRTC: Error adding ICE candidate: %s", e.what());
            return CHIP_ERROR_INTERNAL;
        }
    }
    
private:
    WebRTC & mParent;
    std::shared_ptr<rtc::PeerConnection> mPeerConnection;
    std::shared_ptr<rtc::DataChannel> mDataChannel;
    bool mIsConnected = false;
};

WebRTC::WebRTC(const char * peerConnectionConfig) : mImpl(std::make_unique<Impl>(*this, peerConnectionConfig))
{
}

WebRTC::~WebRTC() = default;

CHIP_ERROR WebRTC::Init(const Transport::InitializeParams & params)
{
    return mImpl->Init(params);
}

CHIP_ERROR WebRTC::SendMessage(const Transport::PeerAddress & address, System::PacketBufferHandle && msgBuf)
{
    return mImpl->SendMessage(std::move(msgBuf));
}

bool WebRTC::CanSendToPeer(const Transport::PeerAddress & address)
{
    return mImpl->IsConnected();
}

void WebRTC::HandleDataChannelMessage(const uint8_t * data, size_t len)
{
    if (mMessageReceivedCallback)
    {
        System::PacketBufferHandle buffer = System::PacketBufferHandle::NewWithData(data, len);
        if (buffer.IsNull())
        {
            ChipLogError(NotSpecified, "WebRTC: Failed to allocate buffer for received message");
            return;
        }
        
        Transport::PeerAddress peerAddress; // In WebRTC, we don't have IP-based addressing
        mMessageReceivedCallback(peerAddress, std::move(buffer));
    }
}

CHIP_ERROR WebRTC::Connect(const char * peerIp, uint16_t port)
{
    return mImpl->Connect(peerIp, port);
}

CHIP_ERROR WebRTC::Disconnect()
{
    return mImpl->Disconnect();
}

bool WebRTC::IsConnected() const
{
    return mImpl->IsConnected();
}

CHIP_ERROR WebRTC::SetRemoteDescription(const char * sdp)
{
    return mImpl->SetRemoteDescription(sdp);
}

CHIP_ERROR WebRTC::GetLocalDescription(char * sdp, size_t maxLen)
{
    return mImpl->GetLocalDescription(sdp, maxLen);
}

CHIP_ERROR WebRTC::AddIceCandidate(const char * candidate)
{
    return mImpl->AddIceCandidate(candidate);
}

} // namespace Transport
} // namespace chip