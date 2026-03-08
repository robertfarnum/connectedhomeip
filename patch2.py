import sys

with open('examples/jf-control-app/commands/pairing/PairingCommand.cpp', 'r') as f:
    text = f.read()

import re

new_text = re.sub(
    r'OwnershipContext request;.*?(?=        \}\n    \}\n    else)',
    r"""StartJcmRequest request;
            memset(&request, 0, sizeof(request));
            request.node_id = nodeId;

            auto call = rpcClient.StartJcm(request, OnRPCTransferDone);
            if (!call.active())
            {
                // The RPC call was not sent. This could occur due to, for example, an invalid channel ID. Handle as an error.
                ChipLogError(JointFabric, "RPC: StartJcm Call Error");
                SetCommandExitStatus(CHIP_ERROR_SHUT_DOWN);
                return;
            }

            err = WaitForResponse(call);
            if (err != CHIP_NO_ERROR)
            {
                ChipLogError(JointFabric, "Joint Commissioning Method (nodeId=%ld) failed: RPC StartJcm Timeout Error",
                             nodeId);
            }
""",
    text,
    flags=re.DOTALL
)

if new_text != text:
    with open('examples/jf-control-app/commands/pairing/PairingCommand.cpp', 'w') as f:
        f.write(new_text)
    print("Success")
else:
    print("Failed")
