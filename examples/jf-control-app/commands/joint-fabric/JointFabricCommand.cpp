/*
 *   Copyright (c) 2024 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#include "JointFabricCommand.h"
#include <commands/common/RemoteDataModelLogger.h>
#include <commands/interactive/InteractiveCommands.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>
#include <thread>
#include <unistd.h>

using namespace ::chip;

CHIP_ERROR JointFabricCommand::RunCommand(chip::Optional<chip::NodeId> jfAdminAppNodeId)
{
    fprintf(stderr,
            "Provision jf-admin-app\n");

    return CHIP_NO_ERROR;
}
