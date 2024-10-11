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

#include <app/server/JointFabricDatastore.h>

namespace chip {
namespace app {

CHIP_ERROR JointFabricDatastore::AddPendingNode(FabricIndex fabricId, NodeId nodeId, const CharSpan & friendlyName)
{
    VerifyOrReturnError(mNodeInformationEntriesCount < kMaxNodes, CHIP_ERROR_NO_MEMORY);

    Clusters::JointFabricDatastore::Structs::DatastoreNodeInformationEntry::Type & entry =
        mNodeInformationEntries[mNodeInformationEntriesCount];
    entry.nodeID                         = nodeId;
    entry.friendlyName                   = friendlyName;
    entry.fabricIndex                    = fabricId;
    entry.commissioningStatusEntry.state = Clusters::JointFabricDatastore::DatastoreStateEnum::kPending;

    ++mNodeInformationEntriesCount;

    return CHIP_NO_ERROR;
}

CHIP_ERROR JointFabricDatastore::UpdateNode(NodeId nodeId, const CharSpan & friendlyName)
{
    for (size_t i = 0; i < mNodeInformationEntriesCount; ++i)
    {
        if (mNodeInformationEntries[i].nodeID == nodeId)
        {
            mNodeInformationEntries[i].friendlyName = friendlyName;
            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

CHIP_ERROR JointFabricDatastore::RemoveNode(NodeId nodeId)
{
    for (size_t i = 0; i < mNodeInformationEntriesCount; ++i)
    {
        if (mNodeInformationEntries[i].nodeID == nodeId)
        {
            mNodeInformationEntries[i] = mNodeInformationEntries[mNodeInformationEntriesCount - 1];
            --mNodeInformationEntriesCount;
            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

} // namespace app
} // namespace chip
