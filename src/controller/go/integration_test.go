//go:build matter && integration

package matter

import (
	"context"
	"path/filepath"
	"testing"
	"time"
)

// TestCommissionCameraApp commissions a running Matter camera-app (or all-clusters-app).
// Requires: a Matter device running on the local network with default credentials.
// Run with: go test -tags 'matter integration' -v -timeout 120s -run TestCommissionCameraApp ./matter/
func TestCommissionCameraApp(t *testing.T) {
	tmpDir := t.TempDir()
	storagePath := filepath.Join(tmpDir, "kvs")

	ctrl, err := NewController(storagePath, 1)
	if err != nil {
		t.Fatalf("NewController failed: %v", err)
	}
	defer ctrl.Shutdown()

	ctx, cancel := context.WithTimeout(context.Background(), 90*time.Second)
	defer cancel()

	// Default credentials for chip-camera-app / chip-all-clusters-app
	const setupCode uint32 = 20202021
	const discriminator uint16 = 3840

	t.Logf("Commissioning device (setupCode=%d, discriminator=%d, onNetwork=true)...", setupCode, discriminator)
	nodeID, err := ctrl.Commission(ctx, setupCode, discriminator, &CommissionCredentials{UseOnNetwork: true})
	if err != nil {
		t.Fatalf("Commission failed: %v", err)
	}
	if nodeID == 0 {
		t.Fatal("Commission returned nodeID=0, expected non-zero")
	}
	t.Logf("Commissioned successfully! nodeID=%d", nodeID)

	// Read BasicInformation cluster (0x0028), NodeLabel attribute (0x0005)
	t.Log("Reading BasicInformation.NodeLabel...")
	resp, err := ctrl.ReadAttribute(ctx, nodeID, 0, 0x0028, 5)
	if err != nil {
		t.Logf("ReadAttribute failed (expected until ReadAttribute C++ impl is complete): %v", err)
	} else {
		t.Logf("BasicInformation.NodeLabel response (%d bytes): %x", len(resp), resp)
	}
}
