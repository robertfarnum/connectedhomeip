//go:build matter

// Package matter provides a Go interface to the Matter (CHIP) device controller.
// It wraps the connectedhomeip C++ SDK via CGo, providing thread-safe device
// commissioning, command sending, and attribute reading capabilities.
//
// Build with: go build -tags matter
// Requires: the native static library (make all) and connectedhomeip SDK.
package matter

import (
	"context"
	"fmt"
	"sync"
)

// #include "matter_controller.h"
import "C"

// CommissionCredentials holds network credentials for device commissioning.
type CommissionCredentials struct {
	SSID               string
	Password           string
	OperationalDataset string
	UseBLEWiFi         bool
	UseBLEThread       bool
	UseOnNetwork       bool
}

// Controller provides a Go-idiomatic interface to the native Matter controller.
// It is safe for concurrent use; all operations are serialized internally.
type Controller struct {
	mu     sync.Mutex
	handle C.matter_controller_handle_t
	closed bool
}

// NewController initializes a new Matter controller with persistent storage.
// storagePath is the directory for KVS fabric state.
// fabricID is the fabric identifier for this controller instance.
func NewController(storagePath string, fabricID uint64) (*Controller, error) {
	handle, err := cInit(storagePath, fabricID)
	if err != nil {
		return nil, fmt.Errorf("matter.NewController: %w", err)
	}
	return &Controller{handle: handle}, nil
}

// Shutdown releases all controller resources. Safe to call multiple times.
func (c *Controller) Shutdown() {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.closed {
		return
	}
	c.closed = true
	cShutdown(c.handle)
}

// Commission pairs and commissions a device onto this controller's fabric.
// Returns the assigned node ID on success.
func (c *Controller) Commission(ctx context.Context, setupCode uint32, discriminator uint16, creds *CommissionCredentials) (uint64, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return 0, fmt.Errorf("matter: controller is shut down")
	}

	// Check context before starting
	select {
	case <-ctx.Done():
		return 0, ctx.Err()
	default:
	}

	nodeID, err := cCommission(c.handle, setupCode, discriminator, creds)
	if err != nil {
		return 0, fmt.Errorf("matter.Commission: %w", err)
	}
	return nodeID, nil
}

// SendCommand sends a cluster command to a commissioned device.
// Returns the TLV-encoded response bytes.
func (c *Controller) SendCommand(ctx context.Context, nodeID uint64, endpoint uint16, clusterID, commandID uint32, payload []byte) ([]byte, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return nil, fmt.Errorf("matter: controller is shut down")
	}

	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}

	data, err := cSendCommand(c.handle, nodeID, endpoint, clusterID, commandID, payload)
	if err != nil {
		return nil, fmt.Errorf("matter.SendCommand: %w", err)
	}
	return data, nil
}

// ReadAttribute reads an attribute value from a commissioned device.
// Returns the TLV-encoded attribute value.
func (c *Controller) ReadAttribute(ctx context.Context, nodeID uint64, endpoint uint16, clusterID, attributeID uint32) ([]byte, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return nil, fmt.Errorf("matter: controller is shut down")
	}

	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}

	data, err := cReadAttribute(c.handle, nodeID, endpoint, clusterID, attributeID)
	if err != nil {
		return nil, fmt.Errorf("matter.ReadAttribute: %w", err)
	}
	return data, nil
}
