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

#pragma once

#include <commands/common/CHIPCommand.h>

class JointFabricCommand : public CHIPCommand
{
public:
    JointFabricCommand(CredentialIssuerCommands * credIssuerCommands) : CHIPCommand("provision-jf-admin-app", credIssuerCommands)
    {
        AddArgument("jf-admin-app-node-id", 0, UINT64_MAX, &jfAdminAppNodeId,
                     "Node ID for the jf-admin-app. If not provided, default value from CHIPProjectAppConfig.h will be used.");
    }

    /////////// CHIPCommand Interface /////////
    CHIP_ERROR RunCommand() override { return RunCommand(jfAdminAppNodeId); }

    chip::System::Clock::Timeout GetWaitDuration() const override { return chip::System::Clock::Seconds16(1); }

private:
    chip::Optional<chip::NodeId> jfAdminAppNodeId;

    CHIP_ERROR RunCommand(chip::Optional<chip::NodeId> mNodeId);
};

class OnboardCommand : public Command
{
public:
    OnboardCommand() : Command("onboard", "Onboard other fabric admins.") {
        AddArgument("passcode", &passcode,
                    "The passcode of the fabric admin to onboard.");
    }

    CHIP_ERROR Run();

private:
    chip::CharSpan passcode;
};
