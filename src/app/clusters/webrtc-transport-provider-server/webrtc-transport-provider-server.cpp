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

#include "webrtc-transport-provider-server.h"

#include <app/AttributeAccessInterface.h>
#include <app/CommandHandler.h>
#include <app/CommandHandlerInterface.h>
#include <app/ConcreteCommandPath.h>
#include <app/EventLogging.h>
#include <app/util/attribute-storage.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/Span.h>
#include <lib/support/logging/CHIPLogging.h>

#include <limits>

// Generated ZAP headers
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::WebRTCTransportProvider;

namespace {
constexpr size_t kMaxSessionIds = 16;
constexpr size_t kMaxSdpLength = 4096;
constexpr size_t kMaxIceCandidateLength = 512;

// Command IDs from the Matter WebRTC Transport Provider cluster spec
constexpr CommandId kCommandIdCreateSession = 0x00;
constexpr CommandId kCommandIdTerminateSession = 0x01;
constexpr CommandId kCommandIdSdpOffer = 0x02;
constexpr CommandId kCommandIdSdpAnswer = 0x03;
constexpr CommandId kCommandIdIceCandidate = 0x04;

// Attribute IDs from the Matter WebRTC Transport Provider cluster spec
constexpr AttributeId kAttributeIdSupportedMediaTypes = 0x00;
constexpr AttributeId kAttributeIdSupportedVideoCodecs = 0x01;
constexpr AttributeId kAttributeIdSupportedAudioCodecs = 0x02;
constexpr AttributeId kAttributeIdActiveSessions = 0x03;
constexpr AttributeId kAttributeIdMaxConcurrentSessions = 0x04;
constexpr AttributeId kAttributeIdSessionState = 0x05;
constexpr AttributeId kAttributeIdSessionMediaConfig = 0x06;

// Event IDs from the Matter WebRTC Transport Provider cluster spec
constexpr EventId kEventIdSessionStateChanged = 0x00;
constexpr EventId kEventIdIceCandidateReceived = 0x01;
} // namespace

// Static instance
Server Server::sInstance;

Server & Server::Instance()
{
    return sInstance;
}

Server::Server()
{
    // Initialize server state
}

void Server::RegisterAttributeAccessInterface(EndpointId endpointId)
{
    registerAttributeAccessOverride(&Instance(), endpointId, Clusters::WebRTCTransportProvider::Id);
}

void Server::InvokeCommand(HandlerContext & ctx)
{
    auto commandId = ctx.mRequestPath.mCommandId;
    
    switch (commandId)
    {
    case kCommandIdCreateSession:
        HandleCreateSession(ctx);
        break;
    case kCommandIdTerminateSession:
        HandleTerminateSession(ctx);
        break;
    case kCommandIdSdpOffer:
        HandleSdpOffer(ctx);
        break;
    case kCommandIdSdpAnswer:
        HandleSdpAnswer(ctx);
        break;
    case kCommandIdIceCandidate:
        HandleIceCandidate(ctx);
        break;
    default:
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::UnsupportedCommand);
        break;
    }
}

