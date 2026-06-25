---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# BSP Artifact Flow and Provenance

## What Problem Does This Solve?

BSP artifact flow explains how source repositories, patches, configurations, and build tasks become the files that a board actually boots and runs.

Provenance answers the matching question: where did each artifact come from, and can it be rebuilt?

Without artifact flow and provenance, embedded Linux debugging becomes guesswork. You may rebuild the kernel while the board still boots an old kernel from eMMC, or patch a DTS file while U-Boot loads a different DTB from a boot partition.

## Core Concepts

- source baseline
- source revision
- patch series
- configuration input
- build task
- deploy artifact
- runtime artifact
- artifact checksum
- boot artifact
- rootfs artifact
- firmware artifact
- manifest
- provenance

## Mental Model

Trace every important runtime fact backward:

```text
observed behavior on board
-> runtime file or boot log
-> deployed artifact
-> build output
-> build task or package
-> source revision, patch, and config
```

Trace every build output forward:

```text
source + patches + config
-> build command or task
-> deploy artifact
-> image layout or package
-> target storage
-> runtime behavior
```

Good BSP integration makes both directions easy.

## Artifact Classes

### Early Boot Artifacts

Examples:

```text
SPL
TPL
tiboot3.bin
u-boot-spl.bin
u-boot.img
u-boot.itb
vendor boot headers
```

These are usually tied to boot ROM rules, boot media layout, SoC requirements, and sometimes signing/encryption.

Questions to answer:

- Which artifact does the ROM load first?
- Which artifact loads U-Boot proper?
- Which file must be flashed at which offset?
- Is the artifact signed or wrapped by a vendor tool?
- Does the artifact include a device tree or configuration blob?

### Kernel Boot Artifacts

Examples:

```text
Image
zImage
uImage
fitImage
vmlinux
System.map
*.dtb
*.dtbo
initramfs
```

Questions to answer:

- Which kernel image format does the bootloader expect?
- Which DTB is loaded with it?
- Are kernel modules built into the rootfs?
- Does the kernel version match the modules directory?
- Is `vmlinux` archived for debugging?

### Runtime Rootfs Artifacts

Examples:

```text
rootfs.tar
rootfs.ext4
rootfs.squashfs
rootfs.ubifs
kernel modules
firmware files
systemd units
application binaries
configuration files
package manifests
```

Questions to answer:

- Which packages are included?
- Which files are runtime files vs development files?
- Which service starts the application?
- Which firmware package owns a firmware blob?
- Does the rootfs match the sysroot used to build applications?

### Image And Deployment Artifacts

Examples:

```text
image.wic
sdcard.img
emmc.img
boot.vfat
rootfs.ext4
update.raucb
swupdate.swu
manifest.json
licenses/
sources/
```

Questions to answer:

- Which partition contains the kernel?
- Which partition contains DTBs?
- Where is the U-Boot environment stored?
- Is there an A/B layout?
- Which image is used for factory flashing?
- Which artifact is used for field update?

## Minimal Artifact Inventory

For each release or test build, record at least:

```text
artifact name
artifact path
artifact checksum
artifact type
source owner
source revision
configuration owner
deployment location
runtime validation method
```

Example:

```text
artifact: Image
path: tmp/deploy/images/am62xx-evm/Image
checksum: sha256:...
source owner: linux-ti-staging recipe
source revision: <commit>
configuration owner: kernel config fragments
deployment location: boot partition
runtime validation: uname -a, boot log
```

## Build-System Examples

### Yocto / OpenEmbedded

Common deploy areas:

```text
tmp/deploy/images/<machine>/
tmp/deploy/licenses/
tmp/deploy/sdk/
tmp/work/<machine-or-arch>/<recipe>/
```

Useful commands:

```sh
find tmp/deploy/images -maxdepth 3 -type f
bitbake -e <recipe>
bitbake -c listtasks <recipe>
```

Artifact questions:

- Which recipe produced this file?
- Which task deployed it?
- Is the image using the expected `MACHINE`?
- Is the package in the rootfs manifest?

### Buildroot

Common output areas:

```text
output/images/
output/build/
output/target/
output/staging/
output/host/
```

Artifact questions:

- Was the file installed to `target`, `staging`, or `host`?
- Did a post-build or post-image script transform it?
- Does `output/images/` contain the image actually flashed?

### TI Processor SDK Linux

Common concepts:

```text
oe-layersetup configuration
selected MACHINE
TI image target
Arago/TI layers
deploy-ti artifacts
```

Artifact questions:

- Which SDK release generated the artifact?
- Which `MACHINE` and image target were used?
- Does the artifact come from the current `deploy-ti` output?
- Does it match the EVM or product board boot media instructions?

## Runtime Validation

Do not stop at "the build produced a file." Confirm the board is using it.

Useful runtime checks:

```sh
cat /proc/cmdline
uname -a
cat /etc/os-release
find /lib/modules -maxdepth 1 -type d
tr -d '\0' < /proc/device-tree/model
```

Useful U-Boot checks:

```text
version
printenv
bdinfo
mmc list
part list mmc 0
```

Useful artifact checks:

```sh
sha256sum Image board.dtb rootfs.ext4 image.wic
file Image u-boot.img
readelf -h vmlinux
```

## Common Scenarios

### Rebuilt Kernel But Board Boots Old Kernel

Likely causes:

- copied kernel to wrong boot partition
- board booted from eMMC instead of SD
- U-Boot loads kernel from TFTP
- A/B slot selected old partition
- image generation used stale deploy artifact

Debug path:

1. Check U-Boot `printenv`.
2. Check boot log kernel version.
3. Check `/proc/cmdline`.
4. Check artifact checksum on boot media.
5. Confirm image generation input.

### Rebuilt DTB But Runtime Device Tree Is Old

Likely causes:

- wrong DTB filename
- U-Boot loads DTB from different partition/path
- FIT image contains embedded DTB
- overlay not applied
- kernel and U-Boot use separate device trees

Debug path:

```sh
tr -d '\0' < /proc/device-tree/model
hexdump -C /proc/device-tree/compatible
```

Compare against:

```sh
dtc -I dtb -O dts deployed-board.dtb
```

### Rootfs Contains Old Application

Likely causes:

- application recipe did not rebuild
- image did not include updated package
- target booted old rootfs slot
- manual file copy shadowed package ownership
- service runs a different binary path

Debug path:

```sh
which app
sha256sum /usr/bin/app
systemctl cat app.service
systemctl status app.service
```

## Common Mistakes

- Treating `tmp/deploy` or `output/images` as self-explanatory.
- Flashing a file without recording its checksum.
- Mixing boot artifacts from one build with a rootfs from another.
- Assuming the newest file by timestamp is the deployed file.
- Ignoring FIT images that bundle kernel and DTB together.
- Forgetting debug artifacts such as `vmlinux` and `System.map`.
- Confusing sysroot contents with runtime rootfs contents.

## Debugging Checklist

- Identify the artifact that should have changed.
- Find the produced build artifact.
- Record its checksum.
- Find where it is copied into the final image.
- Confirm the board boots or runs that exact artifact.
- Confirm source revision and patches.
- Confirm configuration inputs.
- Confirm deployment path and bootloader load path.
- Archive the artifact and manifest for future comparison.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Configuration and Patch Ownership](configuration-and-patch-ownership.md)
- [Boot Debugging and Runtime Validation](boot-debugging-and-runtime-validation.md)
- [Release Reproducibility](release-reproducibility.md)

## References

- Linux kernel documentation
- U-Boot documentation
- Yocto Project documentation
- Buildroot manual
- TI Processor SDK Linux documentation
