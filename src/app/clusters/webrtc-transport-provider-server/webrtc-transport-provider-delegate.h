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

#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>

namespace chip {
namespace app {
namespace Clusters {
namespace WebRTCTransportProvider {

/**
 * @brief Session configuration parameters
 */
struct SessionConfiguration
{
    // Supported media types - bit flags that can be combined
    static constexpr uint16_t kMediaTypeVideo = 0x01;
    static constexpr uint16_t kMediaTypeAudio = 0x02;
    static constexpr uint16_t kMediaTypeData = 0x04;

    // Video codec types
    static constexpr uint8_t kVideoCodecH264 = 0;
    static constexpr uint8_t kVideoCodecH265 = 1;
    static constexpr uint8_t kVideoCodecVP8 = 2;
    static constexpr uint8_t kVideoCodecVP9 = 3;
    static constexpr uint8_t kVideoCodecAV1 = 4;

    // Audio codec types
    static constexpr uint8_t kAudioCodecOpus = 0;
    static constexpr uint8_t kAudioCodecAAC = 1;
    static constexpr uint8_t kAudioCodecPCM = 2;

    // Quality preset
    static constexpr uint8_t kQualityPresetLow = 0;
    static constexpr uint8_t kQualityPresetMedium = 1;
    static constexpr uint8_t kQualityPresetHigh = 2;
    static constexpr uint8_t kQualityPresetCustom = 3;

    // Session info
    uint16_t mediaTypes;       // Bitwise combination of media types
    uint8_t videoCodec;        // Video codec type if video is enabled
    uint8_t audioCodec;        // Audio codec type if audio is enabled
    uint8_t qualityPreset;     // Quality preset or custom
    uint16_t maxWidth;         // Maximum video width (if custom quality)
    uint16_t maxHeight;        // Maximum video height (if custom quality)
    uint16_t maxFrameRate;     // Maximum video frame rate (if custom quality)
    uint32_t maxBitrate;       // Maximum bitrate in kbps (if custom quality)
    bool enableDtls;           // Enable DTLS for encryption
};

/**
 * @brief Delegate for WebRTC Transport Provider cluster
 * 
 * This delegate interface provides the platform-specific functionality needed
 * for the WebRTC Transport Provider cluster. Implementers should provide
 * concrete implementations for the methods defined here.
 */
class Delegate
{
public:
    virtual ~Delegate() = default;

    /**
     * @brief Create a new WebRTC session
     * 
     * @param config The session configuration
     * @param[out] sessionId The assigned session ID on success
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR CreateSession(const SessionConfiguration & config, uint32_t & sessionId) = 0;

    /**
     * @brief Terminate an existing WebRTC session
     * 
     * @param sessionId The session ID to terminate
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR TerminateSession(uint32_t sessionId) = 0;

    /**
     * @brief Process an SDP offer from a requestor
     * 
     * @param sessionId The session ID
     * @param sdpOffer The SDP offer
     * @param[out] sdpAnswer The generated SDP answer
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR ProcessSdpOffer(uint32_t sessionId, const CharSpan & sdpOffer, MutableCharSpan & sdpAnswer) = 0;

    /**
     * @brief Create an SDP offer for initiating a session
     * 
     * @param sessionId The session ID
     * @param[out] sdpOffer The generated SDP offer
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR CreateSdpOffer(uint32_t sessionId, MutableCharSpan & sdpOffer) = 0;

    /**
     * @brief Process an SDP answer from a requestor
     * 
     * @param sessionId The session ID
     * @param sdpAnswer The SDP answer
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR ProcessSdpAnswer(uint32_t sessionId, const CharSpan & sdpAnswer) = 0;

    /**
     * @brief Process an ICE candidate from a requestor
     * 
     * @param sessionId The session ID
     * @param iceCandidate The ICE candidate
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR ProcessIceCandidate(uint32_t sessionId, const CharSpan & iceCandidate) = 0;

    /**
     * @brief Get the current list of active sessions
     * 
     * @param[out] sessionIds Buffer to store session IDs
     * @param[in,out] sessionCount In: buffer size, Out: number of sessions
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR GetActiveSessions(MutableSpan<uint32_t> sessionIds, size_t & sessionCount) = 0;

    /**
     * @brief Get the session state
     * 
     * @param sessionId The session ID
     * @param[out] state The current state
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR GetSessionState(uint32_t sessionId, uint8_t & state) = 0;

    /**
     * @brief Get session configuration
     * 
     * @param sessionId The session ID
     * @param[out] config The session configuration
     * @return CHIP_ERROR Error code
     */
    virtual CHIP_ERROR GetSessionConfiguration(uint32_t sessionId, SessionConfiguration & config) = 0;
};

} // namespace WebRTCTransportProvider
} // namespace Clusters
} // namespace app
} // namespace chip