CHIP_ERROR Server::Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder)
{
    VerifyOrReturnError(mDelegate != nullptr, CHIP_ERROR_INCORRECT_STATE);
    
    switch (aPath.mAttributeId)
    {
    case kAttributeIdSupportedMediaTypes:
    {
        // Return supported media types as bit flags (video, audio, data)
        uint16_t supportedMediaTypes = SessionConfiguration::kMediaTypeVideo | 
                                      SessionConfiguration::kMediaTypeAudio | 
                                      SessionConfiguration::kMediaTypeData;
        return aEncoder.Encode(supportedMediaTypes);
    }
    
    case kAttributeIdSupportedVideoCodecs:
    {
        // Return supported video codecs
        Clusters::WebRTCTransportProvider::Structs::SupportedVideoCodecs::Type codecsStruct;
        codecsStruct.h264 = true;
        codecsStruct.h265 = true;
        codecsStruct.vp8 = true;
        codecsStruct.vp9 = true;
        codecsStruct.av1 = false; // Example: device doesn't support AV1
        return aEncoder.Encode(codecsStruct);
    }
    
    case kAttributeIdSupportedAudioCodecs:
    {
        // Return supported audio codecs
        Clusters::WebRTCTransportProvider::Structs::SupportedAudioCodecs::Type codecsStruct;
        codecsStruct.opus = true;
        codecsStruct.aac = true;
        codecsStruct.pcm = true;
        return aEncoder.Encode(codecsStruct);
    }
    
    case kAttributeIdActiveSessions:
    {
        // Return list of active sessions
        uint32_t sessionIds[kMaxSessionIds];
        size_t sessionCount = kMaxSessionIds;
        CHIP_ERROR err = mDelegate->GetActiveSessions(MutableSpan<uint32_t>(sessionIds, kMaxSessionIds), sessionCount);
        if (err != CHIP_NO_ERROR)
        {
            return err;
        }
        
        return aEncoder.EncodeList([&sessionIds, sessionCount](const auto & encoder) -> CHIP_ERROR {
            for (size_t i = 0; i < sessionCount; i++)
            {
                ReturnErrorOnFailure(encoder.Encode(sessionIds[i]));
            }
            return CHIP_NO_ERROR;
        });
    }
    
    case kAttributeIdMaxConcurrentSessions:
    {
        // Return maximum number of concurrent sessions
        uint16_t maxSessions = 4; // Example: 4 concurrent sessions max
        return aEncoder.Encode(maxSessions);
    }
    
    case kAttributeIdSessionState:
    {
        // Return the session state for a specific session
        uint32_t sessionId;
        ReturnErrorOnFailure(aEncoder.GetSubjectDescriptor().GetKeyValue(sessionId));
        
        uint8_t state;
        CHIP_ERROR err = mDelegate->GetSessionState(sessionId, state);
        if (err != CHIP_NO_ERROR)
        {
            return err;
        }
        
        return aEncoder.Encode(state);
    }
    
    case kAttributeIdSessionMediaConfig:
    {
        // Return the session media configuration for a specific session
        uint32_t sessionId;
        ReturnErrorOnFailure(aEncoder.GetSubjectDescriptor().GetKeyValue(sessionId));
        
        SessionConfiguration config;
        CHIP_ERROR err = mDelegate->GetSessionConfiguration(sessionId, config);
        if (err != CHIP_NO_ERROR)
        {
            return err;
        }
        
        Clusters::WebRTCTransportProvider::Structs::SessionMediaConfiguration::Type mediaConfig;
        mediaConfig.enableVideo = (config.mediaTypes & SessionConfiguration::kMediaTypeVideo) != 0;
        mediaConfig.enableAudio = (config.mediaTypes & SessionConfiguration::kMediaTypeAudio) != 0;
        mediaConfig.enableDataChannel = (config.mediaTypes & SessionConfiguration::kMediaTypeData) != 0;
        mediaConfig.videoCodec = config.videoCodec;
        mediaConfig.audioCodec = config.audioCodec;
        mediaConfig.qualityPreset = config.qualityPreset;
        mediaConfig.maxWidth = config.maxWidth;
        mediaConfig.maxHeight = config.maxHeight;
        mediaConfig.maxFrameRate = config.maxFrameRate;
        mediaConfig.maxBitrate = config.maxBitrate;
        
        return aEncoder.Encode(mediaConfig);
    }
    
    default:
        return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
    }
}

CHIP_ERROR Server::Write(const ConcreteDataAttributePath & aPath, AttributeValueDecoder & aDecoder)
{
    // All attributes are read-only in this cluster
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

void Server::HandleCreateSession(HandlerContext & ctx)
{
    VerifyOrExit(mDelegate != nullptr, ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure));
    
    Clusters::WebRTCTransportProvider::Commands::CreateSession::DecodableType requestData;
    
    if (CHIP_NO_ERROR != DataModel::Decode(ctx.mPayload, requestData))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::InvalidCommand);
        return;
    }
    
    // Convert request data to SessionConfiguration
    SessionConfiguration config;
    config.mediaTypes = 0;
    if (requestData.enableVideo.ValueOr(false))
        config.mediaTypes |= SessionConfiguration::kMediaTypeVideo;
    if (requestData.enableAudio.ValueOr(false))
        config.mediaTypes |= SessionConfiguration::kMediaTypeAudio;
    if (requestData.enableDataChannel.ValueOr(false))
        config.mediaTypes |= SessionConfiguration::kMediaTypeData;
    
    config.videoCodec = requestData.videoCodec.ValueOr(0);
    config.audioCodec = requestData.audioCodec.ValueOr(0);
    config.qualityPreset = requestData.qualityPreset.ValueOr(SessionConfiguration::kQualityPresetMedium);
    config.maxWidth = requestData.maxWidth.ValueOr(1280);
    config.maxHeight = requestData.maxHeight.ValueOr(720);
    config.maxFrameRate = requestData.maxFrameRate.ValueOr(30);
    config.maxBitrate = requestData.maxBitrate.ValueOr(2000);
    config.enableDtls = requestData.enableEncryption.ValueOr(true);
    
    // Create the session
    uint32_t sessionId;
    CHIP_ERROR err = mDelegate->CreateSession(config, sessionId);
    if (err != CHIP_NO_ERROR)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure);
        return;
    }
    
    // Send response
    Clusters::WebRTCTransportProvider::Commands::CreateSessionResponse::Type response;
    response.sessionId = sessionId;
    
    ctx.mCommandHandler.AddResponse(ctx.mRequestPath, response);
    
exit:
    return;
}

void Server::HandleTerminateSession(HandlerContext & ctx)
{
    VerifyOrExit(mDelegate != nullptr, ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure));
    
    Clusters::WebRTCTransportProvider::Commands::TerminateSession::DecodableType requestData;
    
    if (CHIP_NO_ERROR != DataModel::Decode(ctx.mPayload, requestData))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::InvalidCommand);
        return;
    }
    
    // Terminate the session
    CHIP_ERROR err = mDelegate->TerminateSession(requestData.sessionId);
    if (err != CHIP_NO_ERROR)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure);
        return;
    }
    
    // Send response
    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Success);
    
