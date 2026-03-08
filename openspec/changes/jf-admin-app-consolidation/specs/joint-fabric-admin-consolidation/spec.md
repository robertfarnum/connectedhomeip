## ADDED Requirements

### Requirement: ICAC Certificate Issuance

The `jf-admin-app` SHALL be responsible for generating and signing ICAC
certificates, replacing the need for an external controller to perform this
task.

#### Scenario: Admin App issues ICAC certificate

- **WHEN** the `jf-admin-app` receives a request to issue an ICAC certificate
  (via RPC or internal trigger)
- **THEN** it generates a valid ICAC certificate signed by its root CA
- **AND** the certificate is available for use in the Joint Fabric network

### Requirement: JCM Initialization

The `jf-admin-app` SHALL be responsible for initializing the Joint Fabric
Credential Manager (JCM) service without requiring an external trigger or
ownership transfer.

#### Scenario: Admin App starts JCM

- **WHEN** the `jf-admin-app` starts up
- **THEN** it initializes the JCM service automatically or via a simple start
  command
- **AND** the JCM service enters a ready state

### Requirement: Consolidated Admin RPC Interface

The `jf-admin-app` SHALL expose a Pigweed RPC interface that allows external
clients (like a control app) to trigger administrative actions such as ICAC
issuance and JCM management.

#### Scenario: Remote triggering of admin tasks

- **WHEN** an external client connects to the `jf-admin-app` via Pigweed RPC
- **AND** sends a command to issue an ICAC certificate
- **THEN** the `jf-admin-app` executes the command and returns the result to the
  client

### Requirement: Independent Operation

The `jf-admin-app` SHALL operate independently of the `jf-control-app` for its
core initialization and management functions, eliminating the need for ownership
transfer protocols between the two applications.

#### Scenario: Standalone operation

- **WHEN** the `jf-admin-app` is launched without a `jf-control-app` present
- **THEN** it can successfully initialize its fabric and credential services
- **AND** it does not wait for an ownership transfer before becoming operational
