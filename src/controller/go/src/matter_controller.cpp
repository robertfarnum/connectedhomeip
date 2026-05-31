#include "matter_controller.h"

#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

#include <app/CommandSender.h>
#include <app/ReadClient.h>
#include <controller/CHIPDeviceController.h>
#include <controller/CHIPDeviceControllerFactory.h>
#include <controller/CommissioningDelegate.h>
#include <controller/ExampleOperationalCredentialsIssuer.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/GroupDataProviderImpl.h>
#include <credentials/attestation_verifier/DefaultDeviceAttestationVerifier.h>
#include <credentials/attestation_verifier/DeviceAttestationVerifier.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <crypto/PersistentStorageOperationalKeystore.h>
#include <crypto/RawKeySessionKeystore.h>
#include <credentials/PersistentStorageOpCertStore.h>
#include <data-model-providers/codegen/Instance.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/ScopedMemoryBuffer.h>
#include <lib/support/TestPersistentStorageDelegate.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/PlatformManager.h>
#include <lib/support/TestGroupData.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

using namespace chip;
using namespace chip::Controller;

namespace {

// Commissioning completion delegate — signals when pairing/commissioning finishes
class CommissioningCompleteDelegate : public DevicePairingDelegate {
public:
    void Reset() {
        std::lock_guard<std::mutex> lock(mu_);
        done_ = false;
        error_ = CHIP_NO_ERROR;
        node_id_ = kUndefinedNodeId;
    }

    bool WaitForComplete(uint32_t timeout_ms) {
        std::unique_lock<std::mutex> lock(mu_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]{ return done_; });
    }

    CHIP_ERROR GetError() const { return error_; }
    NodeId GetNodeId() const { return node_id_; }

    void OnPairingComplete(CHIP_ERROR error) override {
        if (error != CHIP_NO_ERROR) {
            std::lock_guard<std::mutex> lock(mu_);
            error_ = error;
            done_ = true;
            cv_.notify_all();
        }
    }

    void OnCommissioningComplete(NodeId nodeId, CHIP_ERROR error) override {
        std::lock_guard<std::mutex> lock(mu_);
        node_id_ = nodeId;
        error_ = error;
        done_ = true;
        cv_.notify_all();
    }

private:
    std::mutex mu_;
    std::condition_variable cv_;
    bool done_ = false;
    CHIP_ERROR error_ = CHIP_NO_ERROR;
    NodeId node_id_ = kUndefinedNodeId;
};

// Internal controller state
struct MatterControllerState {
    std::mutex mu;
    bool initialized = false;
    bool shutdown_called = false;

    DeviceCommissioner commissioner;
    CommissioningCompleteDelegate pairing_delegate;
    ExampleOperationalCredentialsIssuer op_creds_issuer;
    TestPersistentStorageDelegate storage;
    PersistentStorageOperationalKeystore operational_keystore;
    Credentials::PersistentStorageOpCertStore op_cert_store;
    Crypto::RawKeySessionKeystore session_keystore;
    Credentials::GroupDataProviderImpl group_data_provider;
    std::string storage_path;
    FabricId fabric_id = 0;
};

char* strdup_safe(const char* s) {
    if (!s) return nullptr;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

} // namespace

