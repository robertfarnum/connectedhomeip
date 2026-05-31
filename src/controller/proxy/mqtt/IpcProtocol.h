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

namespace chip {
namespace Controller {
namespace Mqtt {

/// IPC frame message types (1 byte on the wire).
enum class IpcMessageType : uint8_t
{
    kConnect    = 0x01, ///< C→M: request broker connection
    kDisconnect = 0x02, ///< C→M: request broker disconnection
    kSend       = 0x03, ///< C→M: publish payload on session topic
    kReceive    = 0x04, ///< M→C: inbound message arrived on session topic
    kStatus     = 0x05, ///< M→C: response to Connect or Send
};

/// Length of the per-session identifier in the wire frame (bytes).
constexpr uint8_t kIpcSessionIdLen = 16;

/// Total IPC frame header size (bytes):
///   1 (msgtype) + 4 (payload_len LE) + kIpcSessionIdLen (session_id).
constexpr uint8_t kIpcHeaderLen = 1 + 4 + kIpcSessionIdLen;

/// Wire frame layout (all multi-byte integers are little-endian):
///
///  +-----------+-----------+-----------+-----------+-----------+
///  | msgtype   | payload_len (4B LE)               |           |
///  +-----------+-----------+-----------+-----------+           |
///  | session_id (kIpcSessionIdLen bytes)                       |
///  +-----------------------------------------------------------+
///  | payload (payload_len bytes, optional)                     |
///  +-----------------------------------------------------------+
///
/// The Status frame payload is a single byte: 0x00 = success, non-zero = error.

/// Maximum single-frame payload size (1 MiB).
constexpr uint32_t kIpcMaxPayloadLen = 1u * 1024u * 1024u;

} // namespace Mqtt
} // namespace Controller
} // namespace chip
