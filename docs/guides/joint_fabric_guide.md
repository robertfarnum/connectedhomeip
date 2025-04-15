# Joint Fabric Guide

- [Joint Fabric Guide](#joint-fabric-guide)
  - [Joint Fabric Example Applications](#joint-fabric-example-applications)
    - [Building the Example Application](#building-the-example-application)
  - [Run Joint Fabric Demo](#run-joint-fabric-demo)
    - [Prepare filesystem and clear previous cached data](#prepare-filesystem-and-clear-previous-cached-data)
    - [Run An Ecosystem Admin (all-clusters-app)](#run-an-ecosystem-admin-all-clusters-app)
    - [Run An Ecosystem Commissioner/Controller (chip-tool)](#run-an-ecosystem-commissionercontroller-chip-tool)
      - [Pair the Ecosystem Admin](#pair-the-ecosystem-admin)

## Joint Fabric Example Applications

chip-tool and all-cluster-app example applications are provided to demonstrate
Joint Fabric feature. You can find them in the examples.

chip-tool example app implements the Ecosystems A and B Commissioner role and
communicates with the all-clusters-app on the other side, facilitating the Joint
Fabric process.

all-clusters-app example app implements the Ecosystems A and B Admin and
demonstrates the end-to-end Joint Fabric feature.

Joint Fabric can be triggered from chip-tool's side. The chip-tool takes on the
Commissioner role. The all-clusters-app, who receives the new ICA signed by the
Anchor Fabric Root CA, assumes the Commissionee role.

### Building the Example Application

-   Building the chip-tool Application

    [chip-tool](https://github.com/project-chip/connectedhomeip/tree/master/examples/chip-tool/README.md)

    > Add the following argument to gn: `--args='chip_enable_joint_fabric=true'`

    ```
    gn --args='chip_enable_joint_fabric=true' gen out
    ninja -C out
    ```

*   Building the all-clusters-app Application

    [all-clusters-app](https://github.com/project-chip/connectedhomeip/tree/master/examples/all-clusters-app/linux/README.md)

    > Add the following argument to gn: `--args='chip_enable_joint_fabric=true'`

    ```
    gn --args='chip_enable_joint_fabric=true' gen out
    ninja -C out
    ```

## Run Joint Fabric Demo

### Prepare filesystem and clear previous cached data

```
# Reset key storage
rm -rf /tmp/chip_*
```

### Run An Ecosystem Admin (all-clusters-app)

On a machine run the all-clusters-app in wifi mode:

`./connectedhomeip/examples/all-clusters-app/linux/out/chip-all-clusters-app --capabilities 0x4 --wifi`

### Run An Ecosystem Commissioner/Controller (chip-tool)

On another machine on the same network run:

`./connectedhomeip/examples/chip-tool/out/chip-tool interactive start`

#### Pair the Ecosystem Admin

`pairing onnetwork 1 20202021`