extern "C" {

matter_controller_handle_t matter_controller_init(
    const char* storage_path,
    uint64_t fabric_id,
    matter_result_t* result
) {
    if (!storage_path || !result) {
        if (result) {
            result->code = -1;
            result->message = strdup_safe("invalid parameters");
        }
        return nullptr;
    }

    result->code = 0;
    result->message = nullptr;

    // Initialize CHIP platform memory
    CHIP_ERROR err = Platform::MemoryInit();
    if (err != CHIP_NO_ERROR) {
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe("failed to initialize CHIP memory");
        return nullptr;
    }

    auto state = new MatterControllerState();
    state->storage_path = storage_path;
    state->fabric_id = static_cast<FabricId>(fabric_id);

    // Initialize persistent storage components
    err = state->operational_keystore.Init(&state->storage);
    if (err != CHIP_NO_ERROR) {
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe("failed to initialize operational keystore");
        delete state;
        Platform::MemoryShutdown();
        return nullptr;
    }

    err = state->op_cert_store.Init(&state->storage);
    if (err != CHIP_NO_ERROR) {
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe("failed to initialize op cert store");
        delete state;
        Platform::MemoryShutdown();
        return nullptr;
    }

    // Initialize factory (this also initializes the CHIP platform stack)
    FactoryInitParams factory_params;
    factory_params.fabricIndependentStorage = &state->storage;
    factory_params.operationalKeystore = &state->operational_keystore;
    factory_params.opCertStore = &state->op_cert_store;
    factory_params.sessionKeystore = &state->session_keystore;
    factory_params.dataModelProvider = chip::app::CodegenDataModelProviderInstance(&state->storage);

    // Initialize group data provider
    state->group_data_provider.SetStorageDelegate(&state->storage);
    state->group_data_provider.SetSessionKeystore(&state->session_keystore);
    err = state->group_data_provider.Init();
    if (err != CHIP_NO_ERROR) {
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe("failed to initialize group data provider");
        delete state;
        Platform::MemoryShutdown();
        return nullptr;
    }
    Credentials::SetGroupDataProvider(&state->group_data_provider);
    factory_params.groupDataProvider = &state->group_data_provider;

    if (factory_params.dataModelProvider == nullptr) {
        result->code = -2;
        result->message = strdup_safe("CodegenDataModelProviderInstance returned null");
        delete state;
        Platform::MemoryShutdown();
        return nullptr;
    }

    err = DeviceControllerFactory::GetInstance().Init(factory_params);
    if (err != CHIP_NO_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "factory init failed: code=%d (0x%x)", 
                 static_cast<int>(err.AsInteger()), static_cast<unsigned>(err.AsInteger()));
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe(msg);
        delete state;
        Platform::MemoryShutdown();
        return nullptr;
    }

    // Setup operational credentials issuer
    err = state->op_creds_issuer.Initialize(state->storage);
    if (err != CHIP_NO_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "op_creds_issuer.Initialize failed: code=%d", static_cast<int>(err.AsInteger()));
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe(msg);
        delete state;
        DeviceLayer::PlatformMgr().Shutdown();
        Platform::MemoryShutdown();
        return nullptr;
    }

    // Set up device attestation verifier (use default test verifier for sim)
    const Credentials::AttestationTrustStore* trust_store =
        Credentials::GetTestAttestationTrustStore();
    Credentials::SetDeviceAttestationVerifier(
        Credentials::GetDefaultDACVerifier(trust_store));

    // Generate the commissioner's own NOC chain so it has a valid operational identity.
    // Without this, commissioner->GetNodeId() returns 0 and AddNOC fails with
    // CHIP_ERROR_INVALID_ADMIN_SUBJECT.
    static constexpr NodeId kControllerNodeId = 112233;
    FabricId commissioner_fabric_id = (state->fabric_id != 0) ? state->fabric_id : 1;

    Platform::ScopedMemoryBuffer<uint8_t> noc_buf;
    Platform::ScopedMemoryBuffer<uint8_t> icac_buf;
    Platform::ScopedMemoryBuffer<uint8_t> rcac_buf;
    if (!noc_buf.Alloc(Controller::kMaxCHIPDERCertLength) ||
        !icac_buf.Alloc(Controller::kMaxCHIPDERCertLength) ||
        !rcac_buf.Alloc(Controller::kMaxCHIPDERCertLength)) {
        result->code = -3;
        result->message = strdup_safe("NOC buffer alloc failed");
        delete state;
        DeviceLayer::PlatformMgr().Shutdown();
        Platform::MemoryShutdown();
        return nullptr;
    }
    MutableByteSpan noc_span(noc_buf.Get(), Controller::kMaxCHIPDERCertLength);
    MutableByteSpan icac_span(icac_buf.Get(), Controller::kMaxCHIPDERCertLength);
    MutableByteSpan rcac_span(rcac_buf.Get(), Controller::kMaxCHIPDERCertLength);

    Crypto::P256Keypair ephemeral_key;
    err = ephemeral_key.Initialize(Crypto::ECPKeyTarget::ECDSA);
    if (err != CHIP_NO_ERROR) {
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe("ephemeral key init failed");
        delete state;
        DeviceLayer::PlatformMgr().Shutdown();
        Platform::MemoryShutdown();
        return nullptr;
    }

    err = state->op_creds_issuer.GenerateNOCChainAfterValidation(
        kControllerNodeId, commissioner_fabric_id, chip::kUndefinedCATs,
        ephemeral_key.Pubkey(), rcac_span, icac_span, noc_span);
    if (err != CHIP_NO_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "GenerateNOCChain failed: code=%d (0x%x)",
                 static_cast<int>(err.AsInteger()), static_cast<unsigned>(err.AsInteger()));
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe(msg);
        delete state;
        DeviceLayer::PlatformMgr().Shutdown();
        Platform::MemoryShutdown();
        return nullptr;
    }

    // Configure commissioner setup params
    SetupParams commissioner_params;
    commissioner_params.operationalKeypair = &ephemeral_key;
    commissioner_params.controllerRCAC = rcac_span;
    commissioner_params.controllerICAC = icac_span;
    commissioner_params.controllerNOC = noc_span;
    commissioner_params.operationalCredentialsDelegate = &state->op_creds_issuer;
    commissioner_params.controllerVendorId = chip::VendorId::TestVendor1;
    commissioner_params.pairingDelegate = &state->pairing_delegate;
    commissioner_params.permitMultiControllerFabrics = true;

    err = DeviceControllerFactory::GetInstance().SetupCommissioner(commissioner_params, state->commissioner);
    if (err != CHIP_NO_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "SetupCommissioner failed: code=%d (0x%x)", 
                 static_cast<int>(err.AsInteger()), static_cast<unsigned>(err.AsInteger()));
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe(msg);
        delete state;
        DeviceLayer::PlatformMgr().Shutdown();
        Platform::MemoryShutdown();
        return nullptr;
    }

    // Set the IPK (Identity Protection Key) on the commissioner's fabric so that
    // CASE session establishment works after commissioning.
    {
        FabricIndex fabricIndex = state->commissioner.GetFabricIndex();
        uint8_t compressed_fabric_id[sizeof(uint64_t)] = {};
        MutableByteSpan compressed_fabric_id_span(compressed_fabric_id);
        err = state->commissioner.GetCompressedFabricIdBytes(compressed_fabric_id_span);
        if (err != CHIP_NO_ERROR) {
            result->code = static_cast<int32_t>(err.AsInteger());
            result->message = strdup_safe("GetCompressedFabricIdBytes failed");
            state->commissioner.Shutdown();
            delete state;
            DeviceLayer::PlatformMgr().Shutdown();
            Platform::MemoryShutdown();
            return nullptr;
        }
        ByteSpan default_ipk = chip::GroupTesting::DefaultIpkValue::GetDefaultIpk();
        err = Credentials::SetSingleIpkEpochKey(&state->group_data_provider, fabricIndex,
                                                 default_ipk, compressed_fabric_id_span);
        if (err != CHIP_NO_ERROR) {
            result->code = static_cast<int32_t>(err.AsInteger());
            result->message = strdup_safe("SetSingleIpkEpochKey failed");
            state->commissioner.Shutdown();
            delete state;
            DeviceLayer::PlatformMgr().Shutdown();
            Platform::MemoryShutdown();
            return nullptr;
        }
    }

    // Start the CHIP event loop
    err = DeviceLayer::PlatformMgr().StartEventLoopTask();
    if (err != CHIP_NO_ERROR) {
        result->code = static_cast<int32_t>(err.AsInteger());
        result->message = strdup_safe("failed to start CHIP event loop");
        state->commissioner.Shutdown();
        delete state;
        DeviceLayer::PlatformMgr().Shutdown();
        Platform::MemoryShutdown();
        return nullptr;
    }

    state->initialized = true;
    return static_cast<matter_controller_handle_t>(state);
}

