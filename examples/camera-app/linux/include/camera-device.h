/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once
#include "camera-device-interface.h"
#include "chime-manager.h"
#include "webrtc-transport-provider-manager.h"
#include <protocols/interaction_model/StatusCode.h>

namespace Camera {

class CameraDevice : public CameraDeviceInterface
{

public:
    CameraDevice();

    // Delegate getters for each cluster
    virtual chip::app::Clusters::ChimeDelegate & GetChimeDelegate() override;
    virtual chip::app::Clusters::WebRTCTransportProvider::Delegate & GetWebRTCTransportProviderDelegate() override;

private:
    // Various cluster server delegates
    ChimeManager mChimeManager;
    chip::app::WebRTCTransportProviderManager mWebRTCTransportProviderManager;
};

} // namespace Camera
