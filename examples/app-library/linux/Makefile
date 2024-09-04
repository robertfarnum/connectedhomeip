libmatter.so: matter-bridge.cpp
	g++ \
		-I/workspaces/connectedhomeip/third_party/nlio/repo/include \
		-I/workspaces/connectedhomeip/third_party/nlassert/repo/include \
		-I/workspaces/connectedhomeip/zzz_generated/app-common \
		-I/workspaces/connectedhomeip/out/linux/gen/include \
		-I/workspaces/connectedhomeip//src/include \
		-I../../platform/linux \
		-I../../../src -o \
		-o libmatter.so matter-bridge.cpp -lCHIP.so \
		-std=c++17 -O3 -Wall -Wextra -fPIC -shared