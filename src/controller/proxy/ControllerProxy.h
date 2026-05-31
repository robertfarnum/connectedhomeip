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

#include <controller/proxy/TransportProtocol.h>
#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>

namespace chip {
namespace Controller {

/// Protocol-agnostic proxy between the Matter controller and a cloud transport.
///
/// ControllerProxy provides a clean, transport-neutral interface to the rest of
/// the controller stack.  Callers never reference MQTT, WebSocket, or any other
/// protocol — they simply send and receive opaque byte payloads through the
/// registered adapter.
///
/// Lifecycle:
///   1. Construct with an inbound message handler.
///   2. Call SetAdapter() to attach and connect a transport.
///   3. Call SendMessage() to send payloads.
///   4. The destructor automatically disconnects the active adapter.
///
/// Thread safety: All methods MUST be called on the chip stack task.
class ControllerProxy
{
public:
    /// Construct a proxy with the given inbound message handler.
    ///
    /// The handler is forwarded to whichever adapter is active.  It will be
    /// called on the chip stack task for every inbound message.
    ///
    /// @param inboundHandler  Callable for received messages.  May be empty, in
    ///                        which case inbound messages are silently discarded.
    explicit ControllerProxy(MessageHandler inboundHandler);

    /// Destructor.  Disconnects the active adapter synchronously if one is set.
    ~ControllerProxy();

    // Non-copyable, non-movable.
    ControllerProxy(const ControllerProxy &)             = delete;
    ControllerProxy & operator=(const ControllerProxy &) = delete;
    ControllerProxy(ControllerProxy &&)                  = delete;
    ControllerProxy & operator=(ControllerProxy &&)      = delete;

    /// Replace the active transport adapter.
    ///
    /// If an adapter is already set, it is disconnected before the new one is
    /// connected.  Passing nullptr disconnects any existing adapter and leaves
    /// the proxy without a transport (subsequent SendMessage calls will fail
    /// with CHIP_ERROR_INCORRECT_STATE).
    ///
    /// @param adapter   New adapter to use.  Not owned; caller must ensure it
    ///                  outlives the proxy (or until the next SetAdapter call).
    /// @param callback  Called with the result of the new adapter's Connect().
    ///                  Called synchronously with CHIP_NO_ERROR when adapter is
    ///                  nullptr.
    void SetAdapter(TransportProtocol * adapter, TransportCompletionCallback callback);

    /// Send a message via the active transport adapter.
    ///
    /// If no adapter is registered (or SetAdapter was called with nullptr),
    /// callback is called with CHIP_ERROR_INCORRECT_STATE.
    ///
    /// @param payload    Bytes to send.
    /// @param sessionId  Opaque 64-bit session identifier passed to the adapter.
    /// @param callback   Called with the transport's send result.
    void SendMessage(chip::ByteSpan payload, uint64_t sessionId, TransportCompletionCallback callback);

private:
    TransportProtocol * mAdapter = nullptr;
    MessageHandler      mInboundHandler;
};

} // namespace Controller
} // namespace chip
