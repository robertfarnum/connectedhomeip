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

/****************************************************************************
 * @file
 * @brief Implementation for the Joint Fabric Datastore Cluster
 ***************************************************************************/

#include <app-common/zap-generated/cluster-objects.h>
#include <app/AttributeAccessInterface.h>
#include <app/AttributeAccessInterfaceRegistry.h>
#include <app/CommandHandler.h>
#include <app/ConcreteCommandPath.h>
#include <app/EventLogging.h>
#include <app/data-model/Encode.h>
#include <app/reporting/reporting.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using chip::Protocols::InteractionModel::Status;

namespace JointFabricDatastoreCluster = chip::app::Clusters::JointFabricDatastore;

class JointFabricDatastoreAttrAccess : public AttributeAccessInterface
{
public:
    JointFabricDatastoreAttrAccess() : AttributeAccessInterface(Optional<EndpointId>::Missing(), JointFabricDatastoreCluster::Id) {}

    CHIP_ERROR Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder) override;

private:
    CHIP_ERROR ReadGroupKeyList(AttributeValueEncoder & aEncoder);
    CHIP_ERROR ReadAdminList(AttributeValueEncoder & aEncoder);
    CHIP_ERROR ReadNodeList(AttributeValueEncoder & aEncoder);
};

JointFabricDatastoreAttrAccess gJointFabricDatastoreAttrAccess;

CHIP_ERROR JointFabricDatastoreAttrAccess::Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder)
{
    VerifyOrDie(aPath.mClusterId == JointFabricDatastoreCluster::Id);

    switch (aPath.mAttributeId)
    {
    case JointFabricDatastoreCluster::Attributes::GroupKeyList::Id: {
        return ReadGroupKeyList(aEncoder);
    }
    case JointFabricDatastoreCluster::Attributes::AdminList::Id: {
        return ReadAdminList(aEncoder);
    }
    case JointFabricDatastoreCluster::Attributes::NodeList::Id: {
        return ReadNodeList(aEncoder);
    }
    // TODO: Others
    default:
        break;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR JointFabricDatastoreAttrAccess::ReadGroupKeyList(AttributeValueEncoder & aEncoder)
{
    auto groupKeySetList        = Server::GetInstance().GetJointFabricDatastorage().GetGroupKeySetList();
    size_t groupKeySetListCount = Server::GetInstance().GetJointFabricDatastorage().GetGroupKeySetListCount();

    return aEncoder.EncodeList([&](const auto & encoder) -> CHIP_ERROR {
        if (groupKeySetList != nullptr)
        {
            for (size_t groupKeySetListIndex = 0; groupKeySetListIndex < groupKeySetListCount; groupKeySetListIndex++)
            {
                auto & entry = groupKeySetList[groupKeySetListIndex];
                ReturnErrorOnFailure(encoder.Encode(entry));
            }
        }
        return CHIP_NO_ERROR;
    });
}

CHIP_ERROR JointFabricDatastoreAttrAccess::ReadAdminList(AttributeValueEncoder & aEncoder)
{
    auto adminEntries        = Server::GetInstance().GetJointFabricDatastorage().GetAdminEntries();
    size_t adminEntriesCount = Server::GetInstance().GetJointFabricDatastorage().GetAdminEntriesCount();

    return aEncoder.EncodeList([&](const auto & encoder) -> CHIP_ERROR {
        if (adminEntries != nullptr)
        {
            for (size_t adminEntriesIndex = 0; adminEntriesIndex < adminEntriesCount; adminEntriesIndex++)
            {
                auto & entry = adminEntries[adminEntriesIndex];
                ReturnErrorOnFailure(encoder.Encode(entry));
            }
        }
        return CHIP_NO_ERROR;
    });
}

CHIP_ERROR JointFabricDatastoreAttrAccess::ReadNodeList(AttributeValueEncoder & aEncoder)
{
    auto nodeInformationEntries        = Server::GetInstance().GetJointFabricDatastorage().GetNodeInformationEntries();
    size_t nodeInformationEntriesCount = Server::GetInstance().GetJointFabricDatastorage().GetNodeInformationEntriesCount();

    return aEncoder.EncodeList([&](const auto & encoder) -> CHIP_ERROR {
        if (nodeInformationEntries != nullptr)
        {
            for (size_t nodeInformationEntriesIndex = 0; nodeInformationEntriesIndex < nodeInformationEntriesCount;
                 nodeInformationEntriesIndex++)
            {
                auto & entry = nodeInformationEntries[nodeInformationEntriesIndex];
                ReturnErrorOnFailure(encoder.Encode(entry));
            }
        }
        return CHIP_NO_ERROR;
    });
}

bool emberAfJointFabricDatastoreClusterAddAdminCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::AddAdmin::DecodableType & commandData)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    JointFabricDatastoreCluster::Structs::DatastoreAdministratorInformationEntry::DecodableType adminInformationEntry =
        commandData.adminInformationEntry.Value();
    JointFabricDatastorage & JointFabricDatastorage = Server::GetInstance().GetJointFabricDatastorage();

    SuccessOrExit(err = JointFabricDatastorage.AddAdmin(adminInformationEntry));

exit:
    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterAddGroupCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::AddGroup::DecodableType & commandData)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddKeySetCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::AddKeySet::DecodableType & commandData)
{
    CHIP_ERROR err                                                                     = CHIP_NO_ERROR;
    JointFabricDatastoreCluster::Structs::GroupKeySetStruct::DecodableType groupKeySet = commandData.groupKeySet;
    JointFabricDatastorage & JointFabricDatastorage = Server::GetInstance().GetJointFabricDatastorage();

    VerifyOrExit(JointFabricDatastorage.IsGroupKeySetEntryPresent(groupKeySet.groupKeySetID) == false,
                 err = CHIP_ERROR_INVALID_ARGUMENT);
    SuccessOrExit(err = JointFabricDatastorage.AddGroupKeySetEntry(groupKeySet));

exit:
    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RemoveNode::DecodableType & commandData)
{
    NodeId nodeId = commandData.nodeID;

    if (commandPath.mEndpointId != kRootEndpointId)
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: invalid endpoint in RemoveNode request");
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::InvalidCommand);
        return true;
    }

    CHIP_ERROR err = Server::GetInstance().GetJointFabricDatastorage().RemoveNode(nodeId);

    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

