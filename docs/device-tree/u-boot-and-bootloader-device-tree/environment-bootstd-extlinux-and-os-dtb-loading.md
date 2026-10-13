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
label linux
    kernel /boot/Image
    fdtdir /boot/dtb
    fdtoverlays ti/k3-am62x-sk-hdmi-audio.dtbo
    append root=PARTUUID=... ro
```

With `fdtdir`, U-Boot's `boot/pxe_utils.c` appends `${fdtfile}` to the directory. On the AM64x and AM62x boards, TI board code sets `fdtfile` from the EEPROM, for example `ti/k3-am642-sk.dtb`. If `fdtfile` is unset, the code falls back to `${soc}-${board}.dtb`. `fdtoverlays` paths are resolved against the same directory. A stanza with `fdt` instead names one file and ignores `fdtfile` entirely.

Audit:

- which `extlinux.conf` was discovered
- path resolution relative to device/partition/prefix
- case sensitivity and filesystem semantics
- whether the stanza explicitly names the DTB
- what fallback occurs when it does not
- whether kernel and DTB are updated atomically

An extlinux file is policy input. Authenticate it or ensure that every selected payload is independently authorized.

## Scripted Loading

TI's AM64x environment boots from SD or eMMC with these pieces (`include/env/ti/mmc.env`, `board/ti/am64x/am64x.env`):

```text
name_kern=Image
bootdir=/boot
get_kern_mmc=load mmc ${bootpart} ${loadaddr}
        ${bootdir}/${name_kern}
get_fdt_mmc=load mmc ${bootpart} ${fdtaddr} ${bootdir}/dtb/${fdtfile}
run_kern=booti ${loadaddr} ${rd_spec} ${fdtaddr}
```

`bootcmd_ti_mmc` runs `get_kern_mmc`, `get_fdt_mmc`, `get_overlay_mmc`, and `run_kern` in sequence, separated by `;`, so a failed load does not stop the next step.

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

Keep mapping deterministic. TI's AM64x board file does it in one table (`board/ti/am64x/evm.c`):

```c
static struct ti_fdt_map ti_am64_evm_fdt_map[] = {
        {"am64x_gpevm", "ti/k3-am642-evm.dtb"},
        {"am64x_skevm", "ti/k3-am642-sk.dtb"},
        { /* Sentinel. */ }
};
```

`setup_board_eeprom_env()` starts from `am64x_gpevm`, reads the EEPROM, and switches to `am64x_skevm` only if the board name is `AM64-SKEVM` or `AM64B-SKEVM`. An unreadable EEPROM jumps straight to the default, and an unknown name prints `Unidentified board claims ... in eeprom header` and also keeps the EVM. `ti_set_fdt_env()` then sets `fdtfile`, falling back to `CONFIG_DEFAULT_FDT_FILE` or `CONFIG_DEFAULT_DEVICE_TREE` if no entry matches. An SK with a blank EEPROM therefore boots the EVM's DTB. That may be acceptable on a development board; a product should decide it deliberately.

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
