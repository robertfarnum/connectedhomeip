//go:build !matter

package matter

import (
	"context"
	"fmt"
)

// CommissionCredentials holds network credentials for device commissioning.
type CommissionCredentials struct {
	SSID               string
	Password           string
	OperationalDataset string
	UseBLEWiFi         bool
	UseBLEThread       bool
	UseOnNetwork       bool
}

// Controller is a stub that returns errors when the matter build tag is not set.
type Controller struct{}

// NewController returns an error indicating native Matter support is not compiled in.
func NewController(storagePath string, fabricID uint64) (*Controller, error) {
	return nil, fmt.Errorf("matter: native controller not available (build without 'matter' tag)")
}

// Shutdown is a no-op on the stub controller.
func (c *Controller) Shutdown() {}

// Commission returns an error indicating native Matter support is not available.
func (c *Controller) Commission(ctx context.Context, setupCode uint32, discriminator uint16, creds *CommissionCredentials) (uint64, error) {
	return 0, fmt.Errorf("matter: native controller not available (build without 'matter' tag)")
}

// SendCommand returns an error indicating native Matter support is not available.
func (c *Controller) SendCommand(ctx context.Context, nodeID uint64, endpoint uint16, clusterID, commandID uint32, payload []byte) ([]byte, error) {
	return nil, fmt.Errorf("matter: native controller not available (build without 'matter' tag)")
}

// ReadAttribute returns an error indicating native Matter support is not available.
func (c *Controller) ReadAttribute(ctx context.Context, nodeID uint64, endpoint uint16, clusterID, attributeID uint32) ([]byte, error) {
	return nil, fmt.Errorf("matter: native controller not available (build without 'matter' tag)")
}