void matter_controller_shutdown(matter_controller_handle_t handle) {
    if (!handle) return;

    auto state = static_cast<MatterControllerState*>(handle);
    std::lock_guard<std::mutex> lock(state->mu);

    if (state->shutdown_called) return;
    state->shutdown_called = true;

    DeviceLayer::PlatformMgr().LockChipStack();
    state->commissioner.Shutdown();
    DeviceLayer::PlatformMgr().UnlockChipStack();

    DeviceLayer::PlatformMgr().Shutdown();
    Platform::MemoryShutdown();

    delete state;
}

matter_commission_result_t matter_controller_commission(
    matter_controller_handle_t handle,
    uint32_t setup_code,
    uint16_t discriminator,
    const matter_commission_credentials_t* credentials
) {
    matter_commission_result_t result = {};

    if (!handle) {
        result.code = -1;
        result.message = strdup_safe("invalid handle");
        return result;
    }

    auto state = static_cast<MatterControllerState*>(handle);
    std::unique_lock<std::mutex> lock(state->mu);

    if (!state->initialized || state->shutdown_called) {
        result.code = -1;
        result.message = strdup_safe("controller not initialized or already shut down");
        return result;
    }

    // Configure commissioning parameters
    CommissioningParameters params;

    if (credentials) {
        if (credentials->use_ble_wifi && credentials->ssid && credentials->password) {
            ByteSpan ssid_span(
                reinterpret_cast<const uint8_t*>(credentials->ssid),
                strlen(credentials->ssid)
            );
            ByteSpan password_span(
                reinterpret_cast<const uint8_t*>(credentials->password),
                strlen(credentials->password)
            );
            params.SetWiFiCredentials(
                Controller::WiFiCredentials(ssid_span, password_span)
            );
        }
    }

    // Assign a node ID for the remote device
    NodeId remote_node_id = 1; // first commissioned device gets node 1

    // Generate manual pairing code from setup code + discriminator
    PayloadContents payload;
    payload.setUpPINCode = setup_code;
    payload.discriminator.SetLongValue(discriminator);
    payload.version = 0;
    payload.rendezvousInformation.SetValue(RendezvousInformationFlag::kOnNetwork);

    char manual_code_buf[32];
    MutableCharSpan manual_code_span(manual_code_buf);
    ManualSetupPayloadGenerator generator(payload);
    CHIP_ERROR err = generator.payloadDecimalStringRepresentation(manual_code_span);
    if (err != CHIP_NO_ERROR) {
        result.code = static_cast<int32_t>(err.AsInteger());
        result.message = strdup_safe("failed to generate manual pairing code");
        return result;
    }

    // Use discovery-based PairDevice with the manual pairing code
    DiscoveryType discovery_type = DiscoveryType::kDiscoveryNetworkOnly;
    if (credentials && !credentials->use_on_network) {
        discovery_type = DiscoveryType::kAll;
    }

    // Reset delegate and start async commissioning
    state->pairing_delegate.Reset();

    err = state->commissioner.PairDevice(
        remote_node_id,
        manual_code_buf,
        params,
        discovery_type
    );

    if (err != CHIP_NO_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "PairDevice failed: code=%d (0x%x)", 
                 static_cast<int>(err.AsInteger()), static_cast<unsigned>(err.AsInteger()));
        result.code = static_cast<int32_t>(err.AsInteger());
        result.message = strdup_safe(msg);
        return result;
    }

    // Release our mutex so the CHIP event loop can deliver callbacks
    state->mu.unlock();

    // Wait for commissioning to complete (timeout: 120 seconds)
    bool completed = state->pairing_delegate.WaitForComplete(120000);

    // Re-acquire mutex
    state->mu.lock();

    if (!completed) {
        result.code = -2;
        result.message = strdup_safe("commissioning timed out");
        return result;
    }

    CHIP_ERROR commission_err = state->pairing_delegate.GetError();
    if (commission_err != CHIP_NO_ERROR) {
        char msg[256];
        snprintf(msg, sizeof(msg), "commissioning failed: code=%d (0x%x)", 
                 static_cast<int>(commission_err.AsInteger()), static_cast<unsigned>(commission_err.AsInteger()));
        result.code = static_cast<int32_t>(commission_err.AsInteger());
        result.message = strdup_safe(msg);
        return result;
    }

    result.code = 0;
    result.node_id = static_cast<uint64_t>(state->pairing_delegate.GetNodeId());
    return result;
}

