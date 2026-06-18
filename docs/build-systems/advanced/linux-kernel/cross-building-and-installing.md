---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Cross-Building and Installing the Kernel

## What Problem Does This Solve?

Embedded kernel builds are usually cross-builds. You build on a workstation or CI runner and install artifacts into a boot partition, root filesystem, SDK, or image build staging area.

This topic covers the commands and install concepts that connect kernel output to a bootable embedded Linux system.

## Core Concepts

- `ARCH`
- `CROSS_COMPILE`
- `O=`
- kernel image target
- `dtbs`
- `modules`
- `modules_install`
- `INSTALL_MOD_PATH`
- `INSTALL_PATH`
- `headers_install`
- staging rootfs
- boot partition

## Mental Model

Build and install are separate:

```text
build kernel image
build DTBs
build modules
install modules into rootfs staging
copy kernel/DTB into boot artifact location
generate final image
```

Do not assume the build command automatically updates the board or image.

## Syntax / API / Mechanism

Cross-build kernel image and DTBs:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 Image dtbs
```

Build modules:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 modules
```

Install modules into a staging rootfs:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH=$PWD/rootfs-staging \
  modules_install
```

Install headers:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_HDR_PATH=$PWD/headers-staging \
  headers_install
```

Print kernel release:

```sh
make O=build-arm64 ARCH=arm64 kernelrelease
```

## Minimal Example

```sh
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 Image dtbs modules
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  INSTALL_MOD_PATH=$PWD/rootfs \
  modules_install
```

Inspect:

```sh
find build -name Image -o -name '*.dtb'
find rootfs/lib/modules -maxdepth 2 -type f
```

## Artifact Deployment

Typical boot partition files:

```text
Image
board.dtb
overlays/*.dtbo
extlinux/extlinux.conf
```

Typical rootfs files:

```text
/lib/modules/<kernel-release>/
/lib/firmware/
```

The exact layout depends on bootloader and distro.

## Kernel Modules And Rootfs

The kernel image and module directory must match:

```sh
uname -r
ls /lib/modules
```

If the board boots kernel `6.1.0-product`, the matching modules should be under:

```text
/lib/modules/6.1.0-product/
```

## Common Scenarios

### Built Kernel But No Modules On Target

Likely causes:

- did not run `modules_install`
- installed modules to wrong staging rootfs
- image generation used a different rootfs
- modules not included in package/image

Check:

```sh
find rootfs-staging/lib/modules -type f -name '*.ko'
```

### Kernel Image And Modules Mismatch

Symptoms:

- `modprobe` fails
- unknown symbols
- invalid module format

Check:

```sh
make O=build kernelrelease
uname -r
modinfo module.ko
```

### Wrong `ARCH`

If `ARCH` is wrong, output paths and compiler expectations are wrong. Always check:

```sh
file build/arch/*/boot/Image
```

### Wrong Cross Compiler

Check verbose output:

```sh
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1 Image
```

Look for the compiler executable actually used.

### Boot Partition Not Updated

The kernel can build correctly and still not boot if the old boot partition remains. Check checksums before and after copying.

## Yocto / Buildroot / TI SDK Integration

Yocto:

- kernel recipe builds image, modules, DTBs
- image recipes include kernel packages/artifacts
- deploy output goes to `tmp/deploy/images/<machine>/`

Buildroot:

- Buildroot controls kernel build and install into target/output images
- kernel image and DTBs usually appear under `output/images/`

TI Processor SDK:

- kernel artifacts appear under SDK-specific deploy locations
- keep `MACHINE`, image target, kernel provider, and deploy artifacts aligned

## Common Mistakes

- Building kernel but forgetting DTBs.
- Copying kernel image without matching DTB.
- Copying kernel image without matching modules.
- Installing modules into the host root by mistake.
- Using host `/lib/modules` for target module builds.
- Reusing stale output directories after changing `ARCH`.
- Assuming `make install` has embedded target semantics.

## Debugging Checklist

- Confirm `ARCH`.
- Confirm `CROSS_COMPILE`.
- Confirm output directory.
- Confirm final `.config`.
- Build image, DTBs, and modules.
- Run `modules_install` into staging rootfs.
- Compare `kernelrelease` with target `uname -r`.
- Check deployed boot partition checksums.
- Check rootfs module directory.
- Boot and validate runtime version.

## Related Topics

- [Kernel Source Tree and Outputs](source-tree-and-outputs.md)
- [Modules and External Modules](modules-and-external-modules.md)
- [Device Tree Builds](device-tree-builds.md)
- [Image Layout and Deployment](../bsp-integration/image-layout-and-deployment.md)

## References

- Linux kernel Kbuild documentation
- Linux kernel external module documentation
- Yocto Project kernel documentation
- Buildroot manual
