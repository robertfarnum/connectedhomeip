# chip-controller-go

Go bindings for the [Matter](https://github.com/project-chip/connectedhomeip) (CHIP) device controller SDK.

Provides a thread-safe Go API for:
- Initializing a Matter commissioner
- Commissioning devices (on-network, BLE-WiFi, BLE-Thread)
- Sending cluster commands
- Reading device attributes

## Prerequisites

- Go 1.25+
- macOS (Darwin arm64) or Linux
- connectedhomeip SDK (symlinked at `third_party/connectedhomeip`)

## Setup

```bash
# Bootstrap the CHIP SDK (one-time)
make bootstrap

# Build the native static library
make all
```

## Usage

```go
import matter "github.com/comcast-cl/chip-controller-go"

ctrl, err := matter.NewController("/path/to/kvs", 1)
if err != nil {
    log.Fatal(err)
}
defer ctrl.Shutdown()

nodeID, err := ctrl.Commission(ctx, 20202021, 3840, &matter.CommissionCredentials{
    UseOnNetwork: true,
})
```

Build with the `matter` tag to enable native compilation:

```bash
go build -tags matter ./...
```

Without the tag, stub implementations return errors (useful for CI/non-native builds).

## Testing

```bash
# Unit tests (controller lifecycle)
make test

# Integration tests (requires a running Matter device)
make integration-test

# All tests
make test-all
```

The integration test starts `chip-all-clusters-app` automatically. Build it with:

```bash
make app
```

Or set `CHIP_ALL_CLUSTERS_APP` to point at an existing binary.

## Architecture

```
├── controller.go       # Public Go API (thread-safe)
├── bridge.go           # CGo FFI layer
├── stub.go             # No-op stub (without build tag)
├── include/
│   └── matter_controller.h   # C ABI header
├── src/
│   └── matter_controller.cpp # C++ SDK wrapper
├── BUILD.gn            # GN build target (static lib)
├── args.gni            # Platform build arguments
└── third_party/
    └── connectedhomeip # CHIP SDK (symlink or submodule)
```

## Integration

To use this package in another Go project:

```
go get github.com/comcast-cl/chip-controller-go
```

For local development, use a replace directive:

```
replace github.com/comcast-cl/chip-controller-go => ./path/to/matter
```

## License

Apache 2.0
