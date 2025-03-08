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

#include <app/AttributeAccessInterface.h>
#include <app/CommandHandlerInterface.h>
#include <app/util/af-types.h>
#include <app/util/basic-types.h>
#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>

#include "webrtc-transport-provider-delegate.h"

namespace chip {
namespace app {
namespace Clusters {
namespace WebRTCTransportProvider {

/**
 * @brief WebRTC Transport Provider Cluster Implementation
 * 
 * The WebRTC Transport Provider cluster is used for devices that provide WebRTC streams
 * such as cameras. It handles the WebRTC session establishment, SDP exchange, and 
 * ICE candidate negotiation according to the Matter specification.
 */
class Server : public CommandHandlerInterface,
               public AttributeAccessInterface
{
public:
    /**
     * @brief Get the Server Instance object (singleton)
     * 
     * @return Reference to the server instance
     */
    static Server & Instance();

    /**
     * @brief Set the delegate for platform-specific functionality
     * 
     * @param delegate Pointer to the delegate implementation
     */
    void SetDelegate(Delegate * delegate) { mDelegate = delegate; }

    /**
     * @brief Get the current delegate
     * 
     * @return Pointer to the current delegate
     */
    Delegate * GetDelegate() const { return mDelegate; }

    // CommandHandlerInterface
    void InvokeCommand(HandlerContext & handlerContext) override;

    // AttributeAccessInterface
    CHIP_ERROR Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder) override;
    CHIP_ERROR Write(const ConcreteDataAttributePath & aPath, AttributeValueDecoder & aDecoder) override;

    /**
     * @brief Register the attribute access interface
     * 
     * @param endpointId The endpoint ID to register
     */
    static void RegisterAttributeAccessInterface(EndpointId endpointId);

    /**
     * @brief Handle ICE candidate received from a peer
     * 
     * @param sessionId The session ID
     * @param candidate The ICE candidate
     * @return CHIP_ERROR Error code
     */
    CHIP_ERROR HandleReceivedIceCandidate(uint32_t sessionId, const CharSpan & candidate);

    /**
     * @brief Add a new ICE candidate
     * 
     * @param sessionId The session ID
     * @param candidate The ICE candidate
     * @return CHIP_ERROR Error code
     */
    CHIP_ERROR AddIceCandidate(uint32_t sessionId, const CharSpan & candidate);

    /**
     * @brief Update the session state
     * 
     * @param sessionId The session ID
     * @param state The new state
     * @return CHIP_ERROR Error code
     */
    CHIP_ERROR UpdateSessionState(uint32_t sessionId, uint8_t state);

private:
    friend Server & Instance();

    static Server sInstance;

    // Private constructor (singleton)
    Server();

    // Delegate for platform-specific functionality
    Delegate * mDelegate = nullptr;

    // Command handlers
    void HandleCreateSession(HandlerContext & ctx);
    void HandleTerminateSession(HandlerContext & ctx);
    void HandleSdpOffer(HandlerContext & ctx);
    void HandleSdpAnswer(HandlerContext & ctx);
    void HandleIceCandidate(HandlerContext & ctx);

    // Session state constants
    static constexpr uint8_t kSessionStateIdle = 0;
    static constexpr uint8_t kSessionStateCreating = 1;
    static constexpr uint8_t kSessionStateNegotiating = 2;
    static constexpr uint8_t kSessionStateConnected = 3;
    static constexpr uint8_t kSessionStateDisconnected = 4;
    static constexpr uint8_t kSessionStateError = 5;
};

} // namespace WebRTCTransportProvider
} // namespace Clusters
} // namespace app
} // namespace chip