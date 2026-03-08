# Proposal: Joint Fabric Admin App Consolidation

## Why

The current Joint Fabric demonstration architecture splits responsibilities
between `jf-control-app` and `jf-admin-app` in a way that creates unnecessary
dependencies. Specifically, `jf-control-app` is currently required for ICAC
certificate issuance and starting the Joint Fabric JCM, necessitating a complex
transfer of ownership to `jf-admin-app`. Consolidating these responsibilities
into `jf-admin-app` will simplify the demo architecture, remove the strict
dependency on `jf-control-app`, and streamline the administrative workflow.

## What Changes

- **Move ICAC Certificate Issuer**: Transfer the responsibility for issuing ICAC
  certificates from `jf-control-app` to `jf-admin-app`.
- **Move JCM Start**: Transfer the initialization of the Joint Fabric JCM from
  `jf-control-app` to `jf-admin-app`.
- **Remove Ownership Transfer**: Eliminate the requirement to transfer ownership
  from `jf-control-app` to `jf-admin-app` as a result of the consolidation.
- **Update RPC Interface**: Update the Pigweed RPC server to enable
  `jf-admin-app` communication via an RPC interface (supporting a modified or
  new control app).
- **Deprecate RPC Methods**: Deprecate existing RPC methods that are rendered
  obsolete by this move.

## Capabilities

### New Capabilities

- `joint-fabric-admin-consolidation`: Defines the requirements for the
  consolidated admin application, including ICAC issuance and JCM management.

### Modified Capabilities

<!-- No existing high-level specs are modified; this is a reorganization of application responsibilities. -->

## Impact

- **Applications**: `jf-admin-app` will gain significant logic; `jf-control-app`
  will have logic removed or be repurposed as a thin RPC client.
- **RPC**: Pigweed RPC definitions and server implementations will be updated.
- **Workflows**: The user flow for running Joint Fabric demos will change (fewer
  steps/apps involved).