exit:
    return;
}

void Server::HandleSdpOffer(HandlerContext & ctx)
{
    VerifyOrExit(mDelegate != nullptr, ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure));
    
    Clusters::WebRTCTransportProvider::Commands::SdpOffer::DecodableType requestData;
    
    if (CHIP_NO_ERROR != DataModel::Decode(ctx.mPayload, requestData))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::InvalidCommand);
        return;
    }
    
    // Process SDP offer
    char sdpAnswerBuffer[kMaxSdpLength];
    MutableCharSpan sdpAnswer(sdpAnswerBuffer, kMaxSdpLength);
    
    CHIP_ERROR err = mDelegate->ProcessSdpOffer(requestData.sessionId, CharSpan(requestData.sdpOffer.data(), requestData.sdpOffer.size()), sdpAnswer);
    if (err != CHIP_NO_ERROR)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure);
        return;
    }
    
    // Send response
    Clusters::WebRTCTransportProvider::Commands::SdpAnswerResponse::Type response;
    response.sessionId = requestData.sessionId;
    response.sdpAnswer = sdpAnswer;
    
    ctx.mCommandHandler.AddResponse(ctx.mRequestPath, response);
    
exit:
    return;
}

void Server::HandleSdpAnswer(HandlerContext & ctx)
{
    VerifyOrExit(mDelegate != nullptr, ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure));
    
    Clusters::WebRTCTransportProvider::Commands::SdpAnswer::DecodableType requestData;
    
    if (CHIP_NO_ERROR != DataModel::Decode(ctx.mPayload, requestData))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::InvalidCommand);
        return;
    }
    
    // Process SDP answer
    CHIP_ERROR err = mDelegate->ProcessSdpAnswer(requestData.sessionId, CharSpan(requestData.sdpAnswer.data(), requestData.sdpAnswer.size()));
    if (err != CHIP_NO_ERROR)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure);
        return;
    }
    
    // Send response
    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Success);
    
exit:
    return;
}

void Server::HandleIceCandidate(HandlerContext & ctx)
{
    VerifyOrExit(mDelegate != nullptr, ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure));
    
    Clusters::WebRTCTransportProvider::Commands::IceCandidate::DecodableType requestData;
    
    if (CHIP_NO_ERROR != DataModel::Decode(ctx.mPayload, requestData))
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::InvalidCommand);
        return;
    }
    
    // Process ICE candidate
    CHIP_ERROR err = mDelegate->ProcessIceCandidate(requestData.sessionId, 
                             CharSpan(requestData.iceCandidate.data(), requestData.iceCandidate.size()));
    if (err != CHIP_NO_ERROR)
    {
        ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Failure);
        return;
    }
    
    // Send response
    ctx.mCommandHandler.AddStatus(ctx.mRequestPath, Protocols::InteractionModel::Status::Success);
    
exit:
    return;
}

CHIP_ERROR Server::HandleReceivedIceCandidate(uint32_t sessionId, const CharSpan & candidate)
{
    VerifyOrReturnError(mDelegate != nullptr, CHIP_ERROR_INCORRECT_STATE);
    
    // Create and log an IceCandidateReceived event
    Clusters::WebRTCTransportProvider::Events::IceCandidateReceived::Type event;
    event.sessionId = sessionId;
    event.iceCandidate = candidate;
    
    // Get all endpoints that implement this cluster
    for (auto endpoint : EnabledEndpointsWithServerCluster(Clusters::WebRTCTransportProvider::Id))
    {
        // Log the event for each endpoint
        EventNumber eventNumber;
        CHIP_ERROR err = LogEvent(event, endpoint, eventNumber);
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(Zcl, "WebRTCTransportProvider: Failed to log IceCandidateReceived event: %" CHIP_ERROR_FORMAT, err.Format());
        }
    }
    
    return CHIP_NO_ERROR;
}

CHIP_ERROR Server::AddIceCandidate(uint32_t sessionId, const CharSpan & candidate)
{
    VerifyOrReturnError(mDelegate != nullptr, CHIP_ERROR_INCORRECT_STATE);
    
    return mDelegate->ProcessIceCandidate(sessionId, candidate);
}

CHIP_ERROR Server::UpdateSessionState(uint32_t sessionId, uint8_t state)
{
    VerifyOrReturnError(mDelegate != nullptr, CHIP_ERROR_INCORRECT_STATE);
    
    // Create and log a SessionStateChanged event
    Clusters::WebRTCTransportProvider::Events::SessionStateChanged::Type event;
    event.sessionId = sessionId;
    event.state = state;
    
    // Get all endpoints that implement this cluster
    for (auto endpoint : EnabledEndpointsWithServerCluster(Clusters::WebRTCTransportProvider::Id))
    {
        // Log the event for each endpoint
        EventNumber eventNumber;
        CHIP_ERROR err = LogEvent(event, endpoint, eventNumber);
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(Zcl, "WebRTCTransportProvider: Failed to log SessionStateChanged event: %" CHIP_ERROR_FORMAT, err.Format());
        }
    }
    
    return CHIP_NO_ERROR;
}