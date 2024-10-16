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

#include <app-common/zap-generated/cluster-objects.h>
#include <lib/core/CHIPPersistentStorageDelegate.h>

namespace chip {
namespace app {

class JointFabricDatastorage
{
public:
    static JointFabricDatastorage & GetInstance()
    {
        static JointFabricDatastorage sInstance;
        return sInstance;
    }

    CHIP_ERROR AddPendingNode(FabricIndex fabricId, NodeId nodeId, const CharSpan & friendlyName);
    CHIP_ERROR UpdateNode(NodeId nodeId, const CharSpan & friendlyName);
    CHIP_ERROR RemoveNode(NodeId nodeId);

    CHIP_ERROR SetNode(NodeId nodeId, Clusters::JointFabricDatastore::DatastoreStateEnum state);
    CHIP_ERROR RefreshGroupKeySet(NodeId nodeId);
    CHIP_ERROR RefreshACLList(NodeId nodeId);

    CHIP_ERROR AddGroupKeySetEntry(uint16_t groupKeySetId);
    CHIP_ERROR AddGroupKeySetEntry(Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type & groupKeySet);
    bool IsGroupKeySetEntryPresent(uint16_t groupKeySetId);
    CHIP_ERROR RemoveGroupKeySetEntry(uint16_t groupKeySetId);
    CHIP_ERROR UpdateGroupKeySetEntry(Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type & groupKeySet);

    CHIP_ERROR RefreshNodes(Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type & groupKeySet);

    CHIP_ERROR AddAdmin(Clusters::JointFabricDatastore::Structs::DatastoreAdministratorInformationEntry::Type & adminId);
    bool IsAdminEntryPresent(NodeId nodeId);
    CHIP_ERROR UpdateAdmin(NodeId nodeId, CharSpan friendlyName, ByteSpan icac);
    CHIP_ERROR RemoveAdmin(NodeId nodeId);

    Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type * GetGroupKeySetList() { return mGroupKeyKetSetList; }
    size_t GetGroupKeySetListCount() { return mGroupKeyKetSetListCount; }

    Clusters::JointFabricDatastore::Structs::DatastoreNodeInformationEntry::Type * GetNodeInformationEntries()
    {
        return mNodeInformationEntries;
    }
    size_t GetNodeInformationEntriesCount() { return mNodeInformationEntriesCount; }

    Clusters::JointFabricDatastore::Structs::DatastoreAdministratorInformationEntry::Type * GetAdminEntries()
    {
        return mAdminEntries;
    }
    size_t GetAdminEntriesCount() { return mAdminEntriesCount; }

private:
    static constexpr size_t kMaxNodes = 32;

    Clusters::JointFabricDatastore::Structs::DatastoreNodeInformationEntry::Type mNodeInformationEntries[kMaxNodes] = {};
    size_t mNodeInformationEntriesCount                                                                             = 0;
    Clusters::GroupKeyManagement::Structs::GroupKeySetStruct::Type mGroupKeyKetSetList[kMaxNodes]                   = {};
    size_t mGroupKeyKetSetListCount                                                                                 = 0;
    Clusters::JointFabricDatastore::Structs::DatastoreAdministratorInformationEntry::Type mAdminEntries[kMaxNodes]  = {};
    size_t mAdminEntriesCount                                                                                       = 0;

    CHIP_ERROR IsNodeIDInDatastore(NodeId nodeId, size_t & index);
};

} // namespace app
} // namespace chip
