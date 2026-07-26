---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Environment, Bootstd, Extlinux, And OS DTB Loading

The Linux DTB can come from a filesystem, network, FIT, EFI path, or an earlier stage. U-Boot environment variables often influence filenames and addresses, while standard boot discovers bootflows through boot devices and boot methods. Trace the actual method rather than assuming `fdtfile` controls every boot.

## Common Environment Variables

U-Boot documents conventional variables such as:

| Variable | Typical role |
|---|---|
| `fdtfile` | DTB filename |
| `fdt_addr_r` | RAM load address for a DTB |
| `fdt_addr` | DTB address in flash or memory on some boards |
| `fdtcontroladdr` | control-FDT address after relocation |
| `bootcmd` | default command sequence |
| `bootargs` | kernel command line input |

These names are conventions and board scripts can use them differently. Inspect the expanded script:

```text
=> printenv bootcmd fdtfile fdt_addr_r fdt_addr fdtcontroladdr
=> env print -a
=> run bootcmd
```

Do not overwrite `fdtcontroladdr` when intending to change the OS DTB. Do not load an OS DTB into `fdt_addr_r` until its range is proven safe.

## Environment Provenance And Trust

Determine whether environment is:

- compiled into U-Boot
- saved in raw flash
- stored in a filesystem
- redundant with sequence/CRC
- authenticated
- reset to defaults after corruption
- mutable from the console or OS

A CRC detects accidental corruption, not malicious modification. If `fdtfile`, boot targets, commands, or verification settings can redirect boot, the environment is part of the policy boundary.

Production designs often constrain mutable variables, authenticate artifacts selected by them, or use a higher-level boot policy that cannot be bypassed through arbitrary commands.

## Standard Boot

U-Boot's standard boot framework models:

- **bootdev**: a device from which boot can be attempted
- **bootmeth**: a method such as extlinux, EFI, or a platform flow
- **bootflow**: one discovered boot instance and its files/state
- **bootstd**: the coordinating device/configuration

The framework scans according to policy and builds bootflows. DT can configure standard-boot behavior, but the hardware control tree, bootflow metadata, and eventual Linux DTB remain separate artifacts.

Inspect:

```text
=> bootdev list
=> bootmeth list
=> bootflow scan -lb
=> bootflow list
=> bootflow select 0
=> bootflow info
```

Command support and flags depend on version/configuration. Capture the selected boot device, method, partition, configuration file, kernel, and FDT.

## Extlinux DTB Selection

An extlinux stanza can specify a Devicetree path, use a directory, or rely on platform behavior depending on parser support and configuration:

```text
label product-a
    linux /Image
    initrd /initramfs.img
    fdt /dtbs/vendor/product-a.dtb
    append root=PARTUUID=... ro
```

Audit:

- which `extlinux.conf` was discovered
- path resolution relative to device/partition/prefix
- case sensitivity and filesystem semantics
- whether the stanza explicitly names the DTB
- what fallback occurs when it does not
- whether kernel and DTB are updated atomically

An extlinux file is policy input. Authenticate it or ensure that every selected payload is independently authorized.

## Scripted Loading

A classic script might do:

```text
load mmc 0:2 ${kernel_addr_r} /boot/Image
load mmc 0:2 ${fdt_addr_r} /boot/dtbs/${fdtfile}
fdt addr ${fdt_addr_r}
booti ${kernel_addr_r} - ${fdt_addr_r}
```

Review every command result. U-Boot scripts that ignore a failed `load` can boot with stale memory from a prior attempt. Before boot:

- clear or invalidate expected state
- check load success and size
- validate FDT header
- ensure the loaded filename matches detected hardware
- ensure addresses and sizes do not overlap
- apply required verification

Interactive success after manually loading a file does not prove `bootcmd` follows the same path.

## Network And Recovery Paths

DHCP/TFTP/PXE can derive filenames from server data, environment, or platform defaults. Recovery often intentionally differs from normal boot but should have an equally explicit trust and compatibility policy.

Test:

- absent server or partial download
- stale cached buffer
- wrong board DTB served
- untrusted DHCP filename
- signed-kernel/unsigned-DTB combinations
- rollback image
- network interruption between kernel and DTB loads

Never let normal boot silently fall into an unauthenticated recovery path.

## Board Identity To Filename

Keep mapping deterministic:

```text
trusted board ID + revision
  -> supported product identity
  -> DTB/FIT configuration
  -> compatible kernel and overlays
```

Sanitize identifiers before constructing paths. Handle unprogrammed EEPROM, invalid CRC, unknown future revisions, and conflicting identity sources. Log both the raw source and selected normalized identity without exposing secrets.

Prefer one maintained mapping function or generated manifest over duplicated shell conditionals in several boot media.

## Runtime Evidence Checklist

Record:

- environment source and current/default distinction
- boot targets and scan order
- selected bootdev/bootmeth/bootflow
- configuration file path and hash
- DTB filename, byte size, hash, and load address
- kernel/initrd ranges
- final working-FDT address
- FIT configuration and verification result where applicable
- fallback decisions

This is enough to reproduce selection before analyzing later mutations.

## Authoritative References

- [U-Boot environment variables](https://docs.u-boot.org/en/latest/usage/environment.html)
- [U-Boot standard boot overview](https://docs.u-boot.org/en/latest/develop/bootstd/index.html)
- [U-Boot extlinux boot method](https://docs.u-boot.org/en/latest/develop/bootstd/extlinux.html)
- [U-Boot `bootflow` command](https://docs.u-boot.org/en/latest/usage/cmd/bootflow.html)
- [U-Boot `load` command](https://docs.u-boot.org/en/latest/usage/cmd/load.html)

## Continue

Proceed to [Bootloader Overlay Application And Working-FDT Safety](bootloader-overlay-application-and-working-fdt-safety.md).
