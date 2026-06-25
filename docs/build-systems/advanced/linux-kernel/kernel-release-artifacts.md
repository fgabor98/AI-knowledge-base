---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kernel Release Artifacts

## What Problem Does This Solve?

A kernel release is more than a bootable image. To debug, reproduce, audit, and support a product, you need the complete set of artifacts that describe exactly what was built and deployed.

For embedded Linux, missing release artifacts often turn a field issue into guesswork.

## Core Concepts

- kernel image
- DTB
- modules
- `vmlinux`
- `System.map`
- `.config`
- `Module.symvers`
- initramfs
- artifact manifest
- provenance
- debug bundle
- deployment bundle

## Mental Model

Separate target artifacts from support artifacts:

```text
target deployment artifacts
  Image / zImage / fitImage
  DTB / DTBO
  modules
  initramfs

debug and provenance artifacts
  vmlinux
  System.map
  .config
  Module.symvers
  build log
  manifest
```

The target may not need debug files, but the engineering team does.

## Required Release Bundle

For each board/kernel release, archive:

- kernel image actually deployed
- DTB or FIT image actually deployed
- loadable modules
- optional initramfs
- `vmlinux`
- `System.map`
- final `.config`
- `Module.symvers`
- kernel source commit
- patch stack or metadata revision
- compiler name and version
- build host/container identity
- build command or CI job
- bootloader artifact references
- rootfs/image reference

## Suggested Directory Layout

```text
release/
  manifest.txt
  boot/
    Image
    board.dtb
    initramfs.cpio.gz
  rootfs-overlay/
    lib/modules/<kernelrelease>/
  debug/
    vmlinux
    System.map
    .config
    Module.symvers
  logs/
    build.log
    config-merge.log
  source/
    patches/
    fragment-list.txt
```

Keep the deployable artifacts separate from debug artifacts so production image creation does not accidentally include debug-only files.

## Manifest Contents

A useful manifest includes:

```text
product: example-product
board: custom-am62x
kernel_release: 6.1.80-ti-g123456
kernel_source: git@example/kernel.git
kernel_commit: 1234567890abcdef
kernel_dirty: no
sdk_release: ti-processor-sdk-linux-xx.yy
machine: am62xx-evm
compiler: aarch64-none-linux-gnu-gcc 12.2.1
build_time_utc: 2026-06-18T10:00:00Z
image_sha256: ...
dtb_sha256: ...
modules_sha256: ...
config_sha256: ...
```

The exact format can be text, JSON, YAML, or build-system metadata. The important part is that it is generated and archived consistently.

## Matching Rules

These artifacts must match:

- kernel image and `vmlinux`
- kernel image and `System.map`
- kernel image and `.config`
- kernel image and modules
- modules and `Module.symvers`
- deployed DTB and kernel driver expectations
- initramfs and kernel command line

If one artifact changes, the release identity should change.

## Runtime Verification

On target:

```sh
uname -r
cat /proc/version
cat /proc/cmdline
find /lib/modules -maxdepth 1 -type d
```

For DTB verification:

```sh
find /proc/device-tree -maxdepth 2 -name compatible -print
```

For module verification:

```sh
modinfo /lib/modules/$(uname -r)/kernel/path/to/module.ko
```

Compare the runtime data with the release manifest.

## Why `vmlinux` Matters

`vmlinux` is usually not deployed to the target, but it is needed for:

- symbolized crash analysis
- ftrace/perf workflows
- address-to-line mapping
- kernel oops decoding
- postmortem debugging

Do not discard it for production builds.

## Why `System.map` Matters

`System.map` maps kernel symbols to addresses for one exact build.

Useful when:

- analyzing kernel logs
- comparing symbol addresses
- checking whether a target is running the expected kernel
- debugging stripped images

## Why `.config` Matters

The final `.config` answers:

- was a driver built in or modular?
- was a debug option enabled?
- was module versioning enabled?
- was initramfs built in?
- was a filesystem supported at boot?

Fragments express intent. `.config` records the effective result.

## Why `Module.symvers` Matters

`Module.symvers` is needed when building external modules that must match the released kernel. Archive it with the release even if you do not currently ship external modules.

## CI Artifact Policy

CI should:

- fail if the source tree is dirty for release builds
- produce a manifest
- archive target artifacts
- archive debug artifacts
- archive build logs
- archive final `.config`
- record compiler version
- record selected machine and image target
- publish checksums

## Common Mistakes

- Shipping `Image` and losing `vmlinux`.
- Replacing a DTB without changing release identity.
- Installing modules from a different build.
- Archiving requested fragments but not final `.config`.
- Keeping artifacts only on a developer workstation.
- Not recording the SDK or Yocto metadata revision.
- Debugging a crash with the wrong `System.map`.

## Debugging Checklist

- Does `uname -r` match the release?
- Does `/lib/modules/<kernelrelease>` exist?
- Do deployed boot artifacts match manifest checksums?
- Was the DTB built from the same source revision?
- Are `vmlinux` and `System.map` archived?
- Is final `.config` archived?
- Is `Module.symvers` available for external module rebuilds?

## Related Topics

- [Cross-Building and Installing](cross-building-and-installing.md)
- [Modules and External Modules](modules-and-external-modules.md)
- [Reproducible Kernel Builds](reproducible-kernel-builds.md)
- [BSP Release Reproducibility](../bsp-integration/release-reproducibility.md)

## References

- Linux kernel Kbuild documentation
- Linux kernel external module documentation
- Yocto Project reproducible builds documentation
