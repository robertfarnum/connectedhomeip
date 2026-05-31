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

#pragma once

#include <cstdint>
#include <functional>

#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>

namespace chip {
namespace Controller {

/// Completion callback invoked when an async transport operation finishes.
/// Called on the chip stack task with the operation result.
using TransportCompletionCallback = std::function<void(CHIP_ERROR)>;

/// Inbound message handler registered with the transport.
/// Invoked on the chip stack task for every message received from the remote.
///
/// @param payload   Byte span over the received payload (valid only during the call).
/// @param sessionId Opaque 64-bit session identifier from the transport.
using MessageHandler = std::function<void(chip::ByteSpan payload, uint64_t sessionId)>;

/// Abstract bidirectional transport protocol interface.
///
/// Implementations bridge the Matter controller to an external network
/// protocol (e.g. MQTT via IPC, WebSocket, AMQP).  The interface deliberately
/// contains no protocol-specific terminology — callers never need to know which
/// transport is active.
///
/// Thread safety: All methods MUST be called on the chip stack task.
/// Implementations SHOULD assert this via assertChipStackLockedByCurrentThread().
class TransportProtocol
{
public:
    virtual ~TransportProtocol() = default;

    /// Register the inbound message handler.
    ///
    /// Must be called before Connect().  Replaces any previously registered
    /// handler.  Passing a null-target std::function clears the handler.
    ///
    /// @param handler  Callable invoked for every received message.
    virtual void SetMessageHandler(MessageHandler handler) = 0;

    /// Establish the transport connection.
    ///
    /// @param callback  Called with CHIP_NO_ERROR on success, or an error code
    ///                  if the connection could not be established.
    virtual void Connect(TransportCompletionCallback callback) = 0;

    /// Tear down the transport connection.
    ///
    /// All in-flight Send callbacks are invoked with
    /// CHIP_ERROR_CONNECTION_ABORTED before this method returns.
    virtual void Disconnect() = 0;

    /// Send a payload to the remote for the given session.
    ///
    /// @param payload    Bytes to transmit.  Implementations must copy the data
    ///                   if they need it to outlive this call.
    /// @param sessionId  Opaque 64-bit session identifier.
    /// @param callback   Called when the transport has accepted (or rejected)
    ///                   the payload.
    virtual void Send(chip::ByteSpan payload, uint64_t sessionId, TransportCompletionCallback callback) = 0;
};

} // namespace Controller
} // namespace chip
