CHIP_ROOT := $(abspath third_party/connectedhomeip)
OUT_DIR := out
TARGET := $(OUT_DIR)/obj/lib/matter_controller.a

# Use SDK environment for gn/ninja
ACTIVATE := source $(CHIP_ROOT)/scripts/activate.sh

.PHONY: all clean gen ninja bootstrap test integration-test test-all app

all: $(TARGET)

# Bootstrap the SDK environment (only needed once)
bootstrap:
	cd $(CHIP_ROOT) && source scripts/bootstrap.sh

gen:
	$(ACTIVATE) && gn gen $(OUT_DIR)

ninja: gen
	$(ACTIVATE) && ninja -C $(OUT_DIR) :matter_controller

$(TARGET): ninja
	@echo "Built: $@"

# Build the test app (chip-all-clusters-app)
app:
	$(ACTIVATE) && cd $(CHIP_ROOT) && \
	gn gen --root=examples/all-clusters-app/linux out/all-clusters-app \
		--args='target_cpu="arm64" chip_device_platform="darwin" chip_config_network_layer_ble=false chip_build_tests=false' && \
	ninja -C out/all-clusters-app chip-all-clusters-app

# Run unit tests
test:
	./run-tests.sh unit

# Run integration tests (starts test app automatically)
integration-test:
	./run-tests.sh integration

# Run all tests
test-all:
	./run-tests.sh all

clean:
	rm -rf $(OUT_DIR)
