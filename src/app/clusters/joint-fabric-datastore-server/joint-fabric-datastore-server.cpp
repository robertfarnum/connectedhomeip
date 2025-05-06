/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
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

bool emberAfJointFabricAdministratorClusterTransferAnchorRequestCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricAdministrator::Commands::TransferAnchorRequest::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricAdministratorClusterTransferAnchorCompleteCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricAdministrator::Commands::TransferAnchorComplete::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddKeySetCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::AddKeySet::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterUpdateKeySetCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::UpdateKeySet::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveKeySetCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RemoveKeySet::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddGroupCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::AddGroup::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterUpdateGroupCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::UpdateGroup::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveGroupCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RemoveGroup::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddAdminCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::AddAdmin::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterUpdateAdminCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::UpdateAdmin::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveAdminCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RemoveAdmin::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddPendingNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::AddPendingNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRefreshNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RefreshNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterUpdateNodeCallback(chip::app::CommandHandler*, chip::app::ConcreteCommandPath const&, chip::app::Clusters::JointFabricDatastore::Commands::UpdateNode::DecodableType const&)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RemoveNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterUpdateEndpointForNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::UpdateEndpointForNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddGroupIDToEndpointForNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::AddGroupIDToEndpointForNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveGroupIDFromEndpointForNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RemoveGroupIDFromEndpointForNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterAddBindingToEndpointForNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::AddBindingToEndpointForNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveBindingFromEndpointForNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RemoveBindingFromEndpointForNode::DecodableType const& c)
{
    return true;
}


bool emberAfJointFabricDatastoreClusterAddACLToNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::AddACLToNode::DecodableType const& c)
{
    return true;
}

bool emberAfJointFabricDatastoreClusterRemoveACLFromNodeCallback(chip::app::CommandHandler* a, chip::app::ConcreteCommandPath const& b, chip::app::Clusters::JointFabricDatastore::Commands::RemoveACLFromNode::DecodableType const& c)
{
    return true;
}
 
void MatterJointFabricDatastorePluginServerInitCallback()
{
    ChipLogProgress(Zcl, "Initiating Joint Fabric Datastore cluster.");
}
