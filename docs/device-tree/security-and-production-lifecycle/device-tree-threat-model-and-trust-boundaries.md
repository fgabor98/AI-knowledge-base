---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Device Tree Threat Model And Trust Boundaries

A DTB is data to the bootloader, but it becomes policy when trusted kernel code consumes it. A hostile tree can describe nonexistent devices, redirect MMIO, alter DMA topology, reserve or expose memory, change console and boot arguments, select firmware, or enable a peripheral that the product intended to disable.

## Define Assets And Security Goals

Start with properties the system must preserve:

- code and configuration integrity
- confidentiality of keys, measurements, crash data, and protected memory
- hardware isolation between peripherals, CPUs, guests, and security domains
- availability of console, storage, watchdog, update, and recovery paths
- correct product identity and feature entitlement
- auditability of the exact boot configuration

“The DTB is authentic” is only one control. It does not by itself preserve availability, freshness, compatibility, or secrecy.

## Map The DT Attack Surface

| DT construct | Example consequence if malicious or stale |
|---|---|
| `reg`, `ranges`, address/size cells | driver accesses the wrong MMIO or translates an address incorrectly |
| `interrupts`, interrupt parent | misrouted interrupt, interrupt storm, or absent wake path |
| `dma-ranges`, `iommus`, stream IDs | incorrect DMA reachability or isolation |
| `reserved-memory`, `memory` | protected memory exposed, RAM hidden, or overlapping ownership |
| `status` | security-sensitive peripheral enabled or critical supplier disabled |
| clocks, resets, regulators, power domains | unsafe power sequencing or persistent probe failure |
| `/chosen/bootargs` | weakened kernel policy, alternate root, or debug behavior |
| firmware-name properties | unintended firmware selected |
| aliases, stdout path | wrong console or device ordering assumptions |
| overlays | authenticated base transformed by an unauthenticated input |
| serial/MAC/calibration data | cloned identity, broken networking, or unsafe calibration |

Kernel and driver validation reduce accidental misuse, but an authenticated hardware description remains part of the platform's trusted computing base.

## Draw The Boot Trust Chain

```text
SoC ROM / immutable root key
  -> first mutable boot stage
  -> later boot stages and control FDT
  -> boot policy / environment / configuration selector
  -> kernel + DTB + DTBO + initramfs
  -> root filesystem and device firmware
```

For each arrow record:

- verifier and verification code
- key or digest root
- signed object and exact covered bytes
- accepted algorithms and key IDs
- failure behavior
- version/rollback check
- mutable inputs used after verification
- evidence emitted for diagnosis or attestation

A public key inside the same untrusted container it verifies is not a trust anchor. The key must be protected by an earlier trusted stage or immutable hardware policy.

## Separate The Two Device Trees

U-Boot commonly has:

- a **control FDT** used by U-Boot itself
- a **working FDT** prepared for the operating system

Compromising the control FDT may alter bootloader drivers, verification policy, key nodes, update metadata, or board detection. Compromising the working FDT affects the kernel handoff. Protect both according to their role; signing only the final working DTB does not repair a mutable verifier configuration.

## Identify Actors And Capabilities

Use concrete actors rather than “attacker”:

| Actor | Plausible capability |
|---|---|
| network attacker | replace an unauthenticated update or provisioning payload |
| local privileged software | write boot partitions or mutable environment |
| physical attacker | replace removable media, interrupt boot, or probe storage |
| compromised build worker | emit a malicious but internally consistent DTB |
| signing-service compromise | authorize arbitrary release artifacts |
| factory/operator error | select the wrong board configuration or reuse identity data |
| outdated authorized release | boot correctly signed but vulnerable components |

Authentication mainly addresses substitution by actors without a signing key. Reproducibility, review, HSM policy, multi-party release approval, measurement, and anti-rollback address different actors.

## Inventory Mutable Inputs

Typical inputs after an artifact is built include:

- bootloader environment and scripts
- EEPROM board identity and option bits
- straps, fuses, GPIO detection, and secure-monitor calls
- firmware-generated memory maps and reserved regions
- command line fragments
- overlays from boot media or a management controller
- random seeds, serial values, MAC addresses, calibration data
- runtime configfs overlay operations

For each input choose one of these dispositions:

1. Authenticate it.
2. Constrain and validate it under authenticated policy.
3. Measure and attest it.
4. Treat it as untrusted and ensure it cannot affect a security goal.
5. Remove it from production.

## Fail Closed Deliberately

Verification failure behavior is part of the security design. Avoid paths that silently fall back from:

- signed FIT to an unsigned legacy image
- signed production slot to an arbitrary removable-media boot
- enforced configuration signature to manually selected components
- failed board identification to a permissive “universal” DTB
- rejected update to a debug shell capable of disabling verification

Recovery can be both usable and verified: authenticate a dedicated recovery image with a scoped recovery key and restricted policy.

## Threat-Model Worksheet

```text
asset/security goal:
trusted root:
artifact or runtime input:
storage and transport:
authorized producer:
possible attacker:
verification/validation:
freshness mechanism:
post-verification mutation:
failure and recovery path:
field evidence:
residual risk:
```

## Review Exercises

1. Explain how an authentic kernel plus an unauthenticated `reserved-memory` layout can violate confidentiality.
2. Determine whether a write-protected boot partition helps if U-Boot accepts an unsigned DTB from removable media.
3. Classify a factory-written MAC address: it may require authenticity and uniqueness, but not secrecy.
4. Find the trust gap when a signed base DTB receives an overlay named by a mutable environment variable.

## Further Reading

- [U-Boot Verified Boot](https://docs.u-boot.org/en/stable/usage/fit/verified-boot.html)
- [U-Boot fdt command](https://docs.u-boot.org/en/stable/usage/cmd/fdt.html)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [FIT Authenticated Selection And Key Policy](fit-authenticated-selection-and-key-policy.md)
