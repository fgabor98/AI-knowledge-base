---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Trusted Firmware, OP-TEE, And Secure-World Boundaries

Normal-world DT can describe how Linux invokes trusted firmware and which memory it must avoid. It cannot grant itself access to secure resources. The effective boundary is enforced by exception level, memory controllers, IOMMUs, firewalls, and authenticated firmware.

## Firmware Interface Nodes

The `/firmware` node commonly contains interfaces rather than addressable devices. Examples include PSCI, SCMI, and OP-TEE. Each binding defines its compatible, invocation method, channels, and optional resources.

Conceptually:

```dts
firmware {
        optee {
                compatible = "linaro,optee-tz";
                method = "smc";
        };
};
```

Use the exact binding and platform integration. An `smc` or `hvc` method selects a calling conduit; it does not describe the complete secure-world memory layout or confer trust on the caller.

## PSCI And SCMI Have Different Roles

PSCI standardizes firmware calls for CPU and system power operations. SCMI defines protocols for management services such as clocks, resets, performance, power domains, sensors, and voltage.

If SCMI owns a resource, Linux usually consumes the SCMI provider rather than programming the hardware controller directly. Describing both direct and firmware-mediated providers as active can create two writers to the same control plane.

Map authority:

| Resource | Possible authority |
|---|---|
| CPU on/off | PSCI |
| system suspend/reset | PSCI or platform firmware |
| clock/rate | SCMI or direct clock controller |
| power domain | SCMI or direct genpd provider |
| remote-core lifecycle | system firmware, remoteproc, or secure service |

The product must choose; DT should reflect that choice.

## OP-TEE Shared Memory

OP-TEE provides a Trusted Execution Environment reached through standardized calls. Communication needs memory visible to both normal and secure worlds, with ownership transitions managed by the TEE protocol and implementation.

Depending on platform and binding version, shared memory can be statically reserved or dynamically registered. Static secure and shared-memory regions may be hidden or communicated by firmware rather than fully described to Linux.

Do not assume a region marked `no-map` is secure. Conversely, do not map or dump an unknown reserved region to “see what is there.” Security controllers may fault, and the contents may contain keys or private state.

## Secure Reserved Memory

For every secure-world region, document:

- which boot stage allocates and protects it
- whether it is visible in the delivered DT
- CPU and DMA access-control settings
- whether Linux must reserve it from the page allocator
- whether normal-world drivers can use a mediated shared window
- behavior across suspend, warm reset, kexec, and crash dump

The DT view can intentionally omit details that would be unsafe or irrelevant to normal world, but Linux still needs an authoritative exclusion so it never allocates protected RAM.

`reserved-memory` prevents ordinary Linux allocation; a TrustZone address-space controller, firewall, or equivalent enforces the security boundary. Both layers must cover the same physical extent without off-by-one or granularity errors.

## Auxiliary Firmware Authentication

Some SoCs require trusted firmware to authenticate and start remote-core images. In that model, the Linux remoteproc driver may:

- request an image
- place it in an approved staging region
- invoke a secure service
- receive a handle/status
- attach to the authenticated running core

It may not be allowed to inspect final secure memory or assert reset directly. DT should describe the service relationship defined by the platform binding rather than exposing protected registers.

Threat-model:

- malicious or rolled-back remote firmware
- malformed resource tables and ELF metadata
- remote DMA outside assigned regions
- forged RPMsg messages
- secrets in trace or crash dumps
- recovery paths that bypass authentication

## Secure And Nonsecure DMA

An IOMMU context in normal world does not necessarily control a secure bus master. Likewise, secure firmware can configure firewalls that make a DT-described normal-world region inaccessible.

For each master, verify security state and enforcement point:

```text
master security attribute
  -> interconnect/firewall permission
  -> IOMMU context and stream identity
  -> target memory security state
```

A fault may appear as a remoteproc load timeout, external abort, SMMU fault, or silent access denial depending on where it is blocked.

## Production Diagnostics

Development logs should identify the failing interface and status without exposing secrets, addresses that weaken hardening, or secure payloads. Separate:

- public boot/recovery status
- privileged service diagnostics
- secure audit records
- crash data requiring controlled extraction

Do not enable arbitrary `/dev/mem`, unrestricted debugfs, JTAG, or verbose secure logs as a production workaround.

Test invalid and rollback images, secure service denial, shared-memory exhaustion, remote crash, suspend/resume, warm reset, and loss of a firmware-managed resource. Verify fail-closed behavior and bounded recovery.

## Review Checklist

- each resource has one control authority
- secure RAM exclusion matches hardware protection granularity
- shared memory has a defined ownership and cache protocol
- remote firmware authentication and rollback policy cover recovery too
- normal-world IOMMU settings are not mistaken for complete isolation
- diagnostics preserve evidence without exposing secrets
- DT contains only the interfaces and exclusions Linux is entitled to use

## Authoritative References

- [Linux OP-TEE binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/arm/firmware/linaro,optee-tz.yaml)
- [Linux PSCI binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/arm/psci.yaml)
- [Linux SCMI binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/firmware/arm,scmi.yaml)
- [Linux TEE subsystem documentation](https://docs.kernel.org/driver-api/tee.html)
- [Arm PSCI specification](https://developer.arm.com/documentation/den0022/latest/)

## Continue

Proceed to [Heterogeneous SoC Integration And Recovery Lab](heterogeneous-soc-integration-and-recovery-lab.md).
