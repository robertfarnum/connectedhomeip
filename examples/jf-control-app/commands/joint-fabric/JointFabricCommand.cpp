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
#include <lib/support/ScopedBuffer.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>
#include <thread>
#include <unistd.h>

using namespace ::chip;

namespace
{
}

CHIP_ERROR JointFabricCommand::RunCommand(chip::Optional<chip::NodeId> nodeId)
{
    fprintf(stderr,
            "Provision jf-admin-app\n");

    CHIP_ERROR err;

    chip::Crypto::P256Keypair ephemeralKey;

    chip::Platform::ScopedMemoryBuffer<uint8_t> jfAdminNoc;
    chip::Platform::ScopedMemoryBuffer<uint8_t> jfAdminIcac;
    chip::Platform::ScopedMemoryBuffer<uint8_t> jfAdminRcac;
    MutableByteSpan jfAdminNocSpan;
    MutableByteSpan jfAdminIcacSpan;
    MutableByteSpan jfAdminRcacSpan;

    NodeId jfAdminNodeId = JF_ADMIN_APP_NODE_ID;

#if JF_GENERATE_CERTS_FOR_ANCHOR
    /* Administrator CAT */
    CASEAuthTag adminCAT = 0xFFFF'0001;

    /* Anchor/Datastore CAT */
    CASEAuthTag AnchorDatastoreCAT = 0xFFFC'0001;
#endif

    err = chip::Platform::MemoryInit();
    VerifyOrExit(err == CHIP_NO_ERROR, ChipLogError(Controller, "Init Memory failure: %s", chip::ErrorStr(err)));

    err = ephemeralKey.Initialize(chip::Crypto::ECPKeyTarget::ECDSA);
    SuccessOrExit(err);

    VerifyOrExit(jfAdminNoc.Alloc(chip::Controller::kMaxCHIPDERCertLength), err = CHIP_ERROR_NO_MEMORY);
    jfAdminNocSpan = MutableByteSpan(jfAdminNoc.Get(), Controller::kMaxCHIPDERCertLength);

    VerifyOrExit(jfAdminIcac.Alloc(chip::Controller::kMaxCHIPDERCertLength), err = CHIP_ERROR_NO_MEMORY);
    jfAdminIcacSpan = MutableByteSpan(jfAdminIcac.Get(), Controller::kMaxCHIPDERCertLength);

    VerifyOrExit(jfAdminRcac.Alloc(chip::Controller::kMaxCHIPDERCertLength), err = CHIP_ERROR_NO_MEMORY);
    jfAdminRcacSpan = MutableByteSpan(jfAdminRcac.Get(), Controller::kMaxCHIPDERCertLength);

    if (nodeId.HasValue())
    {
        jfAdminNodeId = nodeId.Value();
    }

    err = mCredIssuerCmds->GenerateControllerNOCChain(
          jfAdminNodeId, /* fabricId = */ 1, { { adminCAT, AnchorDatastoreCAT } },
          ephemeralKey, jfAdminRcacSpan, jfAdminIcacSpan, jfAdminNocSpan);

exit:
    chip::Platform::MemoryShutdown();
    ephemeralKey.Clear();

    ChipLogProgress(Controller, "jf-control-app initialization status: %s", chip::ErrorStr(err));
    return err;
}
