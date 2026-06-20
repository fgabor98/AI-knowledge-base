---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Multiconfig and Firmware Builds

## What Problem Does This Solve?

Modern SoCs can run Linux on application cores while auxiliary cores run separate firmware built with different compilers, machines, distributions, or metadata. BitBake multiconfig models these as separate build configurations with explicit cross-configuration dependencies.

## Core Concepts

- multiconfig
- `BBMULTICONFIG`
- `conf/multiconfig/*.conf`
- cross-config dependency
- `mcdepends`
- artifact handoff
- auxiliary-core firmware
- remoteproc
- PRU/R5/M4 firmware
- deployment ownership

## Mental Model

```text
Linux configuration
  machine, toolchain, image, rootfs

firmware configuration
  auxiliary-core target, compiler, firmware recipe

explicit cross-config dependency
-> Linux image/package consumes firmware artifact
```

Multiconfig prevents hidden manual copying between unrelated build trees.

## Configuration Files

Conceptual build configuration:

```bitbake
BBMULTICONFIG = "mcu"
```

Then:

```text
conf/multiconfig/mcu.conf
```

The multiconfig file can select different machine, distro, tune, toolchain policy, or layer-visible configuration.

Exact availability and integration depend on layers and release.

## Building A Multiconfig Target

Conceptual syntax:

```sh
bitbake mc:mcu:product-firmware
```

A normal target and multiconfig target can be built in one invocation or connected through dependencies.

## Cross-Config Dependency

A task in one configuration can depend on a task in another.

Conceptual form:

```bitbake
do_compile[mcdepends] += "mc:mcu:product-firmware:do_deploy"
```

Use the exact task and recipe relationship needed. Do not add broad dependencies to serialize whole builds unnecessarily.

## Worked Example: Firmware Package For Linux Rootfs

Goal:

- build auxiliary firmware in `mcu` config
- deploy deterministic binary
- package it into Linux rootfs under `/lib/firmware/product/`

Firmware config produces:

```text
product-r5f-fw.bin
```

Linux-side packaging recipe declares a cross-config dependency on firmware deploy and installs the artifact into `${D}${nonarch_base_libdir}/firmware/product/`.

Conceptual dependency:

```bitbake
do_install[mcdepends] += "mc:mcu:product-firmware:do_deploy"
```

Conceptual install:

```bitbake
do_install() {
    install -d ${D}${nonarch_base_libdir}/firmware/product
    install -m 0644 ${FIRMWARE_DEPLOY_DIR}/product-r5f-fw.bin \
        ${D}${nonarch_base_libdir}/firmware/product/
}
```

The actual cross-config artifact path should be exposed through deliberate metadata/class interfaces, not guessed from another `tmp` directory.

## Artifact Handoff Design

Good handoff:

- explicit dependency
- stable artifact name
- checksum/version metadata
- deterministic deploy task
- package ownership
- no developer absolute paths

Bad handoff:

- shell script copies from another build after BitBake finishes
- recipe scans arbitrary sibling directories
- image depends on whichever firmware file was built last

## Firmware Versioning

Record firmware identity in:

- filename or manifest
- package version
- build metadata
- runtime logs/API if supported

Linux driver, device tree, and firmware ABI must remain compatible.

## Remoteproc Integration

For Linux-loaded firmware, align:

- firmware package path/name
- device tree `firmware-name` or driver expectations
- remoteproc driver config
- memory carveouts
- resource table
- boot/start policy

An image can contain firmware successfully while runtime loading fails due to DT/ABI/memory mismatch.

## TI SoC Examples

TI devices can include PRU, R5F, M4F, DSP, or other cores depending on SoC.

Potential build sources:

- TI firmware packages
- external MCU+ SDK builds
- product firmware recipes
- prebuilt signed firmware

For each core document:

- producer/toolchain
- artifact filename
- package/install path
- consumer driver
- device tree node
- signing/security requirements
- release version compatibility

## When Not To Use Multiconfig

Do not add multiconfig merely because there are multiple recipes.

Normal recipes are sufficient when all components share the same configuration context and toolchain model.

Use multiconfig when separate configurations are genuinely needed.

## Debugging Cross-Config Builds

Check:

- `BBMULTICONFIG` includes expected name
- config file is found
- target syntax is correct
- cross-config task dependency is correct
- deploy artifact exists
- consumer task reruns when firmware changes
- signatures include artifact/version inputs

## Worked Runtime Verification

On target:

```sh
find /lib/firmware -name '*product*' -type f
dmesg | grep -i remoteproc
```

Where platform interfaces expose state, check firmware version and remote processor status. Preserve Linux and firmware logs together.

## Common Mistakes

- Copying firmware manually between build directories.
- Depending on a directory rather than a producing task.
- Failing to rerun Linux package/image when firmware changes.
- Installing firmware under wrong runtime filename.
- Ignoring firmware/driver/device-tree ABI.
- Using multiconfig when a normal recipe dependency suffices.
- Releasing firmware without toolchain/source provenance.

## Debugging Checklist

- Why is a separate configuration required?
- Which config builds firmware?
- Which task deploys it?
- Which Linux-side task consumes it?
- Is dependency explicit and signature-aware?
- Which package owns firmware?
- Does image manifest include package?
- Does runtime driver request exact filename?
- Are DT memory/remoteproc settings compatible?

## Related Topics

- [Machine and Distro Configuration](machine-and-distro-configuration.md)
- [Kernel Recipe Internals](kernel-recipe-internals.md)
- [CI, Hash Equivalence, and Shared State](ci-hash-equivalence-and-sstate.md)
- [TI Processor SDK Linux](../ti-processor-sdk/index.md)

## References

- BitBake multiconfig documentation
- Yocto Project Reference Manual
- Linux remoteproc documentation
