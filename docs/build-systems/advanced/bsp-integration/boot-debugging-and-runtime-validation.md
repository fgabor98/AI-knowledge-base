---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Boot Debugging and Runtime Validation

## What Problem Does This Solve?

BSP integration is not complete when the image builds. It is complete when the correct artifacts boot on the target and runtime evidence proves the expected kernel, U-Boot, DTB, modules, rootfs, services, and configuration are active.

This topic teaches a layer-by-layer boot debugging workflow.

## Core Concepts

- serial console
- boot stage
- first failure
- U-Boot environment
- kernel command line
- DTB validation
- rootfs mount
- init system
- service startup
- runtime artifact verification
- A/B slot awareness

## Mental Model

Debug boot from earliest visible failure:

```text
power/reset
-> ROM
-> SPL/TPL
-> U-Boot
-> kernel
-> rootfs
-> init
-> services
```

Do not debug later layers until earlier layers are proven correct.

## Required Evidence

For every boot test, capture:

- full serial log from reset
- board identifier and hardware revision
- boot media used
- image/artifact checksums
- U-Boot version
- kernel version
- kernel command line
- DTB identity
- rootfs version
- service status

Without this evidence, boot debugging becomes anecdotal.

## Stage 1: No Serial Output

Likely layer:

```text
ROM, power, boot media, first-stage artifact, flashing layout
```

Check:

- power rails
- boot mode straps
- serial port and baud rate
- boot media selection
- first-stage offset
- signed/encrypted image requirements
- board recovery mode

Build artifacts to inspect:

- SPL/TPL/platform first-stage binary
- vendor boot header
- flash layout
- secure boot packaging

## Stage 2: SPL Starts But U-Boot Does Not

Likely layer:

```text
SPL/TPL configuration, DDR init, boot media driver, U-Boot proper artifact
```

Symptoms:

- SPL banner appears
- DDR failure
- cannot load next stage
- hangs before U-Boot prompt

Check:

- SPL size
- DDR configuration
- boot media support in SPL
- path/offset of U-Boot proper
- board-specific packaging
- U-Boot proper artifact checksum

## Stage 3: U-Boot Starts But Kernel Does Not

Likely layer:

```text
U-Boot environment, boot command, filesystem support, kernel/DTB path, load address
```

Useful U-Boot commands:

```text
version
printenv
bdinfo
mmc list
part list mmc 0
ls mmc 0:1
fatls mmc 0:1
ext4ls mmc 0:2
```

Check:

- `bootcmd`
- `bootargs`
- kernel filename
- DTB filename
- rootfs partition
- load addresses
- selected boot media
- whether TFTP/NFS is used instead of local storage

## Stage 4: Kernel Starts But Panics

Likely layer:

```text
kernel config, DTB, rootfs path, storage driver, filesystem support, initramfs
```

Useful checks from boot log:

```text
Kernel command line:
OF: fdt:
VFS: Cannot open root device
Kernel panic - not syncing
```

Common causes:

- wrong `root=`
- missing storage driver
- driver built as module but needed before rootfs mount
- wrong DTB
- rootfs UUID mismatch
- filesystem support missing
- init path missing

Build artifacts to inspect:

- kernel `.config`
- kernel image
- DTB
- rootfs image
- bootloader command line

## Stage 5: Rootfs Mounts But Init Fails

Likely layer:

```text
rootfs contents, init system, permissions, dynamic libraries
```

Symptoms:

- kernel boots
- rootfs mounts
- init missing
- systemd fails early
- shell unavailable

Check:

- `/sbin/init`
- dynamic linker
- shared libraries
- init system package
- file permissions
- rootfs corruption

Useful offline inspection:

```sh
find rootfs -maxdepth 2 -name init -o -name systemd
readelf -l rootfs/bin/busybox
```

## Stage 6: Services Fail

Likely layer:

```text
application package, service unit, config, runtime dependencies, devices
```

Runtime commands:

```sh
systemctl status service-name
journalctl -u service-name
systemctl cat service-name
ldd /usr/bin/app
readelf -d /usr/bin/app
```

Check:

- binary path in service file
- config file presence
- users and groups
- device nodes
- firmware files
- shared libraries
- kernel driver/module loaded
- permissions

## Runtime Validation Commands

General:

```sh
cat /etc/os-release
cat /proc/cmdline
uname -a
mount
lsblk
```

Kernel modules:

```sh
find /lib/modules -maxdepth 2 -type f -name '*.ko'
lsmod
modinfo <module>
```

Device tree:

```sh
tr -d '\0' < /proc/device-tree/model
tr -d '\0' < /proc/device-tree/compatible
find /proc/device-tree -name status
```

Firmware:

```sh
dmesg | grep -i firmware
find /lib/firmware -type f
```

Applications:

```sh
which app
sha256sum /usr/bin/app
systemctl status app.service
```

## A/B And Slot Awareness

If the product uses A/B updates, always identify the active slot:

- active boot partition
- active rootfs partition
- bootloader environment slot variables
- rollback counter
- update status

Common mistake: update slot B, then debug slot A.

## Common Scenarios

### Kernel Loads Wrong DTB

Evidence:

- boot log shows unexpected machine model
- `/proc/device-tree/model` is old
- expected node missing

Debug:

- inspect U-Boot `fdtfile`
- inspect boot partition contents
- inspect FIT image if used
- decompile deployed DTB

### Manual Boot Works But Automatic Boot Fails

Likely causes:

- environment variable mismatch
- saved environment overrides compiled default
- boot script differs from manual commands
- bootcount/rollback logic changes path

Debug:

```text
printenv
env default -a
```

Use destructive environment reset commands only when you know the recovery path.

### Service Works Manually But Not Under systemd

Likely causes:

- different working directory
- missing environment variables
- missing device readiness ordering
- permissions
- service starts before kernel module or device appears

Debug:

```sh
systemctl cat service
journalctl -u service -b
```

## Common Mistakes

- Not capturing the full serial log.
- Debugging rootfs before proving kernel command line.
- Debugging kernel before proving U-Boot loaded the expected kernel and DTB.
- Ignoring saved U-Boot environment.
- Assuming SD boot when the board booted eMMC.
- Forgetting A/B slot state.
- Checking build output but not deployed media.
- Trusting runtime behavior without version/checksum evidence.

## Debugging Checklist

- Capture serial from reset.
- Mark the first failure.
- Identify the boot stage.
- Identify the artifact for that stage.
- Confirm artifact checksum on deployed media.
- Confirm version strings.
- Confirm boot media and slot.
- Confirm kernel command line.
- Confirm DTB identity.
- Confirm rootfs package and service state.
- Record the final fix in source metadata, not only on the target.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Artifact Flow and Provenance](artifact-flow-and-provenance.md)
- [Image Layout and Deployment](image-layout-and-deployment.md)
- [Release Reproducibility](release-reproducibility.md)

## References

- U-Boot documentation
- Linux kernel admin guide
- systemd documentation
- TI Processor SDK Linux documentation
