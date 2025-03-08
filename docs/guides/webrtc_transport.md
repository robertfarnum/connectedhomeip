# WebRTC Transport for Matter

This guide explains how to enable and use the WebRTC transport in Matter applications.

## Overview

The WebRTC transport allows Matter devices to communicate directly over peer-to-peer data channels, which can be useful for:

- Direct device-to-device communication without needing a local network
- Communication through NATs and firewalls
- Audio/video streaming applications that leverage Matter

The implementation uses [libdatachannel](https://github.com/paullouisageneau/libdatachannel), a standalone WebRTC implementation.

## Enabling WebRTC Transport

To enable WebRTC transport in your build:

1. Fetch the libdatachannel dependency:

```bash
./scripts/setup/fetch_libdatachannel.sh
```

2. Build with WebRTC transport enabled:

```bash
./gn_build.sh --args="chip_enable_webrtc_transport=true"
```

## Usage

### Creating a WebRTC Transport

```cpp
#include <transport/raw/WebRTC.h>

// Create a WebRTC transport instance
chip::Transport::WebRTC webrtcTransport;

// Initialize the transport
chip::Transport::InitializeParams initParams;
initParams.exchangeMgr = &exchangeMgr;
initParams.sessionMgr = &sessionMgr;
CHIP_ERROR err = webrtcTransport.Init(initParams);
```

### Establishing a Connection

WebRTC requires signaling to establish a connection. This typically happens through a separate channel:

```cpp
// As the initiator:
webrtcTransport.Connect(nullptr, 0); // IP/port are not used directly
char sdpOffer[4096];
webrtcTransport.GetLocalDescription(sdpOffer, sizeof(sdpOffer));

// Exchange SDP through signaling channel (app specific)
SendSDPToRemotePeer(sdpOffer);

// When receiving remote description:
webrtcTransport.SetRemoteDescription(remoteSdp);

// When receiving ICE candidates:
webrtcTransport.AddIceCandidate(iceCandidate);
```

### Adding to TransportMgr

To use WebRTC with the Matter stack, add it to your transport manager:

```cpp
TransportMgr<WebRTC> transportMgr;
transportMgr.Init(&webrtcTransport);
```

## Signaling Server Considerations

WebRTC requires a signaling mechanism to exchange session descriptions and ICE candidates. This is not provided by Matter and must be implemented separately using:

- A cloud service
- A local network discovery service
- QR codes for initial exchange
- Any other out-of-band communication method

## Security Considerations

- WebRTC includes its own encryption layer (DTLS)
- Matter secure channel runs on top of WebRTC, providing application-layer security
- WebRTC's encryption does not replace Matter security

## Limitations

- Signaling must be handled by the application
- STUN/TURN servers may be needed for NAT traversal
- Not all platforms support WebRTC