---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Runtime Tree Surfaces And Boot-FDT Identity

Linux may expose both the raw flattened blob supplied at boot and a filesystem view of its live unflattened tree. They answer different questions. Availability depends on kernel configuration, architecture, security policy, and mounted virtual filesystems.

## Inventory The Surfaces

```bash
uname -a
ls -ld /sys/firmware/devicetree /sys/firmware/devicetree/base 2>/dev/null
ls -ld /proc/device-tree 2>/dev/null
ls -l /sys/firmware/fdt 2>/dev/null
readlink -f /proc/device-tree 2>/dev/null
```

Typical surfaces:

| Surface | Meaning |
|---|---|
| `/sys/firmware/fdt` | raw boot FDT blob when the platform/kernel exports it |
| `/sys/firmware/devicetree/base` | filesystem representation of Linux's current live OF tree |
| `/proc/device-tree` | compatibility/convenience view, commonly resolving to the exported live tree |

Do not assume `/proc/device-tree` is an independent snapshot. Resolve it on the target.

Absence can mean the kernel was not booted with DT, support/export is disabled, sysfs/procfs is unavailable, or access is restricted. It does not prove firmware supplied no hardware description.

## Raw Boot Blob

When present, preserve it before runtime experiments:

```bash
sudo cp -- /sys/firmware/fdt boot-fdt.dtb
sha256sum boot-fdt.dtb
stat -c '%s bytes' boot-fdt.dtb
dtc -I dtb -O dts -o boot-fdt.decoded.dts boot-fdt.dtb
```

This is the byte-oriented artifact Linux exposes as the boot FDT. It already includes firmware and bootloader changes made before handoff. It is the best runtime-side candidate for exact comparison with a pre-handoff capture.

It does not generally track later changes to the live unflattened tree, such as runtime overlay application. Prove behavior for the target kernel rather than assuming the raw blob is rewritten.

## Live Tree Filesystem

```bash
find /sys/firmware/devicetree/base -maxdepth 2 -type d -print | sort
```

Directories represent nodes; ordinary files represent properties. Property files contain raw bytes:

```bash
find /sys/firmware/devicetree/base/chosen -maxdepth 1 -type f -printf '%f\n'
```

The filesystem view reflects the live OF node/property model and can reflect dynamic overlay changes. It is not the original FDT container:

- no original header or padding
- no original structure/string block ordering
- no recoverable source includes/comments/macros
- FDT reservation block is not represented as ordinary node files
- filesystem traversal order is not release serialization

Do not hash the directory tree and compare it to a DTB hash.

## Root Identity

```bash
dtroot=/sys/firmware/devicetree/base

tr '\0' '\n' <"$dtroot/model"
tr '\0' '\n' <"$dtroot/compatible"
```

`model` is descriptive. The ordered root `compatible` list is machine identity. Preserve every entry, not only the first.

If `model` says revision B but compatible says revision A, capture the raw bytes and boot evidence before “fixing” either; firmware may have mutated one source inconsistently.

## `/chosen` Handoff

```bash
find "$dtroot/chosen" -maxdepth 1 -type f -printf '%f\n' | sort
tr '\0' '\n' <"$dtroot/chosen/bootargs" 2>/dev/null
tr '\0' '\n' <"$dtroot/chosen/stdout-path" 2>/dev/null
```

Treat boot arguments, initrd bounds, seeds, and console fields according to their bindings and sensitivity. Do not print or export random seeds in routine field logs.

Compare `/proc/cmdline` with DT `bootargs`, but do not assume they must be identical. Architecture, built-in command line, bootconfig, or kernel policy can alter the effective command line. The difference needs a documented owner.

## Identity Checkpoints

Collect:

```text
built DTB hash
packaged/extracted DTB hash
bootloader selected base and overlay hashes
pre-handoff captured DTB hash
/sys/firmware/fdt hash when available
live-tree normalized semantic capture
```

Interpretation:

- pre-handoff hash equals boot-FDT hash: strong evidence of byte-identical handoff/export
- hashes differ but semantics match: investigate packing, padding, capture extent, or kernel export behavior
- live semantics differ from boot FDT: identify runtime overlay/kernel changes
- no raw boot blob: rely on pre-handoff capture plus live semantic comparison

## Kernel And Boot Identity

Record the software reading the tree:

```bash
uname -r
cat /proc/version
cat /proc/cmdline
cat /sys/kernel/uevent_seqnum 2>/dev/null
```

Kernel build configuration may be available at `/proc/config.gz` or `/boot/config-$(uname -r)`, but neither path is universal. Verify its identity rather than assuming a neighboring config matches the running kernel.

## Runtime Overlay Check

If the system supports runtime overlays through a product-specific mechanism, capture live tree before and after application. Do not infer overlay state solely from a loader directory; confirm introduced nodes/properties in the live tree and corresponding device-model effects.

The raw `/sys/firmware/fdt` blob can remain unchanged while `/sys/firmware/devicetree/base` changes.

## First-Minute Checklist

- [ ] kernel release/build identity captured
- [ ] all available DT surfaces inventoried
- [ ] raw boot FDT copied and hashed before mutation
- [ ] root model and all compatible strings decoded safely
- [ ] `/chosen` captured with sensitive fields handled
- [ ] `/proc/device-tree` target resolved
- [ ] live tree captured before overlays/rebind/reboot
- [ ] pre-handoff and packaged hashes obtained where possible

## Authoritative References

- [Linux sysfs firmware OF ABI](https://docs.kernel.org/admin-guide/abi-testing-files.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)

## Continue

Proceed to [Binary-Safe Property Inspection And Decoding](binary-safe-property-inspection-and-decoding.md).
