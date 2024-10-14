# Joint Fabric Guide

-   [Fabric Synchronization Guide](#fabric-synchronization-guide)
    -   [Joint Fabric Example Applications](#joint-fabric-example-applications)
    -   [Run Joint Fabric Demo on UNIX](#run-fabric-sync-demo-on-rp4)

## Joint Fabric Example Applications

The jf-control-app and chip-jf-admin-app example applications are used to demonstrate the Joint Fabric feature. You can find them in the examples.

The jf-control-app example app implements the Commissioner role for Ecosystems A and B and communicates with the chip-jf-admin-app on the other side, facilitating the Joint Fabric process.

The chip-jf-admin-app example app implements the Admin role for Ecosystems A and B and demonstrates the end-to-end Joint Fabric feature.

Joint Fabric can be triggered from the jf-control-app's side, where it assumes the Commissioner role. The chip-jf-admin-app, which receives the new ICA signed by the Anchor Fabric Root CA, assumes the Commissionee role.

Due to some PKI hard-coding, chip-jf-admin-app must be the first application commissioned by jf-control-app.

### Building the Example Application

-   Building the jf-control-app Application

```
$ cd ~/connectedhomeip/examples/jf-control-app
$ git submodule update --init
$ source third_party/connectedhomeip/scripts/activate.sh
$ gn gen out/debug
$ ninja -C out/debug
```

*   Building the chip-jf-admin-app Application

```
$ cd ~/connectedhomeip/examples/jf-admin-app/linux
$ git submodule update --init
$ source third_party/connectedhomeip/scripts/activate.sh
$ gn gen out/debug --args='chip_enable_joint_fabric=true'
$ ninja -C out/debug
```

> Add the following argument to gn: `--args='chip_enable_joint_fabric=true'`

## Run Joint Fabric Demo on UNIX

### Prepare the filesystem and clear any previous cached data.

```
# Reset key storage
rm -rf /tmp/chip_*
rm -rf /tmp/jf-kvs
mkdir -p /tmp/jf-kvs/chip-jf-admin-app
mkdir -p /tmp/jf-kvs/jf-control-app
mkdir -p /tmp/jf-kvs/secondary-jf-control-app
```

### Run Ecosystem B Admin (chip-jf-admin-app B)

```
./out/debug/chip-jf-admin-app --capabilities 0x4 --discriminator 1261 --passcode 110220033 \
    --KVS /tmp/jf-kvs/chip-jf-admin-app/acs-app --chip-tool-kvs /tmp/jf-kvs/jf-control-app
```

### Run Ecosystem B Controller (jf-control-app B) and issue a Ecosystem B NOC Chain to Ecosystem B Admin

From a new console, pair the new device:

```
$ ./out/debug/jf-control-app
$ >>> pairing onnetwork 1 110220033 --storage-directory /tmp/jf-kvs/jf-control-app
```

Ensure that the pairing was successful by reading the Ecosystem B Admin Serial Number:

```
$ ./out/debug/jf-control-app
$ >>> basicinformation read serial-number 1 0 --storage-directory /tmp/jf-kvs/jf-control-app
```

### Run Ecosystem A Admin (chip-jf-admin-app A)

Run Ecosystem A preferrably on a different device than the one used to run
Ecosystem B apps - Let's call Device A. Also prepare filesystem and clear
previous cached data as instructed above.

```
./out/debug/chip-jf-admin-app --capabilities 0x4 --discriminator 1262 --passcode 110220044 \
    --KVS /tmp/jf-kvs/chip-jf-admin-app/secondary-acs-app --chip-tool-kvs /tmp/jf-kvs/secondary-jf-control-app
```

### Run Ecosystem A Controller (jf-control-app A) and issue a A NOC Chain to Ecosystem A Admin

From a new console on Device A, pair new device:

```
$ ./out/debug/jf-control-app
$ >> pairing onnetwork 3 110220044 --storage-directory /tmp/jf-kvs/secondary-jf-control-app
```

Ensure pairing was successful by reading Ecosystem A Admin Serial Number:

```
$ ./out/debug/jf-control-app
$ >>> basicinformation read serial-number 3 0 --storage-directory /tmp/jf-kvs/secondary-jf-control-app
```

### Open a new Pairing Window in Ecosystem B

Return to Device B and use the jf-control-app to open a new pairing window in the Ecosystem B Admin.

To allow the other ecosystem to initiate Joint Fabric, run the following command:

```
$ ./out/debug/jf-control-app
$ >>> pairing open-commissioning-window 1 0 400 1000 1261 --storage-directory /tmp/jf-kvs/jf-control-app
```

### Run Ecosystem A Controller (jf-control-app A) using Joint Commissioning Method (JCM)

This controller will issue Ecosystem B a new A NOC Chain. Immediately afterward, it will initiate a Joint Fabric exchange to provide the chip-jf-admin-app B with a new ICA signed by A's Root CA.

Execute the following in a new console:

```
$ ./out/debug/jf-control-app
$ >>> pairing onnetwork-joint-fabric 2 110220033 --storage-directory /tmp/jf-kvs/secondary-jf-control-app
```

Ensure that the pairing was successful by reading the Ecosystem B Admin Serial Number:

```
$ ./out/debug/jf-control-app
$ >>> basicinformation read serial-number 2 0 --storage-directory /tmp/jf-kvs/secondary-jf-control-app
```