bool emberAfJointFabricDatastoreClusterUpdateNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::UpdateNode::DecodableType & commandData)
{
    NodeId nodeId                 = commandData.nodeID;
    const CharSpan & friendlyName = commandData.friendlyName;

    if (commandPath.mEndpointId != kRootEndpointId)
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: invalid endpoint in UpdateNode request");
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::InvalidCommand);
        return true;
    }

    CHIP_ERROR err = Server::GetInstance().GetJointFabricDatastorage().UpdateNode(nodeId, friendlyName);

    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

bool emberAfJointFabricDatastoreClusterRefreshNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RefreshNode::DecodableType & commandData)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    NodeId nodeId  = commandData.nodeID;

    JointFabricDatastorage & JointFabricDatastorage = Server::GetInstance().GetJointFabricDatastorage();

    if (commandPath.mEndpointId != kRootEndpointId)
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: invalid endpoint in RefreshNode request");
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::InvalidCommand);
        return true;
    }

    // 1. && 2.
    SuccessOrExit(err = JointFabricDatastorage.SetNode(nodeId, Clusters::JointFabricDatastore::DatastoreStateEnum::kPending));

    // 3. TODO: Read the PartsList of the Descriptor cluster from the Node.

    // 4.
    SuccessOrExit(err = JointFabricDatastorage.RefreshGroupKeySet(nodeId));

    // 5.
    SuccessOrExit(err = JointFabricDatastorage.RefreshACLList(nodeId));

    // 6.
    SuccessOrExit(err = JointFabricDatastorage.SetNode(nodeId, Clusters::JointFabricDatastore::DatastoreStateEnum::kCommitted));

