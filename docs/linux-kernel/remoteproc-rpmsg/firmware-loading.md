---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Firmware Loading

## What Problem Does This Solve?

Remote cores run firmware that must match the SoC, board, kernel driver,
Device Tree, memory layout, and userspace protocol. A missing or mismatched
firmware file can look like a kernel driver bug even when the kernel code is
correct.

Firmware loading answers:

- Which file does the kernel request?
- Where must the file be installed?
- Is the file available when the remoteproc driver starts?
- Does the firmware's resource table match the kernel and Device Tree?
- Is the firmware compatible with the RPMsg protocol userspace expects?
- How is firmware updated, signed, verified, and rolled back?

Treat firmware as part of the platform release, not as an afterthought.

## Firmware Loader Basics

Linux drivers normally request firmware with the kernel firmware loader:

```c
const struct firmware *fw;
int ret;

ret = request_firmware(&fw, "demo-fw.elf", dev);
if (ret)
    return dev_err_probe(dev, ret, "failed to load firmware\n");

/* use fw->data and fw->size */

release_firmware(fw);
```

Remoteproc uses the firmware loader internally when starting a Linux-managed
remote processor.

The common runtime location is:

```text
/lib/firmware/
```

Example:

```text
/lib/firmware/demo-r5-fw.elf
/lib/firmware/ti-pruss/am335x-pru0-fw
/lib/firmware/vendor/board/m4-control.elf
```

The exact filename comes from the remoteproc driver, Device Tree, ACPI/platform
data, or a sysfs override when supported.

## Firmware Search And Timing

Firmware must be available at the time it is requested.

Common cases:

| Situation | Firmware Location Requirement |
| --- | --- |
| driver starts remote core after rootfs is mounted | rootfs `/lib/firmware` is enough |
| driver auto-boots before rootfs | firmware must be built into kernel or initramfs |
| remote core is started by bootloader | Linux may attach without loading firmware |
| firmware is loaded by secure firmware | Linux may only request a service or monitor |

If boot logs show firmware load failure early in boot, ask:

```text
Was the root filesystem mounted yet?
Was firmware included in the initramfs?
Is the driver auto-booting too early?
Should the core be started by userspace after rootfs is ready?
```

## Remoteproc Firmware Names

Inspect:

```sh
cat /sys/class/remoteproc/remoteproc0/firmware
```

Start:

```sh
echo start | sudo tee /sys/class/remoteproc/remoteproc0/state
```

Some platforms allow changing the firmware file while stopped:

```sh
echo vendor/demo-fw.elf | sudo tee /sys/class/remoteproc/remoteproc0/firmware
echo start | sudo tee /sys/class/remoteproc/remoteproc0/state
```

Device Tree may specify a firmware name:

```dts
r5f@41000000 {
    compatible = "example,r5f";
    reg = <0x41000000 0x10000>;
    firmware-name = "vendor/r5-control.elf";
};
```

Only use properties defined by the binding for that remoteproc driver. Some
drivers use `firmware-name`; others have driver-specific naming rules.

## ELF Firmware And Resource Tables

Remoteproc commonly loads ELF firmware images. The ELF image may contain:

- program headers describing loadable segments
- symbol/debug information, depending on build
- a resource table section
- firmware text, data, and metadata

The remoteproc core loads the segments into the right memory regions and parses
the resource table when present.

Resource table entries can request:

```text
carveout memory
device memory mappings
trace buffers
virtio devices
vrings
```

Firmware-side build systems often produce a map file. Use it to confirm:

```text
entry point
text/data/bss addresses
resource table address
vring locations
shared memory addresses
trace buffer address and size
```

## Firmware Linker Script

The remote firmware linker script must match the memory map Linux provides.

Example conceptual linker regions:

```ld
MEMORY
{
    IRAM  (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    DRAM  (rwx) : ORIGIN = 0x00010000, LENGTH = 64K
    SHMEM (rw)  : ORIGIN = 0x80000000, LENGTH = 1M
}
```

Linux-side Device Tree might reserve the backing memory:

```dts
reserved-memory {
    r5f_dma_memory_region: r5f-dma-memory@9c000000 {
        compatible = "shared-dma-pool";
        reg = <0x0 0x9c000000 0x0 0x100000>;
        no-map;
    };
};
```

The addresses do not have to be numerically identical if the platform remoteproc
driver translates device addresses. They do have to be intentionally compatible.

## Version Compatibility

Remote firmware has several compatibility surfaces:

| Interface | Compatibility Risk |
| --- | --- |
| resource table | Linux may expect different vdev, vring, trace, or carveout data |
| shared-memory layout | Linux and firmware may read/write different offsets |
| RPMsg endpoint name | Linux driver may not bind |
| RPMsg protocol format | messages may be decoded incorrectly |
| mailbox notify IDs | kicks may not reach the other side |
| power/reset expectations | firmware may assume bootloader or Linux setup |
| cacheability assumptions | data may be stale or corrupted |

Add explicit version information to product protocols. Do not rely only on a
firmware filename.

Example RPMsg hello message:

```c
struct demo_hello {
    __le16 abi_major;
    __le16 abi_minor;
    __le32 features;
};
```

Linux should reject incompatible major versions and log the firmware version it
observed.

## Firmware Metadata

Good firmware release artifacts include:

- source revision or build ID
- target SoC and core
- board or product variant
- ABI/protocol version
- toolchain version
- resource table summary
- linker map
- license metadata
- signature or hash, if the product requires it

Firmware can expose version metadata through:

