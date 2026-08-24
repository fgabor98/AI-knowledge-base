---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Bootloaders And Firmware Images

A bootloader is a small security, selection, recovery, and handoff system. A firmware image is more than application code: it includes memory layout, metadata, target identity, version, integrity/authentication data, update state, and a recovery story for power loss and failure.

## Learning Objectives

- distinguish immutable boot ROM, first-stage bootloader, secure boot, and application startup;
- define image layout, headers, alignment, hashes, signatures, and version policy;
- design A/B, swap, or single-slot updates with power-loss recovery;
- implement bootloader/application handoff without undocumented CPU state;
- handle rollback, anti-rollback, recovery, and field diagnostics;
- test corrupted, interrupted, old, wrong-target, and resource-exhaustion cases.

## Boot Stages

A product may contain:

```text
ROM/immutable root -> first-stage loader -> secure loader
                   -> recovery/update agent -> application image
```

Each stage should have a narrow responsibility and a defined trust boundary. The more code runs before authentication and isolation, the larger the attack and failure surface. Keep recovery code simple, bounded, and independently testable.

## Image Manifest

A manifest may contain:

~~~c
#include <stdint.h>

struct image_manifest {
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint32_t image_size;
    uint32_t load_address;
    uint32_t entry_address;
    uint32_t security_version;
    uint8_t  target_id[16];
    uint8_t  image_hash[32];
    uint8_t  signature[64];
};
~~~

The actual fields and encoding are product-specific. Define byte order, alignment, maximum sizes, signature coverage, reserved fields, and versioning. Do not sign a native C struct without a canonical serialization rule; padding and endianness can change the signed bytes.

## Authenticity And Integrity

Integrity detects accidental corruption; authenticity establishes that an authorized signer approved the image. A secure boot/update flow normally:

1. reads and bounds-checks the manifest;
2. validates magic, format, header size, target, image size, and address range;
3. hashes the canonical image and metadata coverage;
4. verifies the signature using a trusted key or key hierarchy;
5. checks security version/anti-rollback policy;
6. records the candidate state atomically;
7. boots only the selected valid image;
8. confirms health before marking it permanent.

Do not act on privileged metadata before authentication if an attacker can alter it. Bind signatures to product, hardware, image type, configuration, and security version to prevent cross-device or cross-role reuse.

## Memory And Image Layout

Define:

- bootloader and application flash regions;
- vector and entry addresses;
- manifest and signature location;
- erase/program alignment;
- RAM shared or reserved during handoff;
- image maximum size and gaps;
- persistent update state and wear budget;
- signature/hash coverage and excluded mutable fields.

The linker script should enforce boundaries with assertions. The post-link tool should verify the final image, not merely trust object sizes. Keep a map, ELF, converted image, manifest, hash, and signing input traceable to one build.

## Single-Slot Updates

Updating in place saves storage but can lose the device if power fails during erase/program. If used, require:

- recovery image or external source;
- atomic progress markers;
- resumable writes;
- verified sectors before advancing;
- a safe fallback when the image is incomplete;
- enough wear budget for retries.

Never treat “download complete” as “bootable.” Verify after writing and before changing the active selection.

## A/B And Swap Updates

A/B layouts keep a known-good image while writing a candidate. A swap or scratch design may exchange sectors with bounded metadata. The selection record should be redundant or journaled and include:

- active slot;
- candidate slot;
- security/version counter;
- trial boot count;
- confirmation state;
- record version and checksum/MAC/signature as appropriate.

On boot, select only a valid image. On a trial boot, require the application to confirm health before consuming the old image. Repeated failures should enter recovery rather than endlessly reboot.

## Anti-Rollback

An attacker may present a correctly signed but vulnerable older image. Use a monotonic security counter, protected fuses, secure storage, or another trusted mechanism. The counter update must be power-loss safe and must occur only after authenticating the candidate. Document factory provisioning, field key rotation, recovery images, and service mode.

## Handoff Contract

Before jumping to the application, define:

- vector-table base and stack pointer;
- instruction-set state and entry encoding;
- interrupt enable/pending/priority state;
- cache and branch predictor state;
- MPU/MMU/TrustZone configuration;
- clocks and peripheral ownership;
- watchdog timeout and refresh responsibility;
- reset reason and boot metadata;
- whether the application performs full reset-style initialization.

An application should not depend on an accidental bootloader state. Either provide a small documented handoff state or make the application reset/reinitialize what it owns.

## Recovery Paths

Recovery can be entered because of:

- no valid image;
- failed signature/hash/target check;
- repeated trial boot failure;
- interrupted update;
- storage corruption;
- watchdog loop;
- explicit user/service request;
- revoked key or version policy.

Recovery must itself have bounded communication, authentication, erase/write limits, watchdog handling, and a way to report why normal boot failed. A recovery mode that accepts unauthenticated replacement images defeats secure boot.

## Testing Matrix

Test at least:

- valid current image;
- valid old image;
- wrong target/product;
- wrong address or oversized image;
- malformed/truncated manifest;
- altered payload and altered signature;
- invalid key, revoked key, and unsupported algorithm;
- interrupted erase/write at every sector boundary;
- corrupted selection record and duplicate records;
- repeated failed trial boots;
- full storage and exhausted wear budget;
- watchdog, brownout, and reset during verification and handoff.

Record image ID, hardware revision, reset cause, selected slot, and failure reason for every test.

## Exercises

1. Define and canonically serialize an image manifest.
2. Add linker assertions for bootloader/application boundaries.
3. Implement a host model of A/B selection and trial confirmation.
4. Inject power loss at every update state transition and verify recovery.
5. Test anti-rollback with valid signatures and old security versions.
6. Verify the handoff state with a debugger and a cold-reset comparison.
7. Create a release artifact set containing ELF, image, manifest, hash, signature input, and provenance.

## Common Mistakes

- treating a hash as authenticity;
- signing native struct padding or noncanonical metadata;
- authenticating after acting on mutable fields;
- omitting target binding or anti-rollback;
- changing the active slot before the candidate is verified;
- updating in place without a recovery strategy;
- lacking atomic selection metadata;
- allowing endless trial boots or watchdog loops;
- depending on undocumented bootloader CPU/peripheral state;
- testing only valid images and uninterrupted power.

## Related Topics

- [Startup, Reset, And Vector Tables](./startup-reset-and-vector-tables.md)
- [Linker Scripts And Memory Layout](../compilation-linking-and-abi/linker-scripts-and-memory-layout.md)
- [Security](../correctness-quality-and-security/security.md)
- [DMA, Cache, And Memory Barriers](./dma-cache-and-memory-barriers.md)
- [Embedded Productization](../../embedded-productization/index.md)

## References

- [Trusted Firmware-M rollback protection](https://tf-m.docs.trustedfirmware.org/en/latest/design_docs/booting/secure_boot_rollback_protection.html)
- [CMSIS startup and vector tables](https://arm-software.github.io/CMSIS_5/5.8.0/Core/html/startup_c_pg.html)
- [NIST Secure Software Development Framework](https://csrc.nist.gov/pubs/sp/800/218/final)
- [The Update Framework](https://theupdateframework.io/)
