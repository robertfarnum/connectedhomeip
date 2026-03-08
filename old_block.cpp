
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
        }
    }
    else
    {
        // When ICD device commissioning fails, the ICDClientInfo stored in OnICDRegistrationComplete needs to be removed.
