# Joint Fabric Guide

-   [Fabric Synchronization Guide](#fabric-synchronization-guide)
    -   [Joint Fabric Example Applications](#joint-fabric-example-applications)
    -   [Run Joint Fabric Demo on UNIX](#run-fabric-sync-demo-on-rp4)

## Joint Fabric Example Applications

The chip-tool and all-clusters-app example applications are used to demonstrate the Joint Fabric feature. You can find them in the examples.

The chip-tool example app implements the Commissioner role for Ecosystems A and B and communicates with the all-clusters-app on the other side, facilitating the Joint Fabric process.

The all-clusters-app example app implements the Admin role for Ecosystems A and B and demonstrates the end-to-end Joint Fabric feature.

Joint Fabric can be triggered from the chip-tool's side, where it assumes the Commissioner role. The all-clusters-app, which receives the new ICA signed by the Anchor Fabric Root CA, assumes the Commissionee role.

### Building the Example Application

-   Building the chip-tool Application

    [chip-tool](https://github.com/project-chip/connectedhomeip/tree/master/examples/chip-tool/README.md)

*   Building the all-clusters-app Application

    [all-clusters-app](https://github.com/project-chip/connectedhomeip/tree/master/examples/all-clusters-app/linux/README.md)

    > Add the following argument to gn: `--args='chip_enable_joint_fabric=true'`

## Run Joint Fabric Demo on UNIX

### Prepare the filesystem and clear any previous cached data.

```
# Reset key storage
rm -rf /tmp/chip_*
rm -rf /tmp/jf-kvs
mkdir -p /tmp/jf-kvs/all-clusters-app
mkdir -p /tmp/jf-kvs/chip-tool
mkdir -p /tmp/jf-kvs/secondary-chip-tool
```

### Run Ecosystem B Admin (all-clusters-app B)

```
./out/host/chip-all-clusters-app --capabilities 0x4 --discriminator 1261 --passcode 110220033 \
    --KVS /tmp/jf-kvs/all-clusters-app/acs-app --chip-tool-kvs /tmp/jf-kvs/chip-tool
```

### Run Ecosystem B Controller (chip-tool B) and issue a Ecosystem B NOC Chain to Ecosystem B Admin

From a new console, pair the new device:

```
./out/host/chip-tool pairing onnetwork 1 110220033 --storage-directory /tmp/jf-kvs/chip-tool
```

Ensure that the pairing was successful by reading the Ecosystem B Admin Serial Number:

```
./out/host/chip-tool basicinformation read serial-number 1 0 --storage-directory /tmp/jf-kvs/chip-tool
```

### Run Ecosystem A Admin (all-clusters-app A)

Run Ecosystem A preferrably on a different device than the one used to run
Ecosystem B apps - Let's call Device A. Also prepare filesystem and clear
previous cached data as instructed above.

```
./out/host/chip-all-clusters-app --capabilities 0x4 --discriminator 1262 --passcode 110220044 \
    --KVS /tmp/jf-kvs/all-clusters-app/secondary-acs-app --chip-tool-kvs /tmp/jf-kvs/secondary-chip-tool
```

### Run Ecosystem A Controller (chip-tool A) and issue a A NOC Chain to Ecosystem A Admin

From a new console on Device A, pair new device:

```
./out/host/chip-tool pairing onnetwork 3 110220044 --storage-directory /tmp/jf-kvs/secondary-chip-tool
```

Ensure pairing was successful by reading Ecosystem A Admin Serial Number:

```
./out/host/chip-tool basicinformation read serial-number 3 0 --storage-directory /tmp/jf-kvs/secondary-chip-tool
```

### Open a new Pairing Window in Ecosystem B

Return to Device B and use the chip-tool to open a new pairing window in the Ecosystem B Admin.

To allow the other ecosystem to initiate Joint Fabric, run the following command:

```
./out/host/chip-tool pairing open-commissioning-window 1 0 400 1000 1261 --storage-directory /tmp/jf-kvs/chip-tool
```

### Run Ecosystem A Controller (chip-tool A) using Joint Commissioning Method (JCM)

This controller will issue Ecosystem B a new A NOC Chain. Immediately afterward, it will initiate a Joint Fabric exchange to provide the all-clusters-app B with a new ICA signed by A's Root CA.

Execute the following in a new console:

```
./out/host/chip-tool pairing onnetwork-joint-fabric 2 110220033 --storage-directory /tmp/jf-kvs/secondary-chip-tool
```

Ensure that the pairing was successful by reading the Ecosystem B Admin Serial Number:

```
./out/host/chip-tool basicinformation read serial-number 2 0 --storage-directory /tmp/jf-kvs/secondary-chip-tool
```
