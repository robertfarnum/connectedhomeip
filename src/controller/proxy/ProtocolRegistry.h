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

/// Holds a fixed-size span of registered TransportProtocol instances and
/// allows lookup by an opaque 8-bit protocol identifier.
///
/// Allocation-free and lock-free.  The caller must ensure the span outlives
/// the registry.
///
/// Thread safety: All methods MUST be called on the chip stack task.
class ProtocolRegistry
{
public:
    /// Construct a registry backed by the given span of adapter pointers.
    ///
    /// @param adapters  Non-owning span of TransportProtocol pointers.
    ///                  Entries may be null; Lookup skips null slots.
    explicit ProtocolRegistry(chip::Span<TransportProtocol *> adapters) : mAdapters(adapters) {}

    /// Look up the adapter at the given index.
    ///
    /// @param index  Zero-based index into the adapter span passed at construction.
    /// @return       Pointer to the adapter, or nullptr if index is out-of-range
    ///               or the slot is null.
    TransportProtocol * Lookup(uint8_t index) const
    {
        if (index >= mAdapters.size())
        {
            return nullptr;
        }
        return mAdapters[index];
    }

    /// Return the number of slots in the registry (including null entries).
    size_t Size() const { return mAdapters.size(); }

private:
    chip::Span<TransportProtocol *> mAdapters;
};

} // namespace Controller
} // namespace chip
