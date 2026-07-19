---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Chosen And Boot Handoff

`/chosen` carries decisions made for this boot. It does not describe a physical device. Firmware commonly creates or modifies it immediately before transferring control to the kernel.

## What Belongs In `/chosen`

Typical data includes:

- the kernel command line in `bootargs`
- the selected console path in `stdout-path`
- architecture- or OS-specific initrd boundaries
- bootloader-provided entropy or firmware handoff metadata where a binding defines it

```dts
/ {
        aliases {
                serial0 = &uart2;
        };

        chosen {
                stdout-path = "serial0:115200n8";
                bootargs = "console=ttyS0,115200 rootwait ro";
        };
};
```

The part before the colon in `stdout-path` is a full path or alias. The suffix is interpreted by the target device binding; serial devices conventionally use baud, parity, bits, and flow-control fields. The path must resolve in the final tree.

An early console still requires working hardware state: the UART must be available and its clocks, power, and pins must permit access. `stdout-path` selects a route; it does not configure every dependency by itself.

## Command-Line Ownership

Linux can obtain command-line fragments from multiple places, including kernel configuration, firmware, the bootloader, and Device Tree. The effective result depends on architecture and kernel configuration. Establish one product policy and test the effective command line:

```sh
cat /proc/cmdline
tr -d '\0' </sys/firmware/devicetree/base/chosen/bootargs
```

These outputs may legitimately differ if the kernel extends, overrides, or ignores the DT command line. Review the relevant `CONFIG_CMDLINE*` settings and boot logs before assuming a bootloader fault.

Keep mutable deployment choices out of the base board DTS where the boot flow owns them. Root filesystem identifiers, recovery modes, debug verbosity, and slot selection often belong to image or boot policy. Conversely, a boot script should not patch permanent hardware wiring into `/chosen` merely because it is easy.

!!! warning
    The command line is observable through logs and `/proc/cmdline`. Do not put passwords, private keys, bearer tokens, or other secrets in `bootargs`.

## Initrd Handoff

Linux commonly consumes `linux,initrd-start` and `linux,initrd-end` from `/chosen`. The values identify the loaded initrd interval and are normally written by firmware. Their cell width is architecture-specific, so follow the architecture boot protocol rather than copying an example.

The initrd range must:

- contain the actual loaded image
- not be overwritten before the kernel consumes it
- be represented consistently in the memory map
- obey architecture placement and alignment requirements

When initrd unpacking fails, compare bootloader load addresses, `/chosen` in the handed-off tree, kernel logs, and reservations. A correct file on storage is not evidence that the handoff range is correct.

## Firmware Mutation Is Part Of The Interface

Think of the final tree as an artifact assembled by several owners:

```text
base DTS + board includes + build overlays
        ↓
packaged DTB
        ↓ firmware selection and fixups
handed-off DTB
        ↓ kernel unflattening
runtime tree
```

Capture all three relevant states when diagnosing boot handoff:

1. the built DTB and its hash
2. the DTB immediately before kernel entry, if the bootloader can save it
3. `/sys/firmware/devicetree/base` or `/proc/device-tree` at runtime

Diffing only source files misses runtime memory sizing, MAC address injection, console selection, initrd metadata, and overlay application.

## Console Failure Checklist

If the kernel appears silent:

1. Check that firmware itself uses the expected UART and electrical path.
2. Resolve `stdout-path`, including any alias, in the final DTB.
3. Check the UART and all ancestors for availability.
4. Validate clock, reset, pinctrl, and power dependencies.
5. Confirm the kernel driver and early-console support are built in where required.
6. Compare `stdout-path` with `console=` and `earlycon` command-line policy.
7. Look for output on every console the platform may have selected.

Avoid “fixing” silence by permanently enabling verbose debug flags in production. Preserve a controlled recovery/debug path.

## Senior Review Questions

- Which component has final write authority over each `/chosen` property?
- Can verified boot authenticate the command line and DTB mutations that affect security policy?
- Is console exposure appropriate for production devices?
- Is boot-slot and recovery policy represented in one authoritative layer?
- Can support tooling capture the exact handed-off DTB without rebuilding it?
- Are bootloader and kernel expectations tested across upgrade combinations?

## Authoritative References

- [Devicetree Specification: `/chosen`](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)
- [Linux kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [Linux arm64 booting requirements](https://docs.kernel.org/arch/arm64/booting.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)

## Next Step

Continue with [Reserved Memory](reserved-memory.md).
