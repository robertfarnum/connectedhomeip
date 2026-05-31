#!/usr/bin/env bash
# run-tests.sh — Run Matter controller tests (unit and/or integration).
#
# Usage:
#   ./run-tests.sh              # unit tests only
#   ./run-tests.sh integration  # start all-clusters-app, run integration test, then stop app
#   ./run-tests.sh all          # unit + integration
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHIP_ROOT="$SCRIPT_DIR/third_party/connectedhomeip"
APP_BIN="${CHIP_ALL_CLUSTERS_APP:-$CHIP_ROOT/out/all-clusters-app/chip-all-clusters-app}"
APP_KVS="/tmp/chip_all_clusters_kvs"
APP_PID=""

cleanup() {
    if [[ -n "$APP_PID" ]] && kill -0 "$APP_PID" 2>/dev/null; then
        echo "Stopping all-clusters-app (PID $APP_PID)..."
        kill "$APP_PID" 2>/dev/null || true
        wait "$APP_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

run_unit() {
    echo "=== Running Matter unit tests ==="
    cd "$SCRIPT_DIR"
    go test -tags 'matter' -v -count=1 ./...
}

start_app() {
    if [[ ! -x "$APP_BIN" ]]; then
        echo "ERROR: all-clusters-app not found at $APP_BIN"
        echo "Build it with: make app"
        echo "Or set CHIP_ALL_CLUSTERS_APP to the binary path"
        exit 1
    fi
    echo "Starting all-clusters-app..."
    rm -f "$APP_KVS"
    "$APP_BIN" --KVS "$APP_KVS" &>/tmp/chip_all_clusters_app.log &
    APP_PID=$!
    # Wait for the app to be ready (listening on port)
    echo "Waiting for app to initialize (PID $APP_PID)..."
    sleep 3
    if ! kill -0 "$APP_PID" 2>/dev/null; then
        echo "ERROR: all-clusters-app failed to start. Check /tmp/chip_all_clusters_app.log"
        exit 1
    fi
    echo "all-clusters-app is running."
}

run_integration() {
    echo "=== Running Matter integration tests ==="
    cd "$SCRIPT_DIR"
    go test -tags 'matter integration' -v -timeout 120s -count=1 -run TestCommissionCameraApp ./...
}

case "${1:-unit}" in
    unit)
        run_unit
        ;;
    integration)
        start_app
        run_integration
        ;;
    all)
        run_unit
        start_app
        run_integration
        ;;
    *)
        echo "Usage: $0 {unit|integration|all}"
        exit 1
        ;;
esac

echo "=== All requested tests passed ==="
