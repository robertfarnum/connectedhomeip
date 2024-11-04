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

#include "commands/common/Commands.h"

#include <iostream>
#include <string>
#include <vector>

#include "RpcClient.h"
#include "RpcServer.h"
#include "commands/clusters/SubscriptionsCommands.h"
#include "commands/icd/ICDCommand.h"
#include "commands/interactive/Commands.h"
#include "commands/joint-fabric/Commands.h"
#include "commands/pairing/Commands.h"
#include "commands/rpc/Commands.h"
#include <device_manager/DeviceManager.h>
#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
// #include "GrpcServer.h"
#include "control_server/SocketServer.h"
#endif /* CONFIG_ENABLE_GRPC */

#include <zap-generated/cluster/Commands.h>

void ApplicationInit()
{
    DeviceMgr().Init();
}

// ================================================================================
// Main Code
// ================================================================================
int main(int argc, char * argv[])
{
    // Convert command line arguments to a vector of strings for easier manipulation
    std::vector<std::string> args(argv, argv + argc);
    int i = 0;

    /* Check for special command line options */
    while (i < argc)
    {
        auto arg = args[(unsigned) i];
        if (arg.compare("--enable-grpc") == 0)
        {
#if defined(CONFIG_ENABLE_GRPC) && CONFIG_ENABLE_GRPC
            // StartGrpcServer();
            SocketServer::sInstance.start();
#endif /* CONFIG_ENABLE_GRPC */
            /* Remove this option from the argument list so that it is not
             * propagated further to the command processing engine. */
            args.erase(args.begin() + i);
            argc--;
            continue;
        }
        i++;
    }

    // Check if "interactive" and "start" are not in the arguments
    if (args.size() < 3 || args[1] != "interactive" || args[2] != "start")
    {
        // Insert "interactive" and "start" after the executable name
        args.insert(args.begin() + 1, "interactive");
        args.insert(args.begin() + 2, "start");
    }

    ExampleCredentialIssuerCommands credIssuerCommands;
    Commands commands;

    registerCommandsJointFabric(commands, &credIssuerCommands);
    registerCommandsICD(commands, &credIssuerCommands);
    registerCommandsInteractive(commands, &credIssuerCommands);
    registerCommandsPairing(commands, &credIssuerCommands);
    registerClusters(commands, &credIssuerCommands);
    registerCommandsSubscriptions(commands, &credIssuerCommands);
    registerCommandsRpc(commands);

    RpcServerStart();

    std::vector<char *> c_args;
    for (auto & arg : args)
    {
        c_args.push_back(const_cast<char *>(arg.c_str()));
    }

    ApplicationInit();

    return commands.Run(static_cast<int>(c_args.size()), c_args.data());
}