exit:
    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveAdminCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RemoveAdmin::DecodableType & commandData)
{
    CHIP_ERROR err                                  = CHIP_NO_ERROR;
    auto nodeId                                     = commandData.nodeID;
    JointFabricDatastorage & JointFabricDatastorage = Server::GetInstance().GetJointFabricDatastorage();

    SuccessOrExit(err = JointFabricDatastorage.RemoveAdmin(nodeId));

exit:
    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterRemoveGroupCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RemoveGroup::DecodableType & commandData)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterUpdateAdminCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::UpdateAdmin::DecodableType & commandData)
{
    CHIP_ERROR err                                  = CHIP_NO_ERROR;
    auto nodeId                                     = commandData.nodeID;
    auto friendlyName                               = commandData.friendlyName;
    auto icac                                       = commandData.icac;
    JointFabricDatastorage & JointFabricDatastorage = Server::GetInstance().GetJointFabricDatastorage();

    SuccessOrExit(err = JointFabricDatastorage.UpdateAdmin(nodeId, friendlyName, icac));

exit:
    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterUpdateGroupCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::UpdateGroup::DecodableType & commandData)
{
    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterAddACLToNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::AddACLToNode::DecodableType & commandData)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveKeySetCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RemoveKeySet::DecodableType & commandData)
{
    CHIP_ERROR err                                  = CHIP_NO_ERROR;
    uint16_t groupKeySetId                          = commandData.groupKeySetId;
    JointFabricDatastorage & JointFabricDatastorage = Server::GetInstance().GetJointFabricDatastorage();

    SuccessOrExit(err = JointFabricDatastorage.RemoveGroupKeySetEntry(groupKeySetId));

exit:
    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterUpdateKeySetCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::UpdateKeySet::DecodableType & commandData)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddPendingNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::AddPendingNode::DecodableType & commandData)
{
    NodeId nodeId                 = commandData.nodeID;
    const CharSpan & friendlyName = commandData.friendlyName;

    if (commandPath.mEndpointId != kRootEndpointId)
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: invalid endpoint in AddPendingNode request");
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::InvalidCommand);
        return true;
    }

    CHIP_ERROR err = Server::GetInstance().GetJointFabricDatastorage().AddPendingNode(commandObj->GetAccessingFabricIndex(), nodeId,
                                                                                      friendlyName);

    if (err == CHIP_NO_ERROR)
    {
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::Status::Success);
    }
    else
    {
        ChipLogError(DataManagement, "JointFabricDatastoreCluster: failed with error: %" CHIP_ERROR_FORMAT, err.Format());
        commandObj->AddStatus(commandPath, Protocols::InteractionModel::ClusterStatusCode(err));
    }

    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterRemoveACLFromNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RemoveACLFromNode::DecodableType & commandData)
{
    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterUpdateEndpointForNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::UpdateEndpointForNode::DecodableType & commandData)
{
    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterAddBindingToEndpointForNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::AddBindingToEndpointForNode::DecodableType & commandData)
{
    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterAddGroupIDToEndpointForNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::AddGroupIDToEndpointForNode::DecodableType & commandData)
{
    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterRemoveBindingFromEndpointForNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RemoveBindingFromEndpointForNode::DecodableType & commandData)
{
    return true;
}

// TODO
bool emberAfJointFabricDatastoreClusterRemoveGroupIDFromEndpointForNodeCallback(
    CommandHandler * commandObj, const ConcreteCommandPath & commandPath,
    const JointFabricDatastoreCluster::Commands::RemoveGroupIDFromEndpointForNode::DecodableType & commandData)
{
    return true;
}

void MatterJointFabricDatastorePluginServerInitCallback()
{
    ChipLogProgress(Zcl, "Initiating Joint Fabric Datastore cluster.");
    AttributeAccessInterfaceRegistry::Instance().Register(&gJointFabricDatastoreAttrAccess);
}
