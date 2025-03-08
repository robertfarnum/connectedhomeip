#!/bin/bash
# Copyright (c) 2024 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script clones the libdatachannel repository for WebRTC support

set -e

CHIP_ROOT="$(dirname "$0")/../.."
LIBDATACHANNEL_ROOT="$CHIP_ROOT/third_party/libdatachannel"
LIBDATACHANNEL_REPO="https://github.com/paullouisageneau/libdatachannel.git"
LIBDATACHANNEL_VERSION="v0.20.3" # Update to the latest stable version as needed

if [ ! -d "$LIBDATACHANNEL_ROOT/repo/.git" ]; then
    echo "Cloning libdatachannel repository from $LIBDATACHANNEL_REPO..."
    
    # Clone the repository
    git clone --recursive $LIBDATACHANNEL_REPO "$LIBDATACHANNEL_ROOT/repo"
    
    # Checkout the specific version
    cd "$LIBDATACHANNEL_ROOT/repo"
    git checkout $LIBDATACHANNEL_VERSION
    git submodule update --init --recursive
    
    echo "libdatachannel repository cloned successfully"
else
    echo "libdatachannel repository already exists"
fi