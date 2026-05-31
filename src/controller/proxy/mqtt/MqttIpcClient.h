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
#include <controller/proxy/mqtt/IpcProtocol.h>
#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>
#include <system/SocketEvents.h>
#include <system/SystemLayer.h>

#include <cstdint>
#include <vector>

namespace chip {
namespace Controller {
namespace Mqtt {

/// Async Unix-socket IPC client that runs on the chip stack task.
///
/// MqttIpcClient manages the low-level read/write loop over a Unix domain
/// socket towards an external MQTT connection manager.  It:
///   - Opens the socket synchronously on Connect().
///   - Registers a socket-watch with SystemLayer for async readable callbacks.
///   - Writes frames synchronously (non-blocking send with internal queue).
///   - Parses incoming frames and dispatches them to registered callbacks.
///
/// Thread safety: All methods MUST be called on the chip stack task.
class MqttIpcClient
{
public:
    /// Called when a complete IPC frame has been received.
    using FrameCallback = std::function<void(IpcMessageType type, chip::ByteSpan sessionId, chip::ByteSpan payload)>;

    MqttIpcClient();
    ~MqttIpcClient();

    // Non-copyable.
    MqttIpcClient(const MqttIpcClient &)             = delete;
    MqttIpcClient & operator=(const MqttIpcClient &) = delete;

    /// Open the Unix socket at the given path and begin watching for reads.
    ///
    /// @param socketPath  Null-terminated path to the Unix domain socket.
    /// @param layer       SystemLayer used to register socket-watch events.
    /// @return CHIP_NO_ERROR on success;
    ///         CHIP_ERROR_CONNECTION_ABORTED if the socket cannot be opened.
    CHIP_ERROR Open(const char * socketPath, chip::System::Layer & layer);

    /// Close the socket and stop watching.  Safe to call when not open.
    void Close();

    /// Returns true when the socket is open and the watch is active.
    bool IsOpen() const { return mFd >= 0; }

    /// Register the callback invoked for every fully-parsed inbound frame.
    void SetFrameCallback(FrameCallback callback) { mFrameCallback = std::move(callback); }

    /// Write a frame to the socket.
    ///
    /// @param type       Message type byte.
    /// @param sessionId  Exactly kIpcSessionIdLen bytes.
    /// @param payload    Payload bytes (may be empty).
    /// @return CHIP_NO_ERROR on success; error on write failure.
    CHIP_ERROR WriteFrame(IpcMessageType type, chip::ByteSpan sessionId, chip::ByteSpan payload);

private:
    /// Called by SystemLayer when the socket is readable.
    static void OnSocketReadable(chip::System::SocketEvents events, intptr_t data);
    void        HandleReadable();

    /// Parse bytes accumulated in mReadBuf.  Returns when no more complete
    /// frames are available.
    void DrainReadBuffer();

    int                              mFd = chip::System::kInvalidFd;
    chip::System::SocketWatchToken   mWatchToken = 0;
    chip::System::Layer *            mLayer = nullptr;
    FrameCallback            mFrameCallback;

    // Receive buffer — holds partial frame bytes across reads.
    std::vector<uint8_t> mReadBuf;
};

} // namespace Mqtt
} // namespace Controller
} // namespace chip