matter_response_t matter_controller_send_command(
    matter_controller_handle_t handle,
    uint64_t node_id,
    uint16_t endpoint,
    uint32_t cluster_id,
    uint32_t command_id,
    const uint8_t* payload,
    uint32_t payload_len
) {
    matter_response_t response = {};

    if (!handle) {
        response.code = -1;
        response.message = strdup_safe("invalid handle");
        return response;
    }

    auto state = static_cast<MatterControllerState*>(handle);
    std::lock_guard<std::mutex> lock(state->mu);

    if (!state->initialized || state->shutdown_called) {
        response.code = -1;
        response.message = strdup_safe("controller not initialized or already shut down");
        return response;
    }

    // Get the device proxy for the target node (establishes CASE session)
    // TODO: Full implementation with proper async callbacks
    // For now, use a simplified synchronous-style approach with callbacks
    Callback::Callback<OnDeviceConnected> on_connected(
        [](void* context, Messaging::ExchangeManager& exchangeMgr, const SessionHandle& sessionHandle) {
            // Device connected - would proceed with command send here
        },
        nullptr
    );
    Callback::Callback<OnDeviceConnectionFailure> on_failure(
        [](void* context, const ScopedNodeId& peerId, CHIP_ERROR error) {
            // Connection failed
        },
        nullptr
    );

    CHIP_ERROR err = state->commissioner.GetConnectedDevice(
        static_cast<NodeId>(node_id),
        &on_connected,
        &on_failure
    );

    if (err != CHIP_NO_ERROR) {
        response.code = static_cast<int32_t>(err.AsInteger());
        response.message = strdup_safe("failed to establish CASE session");
        return response;
    }

    // Encode and send the command
    // TODO: Full implementation with CommandSender and callback handling
    response.code = 0;
    response.data = nullptr;
    response.data_len = 0;
    return response;
}

