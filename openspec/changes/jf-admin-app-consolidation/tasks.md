## 1. Move ICAC Responsibilities

- [x] 1.1 Extract ICAC certificate generation/signing logic from
      `jf-control-app`.
- [x] 1.2 Integrate ICAC certificate logic into `jf-admin-app` initialization or
      management classes.
- [x] 1.3 Ensure `jf-admin-app` has the necessary root CAs and crypto materials
      available on startup to issue these certificates.

## 2. Move JCM Initialization

- [x] 2.1 Locate the Joint Fabric Credential Manager (JCM) initialization
      sequence inside `jf-control-app`.
- [x] 2.2 Migrate this startup sequence to `jf-admin-app`.
- [x] 2.3 Modify the `jf-admin-app` startup flow to initialize the JCM
      automatically or wait for an explicit RPC start command instead of an
      ownership transfer sequence.

## 3. Remove Ownership Transfer Logic

- [x] 3.1 Strip the logic in `jf-control-app` that initiates the transfer of
      ownership to `jf-admin-app`.
- [x] 3.2 Strip the logic in `jf-admin-app` that listens for and handles the
      incoming ownership transfer from `jf-control-app`.

## 4. Define and Update Pigweed RPCs

- [x] 4.1 Define new Pigweed RPC services/methods for remote administration of
      `jf-admin-app` (e.g., `IssueIcac`, `StartJcm`).
- [x] 4.2 Implement the server-side handlers for these new RPCs in
      `jf-admin-app` to trigger the moved logic.
- [x] 4.3 Deprecate/remove old RPC definitions and handlers related to the old
      ownership transfer and distributed setup flow.
- [x] 4.4 Update `jf-control-app` (or client tooling) to use the new RPC
      interface to trigger demo setup steps instead of running them locally.

## 5. Clean up and Verification

- [ ] 5.1 Remove newly dead/unused code in `jf-control-app` stemming from the
      consolidation.
- [ ] 5.2 Build both applications (`jf-admin-app` and `jf-control-app`) and
      verify compilation succeeds.
- [ ] 5.3 Run a test Joint Fabric setup using the new architecture to verify
      ICAC issuance and JCM start work correctly over RPC or default startup.
