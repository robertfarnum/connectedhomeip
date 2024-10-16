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

#include <app/server/JointFabricDatastorage.h>

namespace chip {
namespace app {

CHIP_ERROR JointFabricDatastorage::AddPendingNode(FabricIndex fabricId, NodeId nodeId, const CharSpan & friendlyName)
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

CHIP_ERROR JointFabricDatastorage::UpdateNode(NodeId nodeId, const CharSpan & friendlyName)
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

CHIP_ERROR JointFabricDatastorage::RemoveNode(NodeId nodeId)
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

CHIP_ERROR JointFabricDatastorage::SetNode(NodeId nodeId, Clusters::JointFabricDatastore::DatastoreStateEnum state)
{
    size_t index = 0;
    ReturnErrorOnFailure(IsNodeIDInDatastore(nodeId, index));
    mNodeInformationEntries[index].commissioningStatusEntry.state = state;
    return CHIP_NO_ERROR;
}

CHIP_ERROR JointFabricDatastorage::IsNodeIDInDatastore(NodeId nodeId, size_t & index)
{
    for (size_t i = 0; i < mNodeInformationEntriesCount; ++i)
    {
        if (mNodeInformationEntries[i].nodeID == nodeId)
        {
            index = i;
            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

CHIP_ERROR JointFabricDatastorage::RefreshGroupKeySet(NodeId nodeId)
{
    size_t index = 0;
    ReturnErrorOnFailure(IsNodeIDInDatastore(nodeId, index));

    for (auto & it : mNodeInformationEntries[index].nodeKeySetList)
    {
        if (it.statusEntry.state == Clusters::JointFabricDatastore::DatastoreStateEnum::kPending)
        {
            ReturnErrorOnFailure(AddGroupKeySetEntry(it.groupKeySetId));
            const_cast<Clusters::JointFabricDatastore::Structs::DatastoreNodeKeyEntry::Type &>(it).statusEntry.state =
                Clusters::JointFabricDatastore::DatastoreStateEnum::kCommitted;
        }
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR JointFabricDatastorage::AddGroupKeySetEntry(uint16_t groupKeySetId)
{
    VerifyOrReturnError(mGroupKeyKetSetListCount < kMaxNodes, CHIP_ERROR_NO_MEMORY);

    Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type & entry = mGroupKeyKetSetList[mGroupKeyKetSetListCount];
    entry.groupKeySetID                                                    = groupKeySetId;

    ++mGroupKeyKetSetListCount;

    return CHIP_NO_ERROR;
}

CHIP_ERROR JointFabricDatastorage::AddGroupKeySetEntry(Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type & groupKeySet)
{
    VerifyOrReturnError(IsGroupKeySetEntryPresent(groupKeySet.groupKeySetID) == false, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(mGroupKeyKetSetListCount < kMaxNodes, CHIP_ERROR_NO_MEMORY);

    mGroupKeyKetSetList[mGroupKeyKetSetListCount] = groupKeySet;
    ++mGroupKeyKetSetListCount;

    return CHIP_NO_ERROR;
}

bool JointFabricDatastorage::IsGroupKeySetEntryPresent(uint16_t groupKeySetId)
{
    for (size_t i = 0; i < mGroupKeyKetSetListCount; ++i)
    {
        if (mGroupKeyKetSetList[i].groupKeySetID == groupKeySetId)
        {
            return true;
        }
    }

    return false;
}

CHIP_ERROR JointFabricDatastorage::RemoveGroupKeySetEntry(uint16_t groupKeySetId)
{
    for (size_t i = 0; i < mGroupKeyKetSetListCount; ++i)
    {
        if (mGroupKeyKetSetList[i].groupKeySetID == groupKeySetId)
        {
            mGroupKeyKetSetList[i] = mGroupKeyKetSetList[mGroupKeyKetSetListCount - 1];
            --mGroupKeyKetSetListCount;
            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

CHIP_ERROR
JointFabricDatastorage::UpdateGroupKeySetEntry(Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type & groupKeySet)
{
    for (size_t i = 0; i < mGroupKeyKetSetListCount; ++i)
    {
        if (mGroupKeyKetSetList[i].groupKeySetID == groupKeySet.groupKeySetID)
        {
            bool field_updated     = memcmp(&mGroupKeyKetSetList[i], &groupKeySet,
                                            sizeof(Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type)) == 0;
            mGroupKeyKetSetList[i] = groupKeySet;

            if (field_updated)
            {
                RefreshNodes(groupKeySet);
            }

            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

CHIP_ERROR JointFabricDatastorage::RefreshNodes(Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type & groupKeySet)
{
    for (size_t i = 0; i < mNodeInformationEntriesCount; ++i)
    {
        for (auto & it : mNodeInformationEntries[i].nodeKeySetList)
        {
            if (it.groupKeySetId == groupKeySet.groupKeySetID)
            {
                const_cast<Clusters::JointFabricDatastore::Structs::DatastoreNodeKeyEntry::Type &>(it).statusEntry.state =
                    Clusters::JointFabricDatastore::DatastoreStateEnum::kPending;

                // TODO: Update the node with the new group key set

                const_cast<Clusters::JointFabricDatastore::Structs::DatastoreNodeKeyEntry::Type &>(it).statusEntry.state =
                    Clusters::JointFabricDatastore::DatastoreStateEnum::kCommitted;
            }
        }
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR
JointFabricDatastorage::AddAdmin(Clusters::JointFabricDatastore::Structs::DatastoreAdministratorInformationEntry::Type & adminId)
{
    VerifyOrReturnError(IsAdminEntryPresent(adminId.nodeID) == false, CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(mAdminEntriesCount < kMaxNodes, CHIP_ERROR_NO_MEMORY);

    mAdminEntries[mAdminEntriesCount] = adminId;
    ++mAdminEntriesCount;

    return CHIP_NO_ERROR;
}

bool JointFabricDatastorage::IsAdminEntryPresent(NodeId nodeId)
{
    for (size_t i = 0; i < mAdminEntriesCount; ++i)
    {
        if (mAdminEntries[i].nodeID == nodeId)
        {
            return true;
        }
    }

    return false;
}

CHIP_ERROR JointFabricDatastorage::UpdateAdmin(NodeId nodeId, CharSpan friendlyName, ByteSpan icac)
{
    for (size_t i = 0; i < mAdminEntriesCount; ++i)
    {
        if (mAdminEntries[i].nodeID == nodeId)
        {
            mAdminEntries[i].friendlyName = friendlyName;
            mAdminEntries[i].icac         = icac;
            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

CHIP_ERROR JointFabricDatastorage::RemoveAdmin(NodeId nodeId)
{
    for (size_t i = 0; i < mAdminEntriesCount; ++i)
    {
        if (mAdminEntries[i].nodeID == nodeId)
        {
            mAdminEntries[i] = mAdminEntries[mAdminEntriesCount - 1];
            --mAdminEntriesCount;
            return CHIP_NO_ERROR;
        }
    }

    return CHIP_ERROR_KEY_NOT_FOUND;
}

CHIP_ERROR
JointFabricDatastorage::RefreshACLList(NodeId nodeId)
{
    size_t index = 0;
    ReturnErrorOnFailure(IsNodeIDInDatastore(nodeId, index));

    for (auto & it : mNodeInformationEntries[index].ACLList)
    {
        if (it.statusEntry.state == Clusters::JointFabricDatastore::DatastoreStateEnum::kPending)
        {
            // TODO: Add it.aclEntry to the ACL list
            const_cast<Clusters::JointFabricDatastore::Structs::DatastoreACLEntry::Type &>(it).statusEntry.state =
                Clusters::JointFabricDatastore::DatastoreStateEnum::kCommitted;
        }
    }

    return CHIP_NO_ERROR;
}

} // namespace app
} // namespace chip
