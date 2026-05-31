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

#include <controller/proxy/mqtt/MqttIpcClient.h>

#include <lib/support/logging/CHIPLogging.h>

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace chip {
namespace Controller {
namespace Mqtt {

namespace {

/// Write `len` bytes from `buf` to `fd`, retrying on EINTR.
/// Returns the total bytes written, or -1 on error.
ssize_t WriteAll(int fd, const uint8_t * buf, size_t len)
{
    size_t remaining = len;
    while (remaining > 0)
    {
        ssize_t n = ::write(fd, buf, remaining);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        buf += static_cast<size_t>(n);
        remaining -= static_cast<size_t>(n);
    }
    return static_cast<ssize_t>(len);
}

/// Encode a 32-bit value as 4 bytes little-endian into dst.
void WriteLE32(uint8_t * dst, uint32_t value)
{
    dst[0] = static_cast<uint8_t>(value & 0xFF);
    dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    dst[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

/// Decode 4 bytes little-endian from src.
uint32_t ReadLE32(const uint8_t * src)
{
    return static_cast<uint32_t>(src[0]) | (static_cast<uint32_t>(src[1]) << 8) |
        (static_cast<uint32_t>(src[2]) << 16) | (static_cast<uint32_t>(src[3]) << 24);
}

} // namespace

MqttIpcClient::MqttIpcClient() : mFd(chip::System::kInvalidFd), mWatchToken(0) {}

MqttIpcClient::~MqttIpcClient()
{
    Close();
}

CHIP_ERROR MqttIpcClient::Open(const char * socketPath, chip::System::Layer & layer)
{
    VerifyOrReturnError(socketPath != nullptr, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(mFd < 0, CHIP_ERROR_INCORRECT_STATE);

    int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        ChipLogError(Controller, "MqttIpcClient: socket() failed: %s", strerror(errno));
        return CHIP_ERROR_CONNECTION_ABORTED;
    }

    struct sockaddr_un addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    int copied = ::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketPath);
    if (copied < 0 || static_cast<size_t>(copied) >= sizeof(addr.sun_path))
    {
        ::close(fd);
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    if (::connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        ChipLogError(Controller, "MqttIpcClient: connect(%s) failed: %s", socketPath, strerror(errno));
        ::close(fd);
        return CHIP_ERROR_CONNECTION_ABORTED;
    }

    chip::System::SocketWatchToken token;
    ReturnErrorOnFailure(layer.StartWatchingSocket(fd, &token));
    ReturnErrorOnFailure(layer.SetCallback(token, OnSocketReadable, reinterpret_cast<intptr_t>(this)));
    ReturnErrorOnFailure(layer.RequestCallbackOnPendingRead(token));

    mFd         = fd;
    mWatchToken = token;
    mLayer      = &layer;
    return CHIP_NO_ERROR;
}

void MqttIpcClient::Close()
{
    if (mFd < 0)
    {
        return;
    }

    if (mLayer != nullptr)
    {
        mLayer->StopWatchingSocket(&mWatchToken);
        mLayer = nullptr;
    }

    ::close(mFd);
    mFd = chip::System::kInvalidFd;
    mReadBuf.clear();
}

CHIP_ERROR MqttIpcClient::WriteFrame(IpcMessageType type, chip::ByteSpan sessionId, chip::ByteSpan payload)
{
    VerifyOrReturnError(mFd >= 0, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(sessionId.size() == kIpcSessionIdLen, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(payload.size() <= kIpcMaxPayloadLen, CHIP_ERROR_INVALID_ARGUMENT);

    uint8_t header[kIpcHeaderLen];
    header[0] = static_cast<uint8_t>(type);
    WriteLE32(&header[1], static_cast<uint32_t>(payload.size()));
    ::memcpy(&header[5], sessionId.data(), kIpcSessionIdLen);

    if (WriteAll(mFd, header, kIpcHeaderLen) < 0)
    {
        ChipLogError(Controller, "MqttIpcClient: write header failed: %s", strerror(errno));
        return CHIP_ERROR_CONNECTION_ABORTED;
    }

    if (!payload.empty())
    {
        if (WriteAll(mFd, payload.data(), payload.size()) < 0)
        {
            ChipLogError(Controller, "MqttIpcClient: write payload failed: %s", strerror(errno));
            return CHIP_ERROR_CONNECTION_ABORTED;
        }
    }

    return CHIP_NO_ERROR;
}

// static
void MqttIpcClient::OnSocketReadable(chip::System::SocketEvents events, intptr_t data)
{
    auto * self = reinterpret_cast<MqttIpcClient *>(data);
    if (events.Has(chip::System::SocketEventFlags::kRead))
    {
        self->HandleReadable();
    }
}

void MqttIpcClient::HandleReadable()
{
    // Read available bytes into the buffer.
    uint8_t chunk[4096];
    for (;;)
    {
        ssize_t n = ::read(mFd, chunk, sizeof(chunk));
        if (n > 0)
        {
            mReadBuf.insert(mReadBuf.end(), chunk, chunk + n);
        }
        else if (n == 0)
        {
            // Remote closed the connection.
            ChipLogProgress(Controller, "MqttIpcClient: IPC connection closed by peer");
            Close();
            return;
        }
        else
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // No more data available right now.
            }
            ChipLogError(Controller, "MqttIpcClient: read error: %s", strerror(errno));
            Close();
            return;
        }
    }

    DrainReadBuffer();
}

void MqttIpcClient::DrainReadBuffer()
{
    while (mReadBuf.size() >= kIpcHeaderLen)
    {
        const uint8_t * hdr        = mReadBuf.data();
        auto            msgType    = static_cast<IpcMessageType>(hdr[0]);
        uint32_t        payloadLen = ReadLE32(&hdr[1]);

        if (payloadLen > kIpcMaxPayloadLen)
        {
            ChipLogError(Controller, "MqttIpcClient: oversized frame %" PRIu32 " bytes, closing", payloadLen);
            Close();
            return;
        }

        size_t totalFrameLen = kIpcHeaderLen + static_cast<size_t>(payloadLen);
        if (mReadBuf.size() < totalFrameLen)
        {
            break; // Incomplete frame; wait for more data.
        }

        chip::ByteSpan sessionId(hdr + 5, kIpcSessionIdLen);
        chip::ByteSpan payload(hdr + kIpcHeaderLen, payloadLen);

        if (mFrameCallback)
        {
            mFrameCallback(msgType, sessionId, payload);
        }

        mReadBuf.erase(mReadBuf.begin(), mReadBuf.begin() + static_cast<ptrdiff_t>(totalFrameLen));
    }
}

} // namespace Mqtt
} // namespace Controller
} // namespace chip
