import sys
with open('examples/jf-control-app/commands/pairing/PairingCommand.cpp', 'r') as f:
    text = f.read()

old_block = """        else
        {
            OwnershipContext request;
            Credentials::P256PublicKeySpan adminICACPKSpan;

            memset(&request, 0, sizeof(request));
            request.node_id                      = nodeId;
            request.jcm                          = mJCM.ValueOr(false);
            JCMDeviceCommissioner & commissioner = static_cast<JCMDeviceCommissioner &>(CurrentCommissioner());
            JCMTrustVerificationInfo & info      = commissioner.GetTrustVerificationInfo();

            /* extract and save the public key of the peer Admin ICAC */
            err = Credentials::ExtractPublicKeyFromChipCert(info.adminICAC.Span(), adminICACPKSpan);
            if (err != CHIP_NO_ERROR)
            {
                ChipLogError(Controller, "Joint Commissioning Method Error parsing adminICAC Public Key");
                SetCommandExitStatus(err);
                return;
            }

            memcpy(request.trustedIcacPublicKeyB.bytes, adminICACPKSpan.data(), adminICACPKSpan.size());
            request.trustedIcacPublicKeyB.size = Crypto::kP256_PublicKey_Length;

            request.peerAdminJFAdminClusterEndpointId = info.adminEndpointId;

            auto call = rpcClient.TransferOwnership(request, OnRPCTransferDone);
            if (!call.active())
            {
                // The RPC call was not sent. This could occur due to, for example, an invalid channel ID. Handle as an error.
                ChipLogError(JointFabric, "RPC: OwnershipTransfer Call Error");
                SetCommandExitStatus(CHIP_ERROR_SHUT_DOWN);
                return;
            }

            err = WaitForResponse(call);
            if (err != CHIP_NO_ERROR)
            {
                ChipLogError(JointFabric, "Joint Commissioning Method (nodeId=%ld) failed: RPC OwnershipTransfer Timeout Error",
                             nodeId);
            }
        }"""

new_block = """        else
        {
            StartJcmRequest request;
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
        }"""

if old_block in text:
    text = text.replace(old_block, new_block)
    with open('examples/jf-control-app/commands/pairing/PairingCommand.cpp', 'w') as f:
        f.write(text)
    print("Success")
else:
    print("Failed to find block")
