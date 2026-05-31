//go:build matter

package matter

/*
#cgo CFLAGS: -I${SRCDIR}/include
#cgo CFLAGS: -I${SRCDIR}/third_party/connectedhomeip/src/include
#cgo CFLAGS: -I${SRCDIR}/third_party/connectedhomeip/src
#cgo CFLAGS: -I${SRCDIR}/out/gen/include
#cgo CFLAGS: -I${SRCDIR}/third_party/connectedhomeip/config/standalone
#cgo LDFLAGS: ${SRCDIR}/out/obj/lib/matter_controller.a
#cgo LDFLAGS: -lstdc++ -lobjc
#cgo LDFLAGS: -framework CoreFoundation -framework Foundation -framework Network -framework CoreData -framework IOKit -framework Security -framework SystemConfiguration
#include "matter_controller.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"unsafe"
)

// cInit calls the C matter_controller_init function.
func cInit(storagePath string, fabricID uint64) (C.matter_controller_handle_t, error) {
	cPath := C.CString(storagePath)
	defer C.free(unsafe.Pointer(cPath))

	var result C.matter_result_t
	handle := C.matter_controller_init(cPath, C.uint64_t(fabricID), &result)
	if result.code != 0 {
		msg := C.GoString(result.message)
		C.matter_result_free(&result)
		return nil, fmt.Errorf("matter init failed (code %d): %s", result.code, msg)
	}
	C.matter_result_free(&result)
	return handle, nil
}

// cShutdown calls the C matter_controller_shutdown function.
func cShutdown(handle C.matter_controller_handle_t) {
	C.matter_controller_shutdown(handle)
}

// cCommission calls the C matter_controller_commission function.
func cCommission(handle C.matter_controller_handle_t, setupCode uint32, discriminator uint16, creds *CommissionCredentials) (uint64, error) {
	var cCreds C.matter_commission_credentials_t
	var cSSID, cPassword, cDataset *C.char

	if creds != nil {
		if creds.SSID != "" {
			cSSID = C.CString(creds.SSID)
			defer C.free(unsafe.Pointer(cSSID))
			cCreds.ssid = cSSID
		}
		if creds.Password != "" {
			cPassword = C.CString(creds.Password)
			defer C.free(unsafe.Pointer(cPassword))
			cCreds.password = cPassword
		}
		if creds.OperationalDataset != "" {
			cDataset = C.CString(creds.OperationalDataset)
			defer C.free(unsafe.Pointer(cDataset))
			cCreds.operational_dataset = cDataset
		}
		cCreds.use_ble_wifi = C.bool(creds.UseBLEWiFi)
		cCreds.use_ble_thread = C.bool(creds.UseBLEThread)
		cCreds.use_on_network = C.bool(creds.UseOnNetwork)
	}

	result := C.matter_controller_commission(
		handle,
		C.uint32_t(setupCode),
		C.uint16_t(discriminator),
		&cCreds,
	)
	defer C.matter_commission_result_free(&result)

	if result.code != 0 {
		msg := C.GoString(result.message)
		return 0, fmt.Errorf("commission failed (code %d): %s", result.code, msg)
	}
	return uint64(result.node_id), nil
}

// cSendCommand calls the C matter_controller_send_command function.
func cSendCommand(handle C.matter_controller_handle_t, nodeID uint64, endpoint uint16, clusterID, commandID uint32, payload []byte) ([]byte, error) {
	var payloadPtr *C.uint8_t
	var payloadLen C.uint32_t
	if len(payload) > 0 {
		payloadPtr = (*C.uint8_t)(unsafe.Pointer(&payload[0]))
		payloadLen = C.uint32_t(len(payload))
	}

	response := C.matter_controller_send_command(
		handle,
		C.uint64_t(nodeID),
		C.uint16_t(endpoint),
		C.uint32_t(clusterID),
		C.uint32_t(commandID),
		payloadPtr,
		payloadLen,
	)
	defer C.matter_response_free(&response)

	if response.code != 0 {
		msg := C.GoString(response.message)
		return nil, fmt.Errorf("send_command failed (code %d): %s", response.code, msg)
	}

	if response.data_len > 0 && response.data != nil {
		data := C.GoBytes(unsafe.Pointer(response.data), C.int(response.data_len))
		return data, nil
	}
	return nil, nil
}

// cReadAttribute calls the C matter_controller_read_attribute function.
func cReadAttribute(handle C.matter_controller_handle_t, nodeID uint64, endpoint uint16, clusterID, attributeID uint32) ([]byte, error) {
	response := C.matter_controller_read_attribute(
		handle,
		C.uint64_t(nodeID),
		C.uint16_t(endpoint),
		C.uint32_t(clusterID),
		C.uint32_t(attributeID),
	)
	defer C.matter_response_free(&response)

	if response.code != 0 {
		msg := C.GoString(response.message)
		return nil, fmt.Errorf("read_attribute failed (code %d): %s", response.code, msg)
	}

	if response.data_len > 0 && response.data != nil {
		data := C.GoBytes(unsafe.Pointer(response.data), C.int(response.data_len))
		return data, nil
	}
	return nil, nil
}