matter_response_t matter_controller_read_attribute(
    matter_controller_handle_t handle,
    uint64_t node_id,
    uint16_t endpoint,
    uint32_t cluster_id,
    uint32_t attribute_id
) {
    matter_response_t response = {};

    if (!handle) {
        response.code = -1;
        response.message = strdup_safe("invalid handle");
        return response;
    }

    auto state = static_cast<MatterControllerState*>(handle);
    std::lock_guard<std::mutex> lock(state->mu);

    if (!state->initialized || state->shutdown_called) {
        response.code = -1;
        response.message = strdup_safe("controller not initialized or already shut down");
        return response;
    }

    // TODO: Full implementation with ReadClient and callback handling
    response.code = 0;
    response.data = nullptr;
    response.data_len = 0;
    return response;
}

void matter_response_free(matter_response_t* response) {
    if (!response) return;
    if (response->message) {
        free((void*)response->message);
        response->message = nullptr;
    }
    if (response->data) {
        free(response->data);
        response->data = nullptr;
        response->data_len = 0;
    }
}

void matter_result_free(matter_result_t* result) {
    if (!result) return;
    if (result->message) {
        free((void*)result->message);
        result->message = nullptr;
    }
}

void matter_commission_result_free(matter_commission_result_t* result) {
    if (!result) return;
    if (result->message) {
        free((void*)result->message);
        result->message = nullptr;
    }
}

} // extern "C"
