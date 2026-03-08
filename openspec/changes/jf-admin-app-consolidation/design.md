# Design: Joint Fabric Admin App Consolidation

## Context

The current Joint Fabric demonstration relies on a split architecture where
`jf-control-app` is responsible for initial setup tasks like ICAC certificate
issuance and starting the Joint Fabric Credential Manager (JCM). It then
transfers ownership to `jf-admin-app`. This creates a tight coupling and a
complex handover process. To simplify the architecture and improve usability, we
are consolidating these responsibilities into `jf-admin-app`.

## Goals / Non-Goals

**Goals:**

- Make `jf-admin-app` the single source of truth and control for Joint Fabric
  administration.
- Eliminate the dependency on `jf-control-app` for basic Joint Fabric setup
  (ICAC issuance, JCM start).
- Enable remote control of `jf-admin-app` via a defined Pigweed RPC interface.
- Simplify the user workflow for Joint Fabric demos.

**Non-Goals:**

- Refactoring the underlying Joint Fabric protocol implementation (logic is
  moving, not changing).
- Complete rewrite of the `jf-control-app` UI (it will be adapted to strictly
  use RPCs).

## Decisions

### 1. Relocate Responsibilities to `jf-admin-app`

We will move the source code and logic for the following components from
`jf-control-app` to `jf-admin-app`:

- **ICAC Certificate Issuer**: The logic for generating and signing ICAC
  certificates.
- **JCM Initialization**: The startup sequence for the Joint Fabric Credential
  Manager.

**Rationale**: `jf-admin-app` is the logical place for these administrative
tasks. Centralizing them removes the artificial split and the need for state
synchronization/transfer between two apps during startup.

### 2. Update Pigweed RPC Interface

We will expand the RPC server in `jf-admin-app` to expose the newly moved
functionality.

- **New RPC Services**: Define services for triggering ICAC issuance and JCM
  operations remotely.
- **Client Adaptation**: `jf-control-app` (or any other client) can use these
  RPCs to drive the process, rather than executing the logic locally.

**Rationale**: This decouples the UI/Control plane (`jf-control-app`) from the
Logic/Admin plane (`jf-admin-app`), allowing for more flexible deployment (e.g.,
headless admin app, remote control).

### 3. Deprecate Ownership Transfer

The complex ownership transfer mechanism currently used to hand off control from
`jf-control-app` to `jf-admin-app` will be removed.

**Rationale**: Since `jf-admin-app` will own the resources from the start, there
is no need to transfer them. This eliminates a significant source of complexity
and potential race conditions.

## Risks / Trade-offs

- **Risk**: Existing scripts and documentation for running Joint Fabric demos
  will break.
    - **Mitigation**: We must explicitly update the demo instructions and any
      automation scripts to reflect the new workflow (starting `jf-admin-app`
      directly).
- **Risk**: Increased binary size of `jf-admin-app`.
    - **Mitigation**: The logic being moved is relatively small compared to the
      overall SDK size. We will monitor size impact but do not expect it to be
      prohibitive.