- RPMsg handshake
- shared-memory header
- trace/log banner
- firmware build note
- vendor-specific control channel

Example firmware log banner:

```text
r5-control fw 2.3.1 board=alpha abi=4 build=2026-06-30
```

## Initramfs And Built-In Firmware

Early firmware loading can require firmware in the initramfs or built into the
kernel image.

Questions:

- Does the remote core need to boot before rootfs?
- Is the rootfs on storage that depends on the remote core?
- Is userspace responsible for starting remoteproc later?
- Does the distribution package firmware into the initramfs?

Lab check:

```sh
lsinitramfs /boot/initrd.img-$(uname -r) | grep firmware
```

Embedded systems may use different initramfs tooling. The key point is timing:
the file must exist before the request happens.

## Packaging Firmware

In product builds, firmware should be installed by the build system.

Good:

```text
firmware source or binary package
  -> build recipe
     -> /lib/firmware/vendor/demo-fw.elf
        -> image manifest records file and version
```

Bad:

```text
developer manually scp's firmware to target
  -> test passes once
  -> CI image and production image still miss firmware
```

For Yocto/OpenEmbedded or SDK-based systems, model remote firmware as a package
or recipe dependency of the image or machine. For Debian-style systems, use a
firmware package with clear versioning and dependencies.

## Signing, Authentication, And Policy

Firmware security policy is platform-specific.

Possible models:

- unsigned lab firmware
- kernel loads file but secure firmware authenticates it
- bootloader authenticates and starts remote firmware
- kernel verifies signature through an integrity policy
- encrypted or signed vendor firmware package

Driver code should not invent security policy locally. Use the platform's
firmware loading, secure boot, and update mechanisms.

Questions for product design:

- Can arbitrary root users replace remote firmware?
- Does remote firmware have DMA access to system memory?
- Does firmware affect safety, security, or radio certification?
- Is rollback allowed?
- Is firmware tied to a signed boot chain?

## Updating Firmware

Firmware updates must be coordinated with kernel, Device Tree, bootloader, and
userspace.

Safe update plan:

```text
define compatibility matrix
install new firmware atomically
stop or detach remote core safely
start new firmware
verify version handshake
fallback if startup fails
preserve crash logs
```

Avoid:

- replacing a running firmware file and assuming the running core changes
- changing shared-memory layout without updating Device Tree
- changing RPMsg endpoint names without updating Linux drivers or userspace
- updating bootloader-started firmware from Linux without owning boot flow

## Missing Firmware Debugging

Symptoms:

```text
remoteproc remoteproc0: powering up demo-fw.elf
remoteproc remoteproc0: request_firmware failed: -2
```

Checks:

```sh
cat /sys/class/remoteproc/remoteproc0/firmware
find /lib/firmware -maxdepth 4 -type f | sort
dmesg | grep -Ei 'firmware|remoteproc|rproc'
```

Common causes:

| Cause | Fix |
| --- | --- |
| filename mismatch | align driver/DT/sysfs name with installed file |
| file installed in wrong directory | install under `/lib/firmware` or configured firmware path |
| rootfs not mounted yet | include firmware in initramfs or start later |
| permissions/labeling issue | fix packaging, permissions, or security policy |
| compressed file not supported by flow | install expected raw file |
| firmware package missing from image | add image/package dependency |

## Resource Table Debugging

Symptoms:

- remote core starts but RPMsg device never appears
- remoteproc reports unsupported resource
- firmware load fails around carveouts
- trace buffers do not appear

Checks:

```sh
dmesg | grep -Ei 'resource|carveout|vdev|vring|trace|remoteproc'
```

Firmware-side:

```sh
readelf -S demo-fw.elf
readelf -l demo-fw.elf
```

Look for:

```text
.resource_table section
loadable segment addresses
memory sizes
alignment
```

If you have the firmware map file, compare it against Device Tree
`reserved-memory` and remoteproc logs.

## Compatibility Checklist

Before releasing a firmware image, record:

- kernel commit or release
- Device Tree source and DTB version
- remoteproc driver expected firmware name
- firmware image hash
- firmware ABI/protocol version
- resource table summary
- reserved-memory layout
- userspace service version
- bootloader handoff expectations
- recovery policy

This is tedious once, but it prevents long debugging sessions caused by mixed
artifacts from different SDK releases.

## Practice Exercises

1. Inspect a target's remoteproc firmware name and find the corresponding file
   under `/lib/firmware`.
2. Temporarily rename the firmware in a lab and record the exact failure in
   `dmesg`.
3. Use `readelf -S` on a remote firmware image and identify the resource table
   section if present.
4. Compare the firmware linker map with the board's reserved-memory nodes.
5. Design a two-field firmware ABI handshake and decide how Linux should reject
   incompatible firmware.

## Review Checklist

- Is the firmware file installed by the build system?
- Is the file available at the time remoteproc requests it?
- Does the filename match driver, Device Tree, and sysfs expectations?
- Does the firmware resource table match Linux-side memory and RPMsg design?
- Are firmware and kernel ABI versions checked explicitly?
- Is update, rollback, signing, and crash preservation policy defined?
- Is bootloader-started firmware handled differently from Linux-started firmware?

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Initramfs Options](../configuration-and-platform-policy/initramfs-options.md)
- [Embedded Productization](../../embedded-productization/index.md)

## Official References

- [Firmware Loading API](https://docs.kernel.org/driver-api/firmware/request_firmware.html)
- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Remoteproc Sysfs ABI](https://docs.kernel.org/ABI/testing/sysfs-class-remoteproc)
