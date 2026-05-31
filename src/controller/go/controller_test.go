//go:build matter

package matter

import (
	"context"
	"path/filepath"
	"testing"
)

// TestControllerLifecycle tests the full init/shutdown lifecycle.
// CHIP platform is a process-level singleton so all lifecycle assertions
// must happen within a single test.
func TestControllerLifecycle(t *testing.T) {
	tmpDir := t.TempDir()
	storagePath := filepath.Join(tmpDir, "kvs")

	ctrl, err := NewController(storagePath, 1)
	if err != nil {
		t.Fatalf("NewController failed: %v", err)
	}
	if ctrl == nil {
		t.Fatal("NewController returned nil controller")
	}

	// Shutdown should succeed without error
	ctrl.Shutdown()

	// Double shutdown should be safe (idempotent)
	ctrl.Shutdown()

	// Operations after shutdown should fail
	ctx := context.Background()

	_, err = ctrl.Commission(ctx, 12345678, 3840, &CommissionCredentials{UseOnNetwork: true})
	if err == nil {
		t.Error("Commission should fail after shutdown")
	}

	_, err = ctrl.SendCommand(ctx, 1, 1, 0x0028, 0, nil)
	if err == nil {
		t.Error("SendCommand should fail after shutdown")
	}

	_, err = ctrl.ReadAttribute(ctx, 1, 1, 0x0028, 0)
	if err == nil {
		t.Error("ReadAttribute should fail after shutdown")
	}
}
