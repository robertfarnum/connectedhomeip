#ifndef MATTER_CONTROLLER_H
#define MATTER_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Opaque handle to the Matter controller instance
typedef void* matter_controller_handle_t;

// Error result structure
typedef struct {
    int32_t code;       // 0 = success, non-zero = error
    const char* message; // Human-readable error message (caller must free)
} matter_result_t;

// Commission credentials for BLE-WiFi pairing
typedef struct {
    const char* ssid;
    const char* password;
    const char* operational_dataset; // Thread operational dataset (hex)
    bool use_ble_wifi;
    bool use_ble_thread;
    bool use_on_network;
} matter_commission_credentials_t;

// Commission result
typedef struct {
    int32_t code;
    const char* message;
    uint64_t node_id;
} matter_commission_result_t;

// Command/attribute response
typedef struct {
    int32_t code;
    const char* message;
    uint8_t* data;       // TLV-encoded response data (caller must free)
    uint32_t data_len;
} matter_response_t;

// Initialize the Matter controller
// storage_path: directory for persistent fabric state (KVS)
// fabric_id: fabric identifier for this controller
// Returns: opaque handle, or NULL on failure (check result)
matter_controller_handle_t matter_controller_init(
    const char* storage_path,
    uint64_t fabric_id,
    matter_result_t* result
);

// Shutdown and release all resources
// Safe to call multiple times (idempotent)
void matter_controller_shutdown(matter_controller_handle_t handle);

// Commission a device onto the fabric
// setup_code: numeric setup PIN code
// discriminator: device discriminator
// credentials: network credentials for WiFi/Thread provisioning
matter_commission_result_t matter_controller_commission(
    matter_controller_handle_t handle,
    uint32_t setup_code,
    uint16_t discriminator,
    const matter_commission_credentials_t* credentials
);

// Send a cluster command to a commissioned device
// node_id: target device node ID
// endpoint: target endpoint
// cluster_id: cluster identifier
// command_id: command identifier
// payload: TLV-encoded command payload
// payload_len: length of payload
matter_response_t matter_controller_send_command(
    matter_controller_handle_t handle,
    uint64_t node_id,
    uint16_t endpoint,
    uint32_t cluster_id,
    uint32_t command_id,
    const uint8_t* payload,
    uint32_t payload_len
);

// Read an attribute from a commissioned device
// node_id: target device node ID
// endpoint: target endpoint
// cluster_id: cluster identifier
// attribute_id: attribute identifier
matter_response_t matter_controller_read_attribute(
    matter_controller_handle_t handle,
    uint64_t node_id,
    uint16_t endpoint,
    uint32_t cluster_id,
    uint32_t attribute_id
);

// Free response data allocated by the controller
void matter_response_free(matter_response_t* response);
void matter_result_free(matter_result_t* result);
void matter_commission_result_free(matter_commission_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // MATTER_CONTROLLER_H
