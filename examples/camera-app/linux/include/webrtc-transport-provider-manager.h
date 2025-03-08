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

#include <app/clusters/webrtc-transport-provider-server/webrtc-transport-provider-delegate.h>
#include <lib/core/CHIPError.h>
#include <platform/CHIPDeviceLayer.h>
#include <map>
#include <mutex>
#include <string>

namespace chip {
namespace app {

/**
 * @brief WebRTC Transport Provider Manager for Camera App
 * 
 * This class implements the WebRTC Transport Provider delegate interface
 * for the camera application, providing WebRTC functionality for streaming
 * camera video and audio.
 */
class WebRTCTransportProviderManager : public Clusters::WebRTCTransportProvider::Delegate
{
public:
    WebRTCTransportProviderManager();
    ~WebRTCTransportProviderManager() override;

    // Delegate implementations
    CHIP_ERROR CreateSession(const Clusters::WebRTCTransportProvider::SessionConfiguration & config,
                             uint32_t & sessionId) override;
    CHIP_ERROR TerminateSession(uint32_t sessionId) override;
    CHIP_ERROR ProcessSdpOffer(uint32_t sessionId, const CharSpan & sdpOffer, MutableCharSpan & sdpAnswer) override;
    CHIP_ERROR CreateSdpOffer(uint32_t sessionId, MutableCharSpan & sdpOffer) override;
    CHIP_ERROR ProcessSdpAnswer(uint32_t sessionId, const CharSpan & sdpAnswer) override;
    CHIP_ERROR ProcessIceCandidate(uint32_t sessionId, const CharSpan & iceCandidate) override;
    CHIP_ERROR GetActiveSessions(MutableSpan<uint32_t> sessionIds, size_t & sessionCount) override;
    CHIP_ERROR GetSessionState(uint32_t sessionId, uint8_t & state) override;
    CHIP_ERROR GetSessionConfiguration(uint32_t sessionId, Clusters::WebRTCTransportProvider::SessionConfiguration & config) override;

private:
    // Session information structure
    struct SessionInfo
    {
        Clusters::WebRTCTransportProvider::SessionConfiguration config;
        uint8_t state;
        std::string sdpOffer;
        std::string sdpAnswer;
        std::vector<std::string> iceCandidates;
    };

    // Sessions storage
    std::mutex mSessionsMutex;
    std::map<uint32_t, SessionInfo> mSessions;
    uint32_t mNextSessionId;

    // Helper methods
    uint32_t GenerateSessionId();
    bool IsSessionValid(uint32_t sessionId);
};

} // namespace app
} // namespace chip