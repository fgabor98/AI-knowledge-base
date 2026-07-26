---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# `/chosen`, Boot Arguments, Initrd, Console, And Seeds

`/chosen` does not describe a device. It carries firmware-to-OS handoff data for this boot. Because Linux consumes much of it before normal driver probing, malformed values can fail early and leave little diagnostic output.

## Establish Ownership Per Property

Common writers include:

- built DTS defaults
- FIT/extlinux/bootflow configuration
- U-Boot environment and scripts
- generic `bootm` preparation
- board code
- EFI stub
- kexec tooling

Assign one final authority to each property and define whether earlier values are replaced, appended, or preserved. Avoid multiple functions independently concatenating `bootargs`.

## `bootargs`

`bootargs` is a string consumed as the kernel command line under architecture/configuration rules. Build it from typed policy fields rather than arbitrary string fragments:

```text
immutable product baseline
  + selected root/slot
  + recovery reason
  + explicitly permitted operator arguments
```

Validate total length, quoting/whitespace, duplicate keys, and mutually exclusive options. Kernel parameters often use last-one-wins or parameter-specific behavior; duplicated `root=`, `console=`, IOMMU, or security options are not harmless.

Treat mutable environment or network configuration as untrusted unless authenticated. A validly signed kernel can be booted insecurely with hostile command-line policy.

Log a redacted/canonical representation. Command lines can contain network identifiers, debug toggles, or credentials that should never have been placed there.

## `stdout-path`

`stdout-path` identifies the firmware-selected boot console device, optionally with device-defined parameters after `:`:

```dts
chosen {
        stdout-path = "serial0:115200n8";
};
```

An alias such as `serial0` must resolve in `/aliases`, or use a valid full path. Confirm the referenced UART is enabled, pinned, clocked, and available to Linux. The deprecated `linux,stdout-path` and PowerPC `stdout` forms should not be introduced on new platforms.

`stdout-path` is not the same as a Linux `console=` policy in all configurations. Test both early console and final console behavior.

## Initrd Range

The standard Linux properties are:

```dts
chosen {
        linux,initrd-start = <...>;
        linux,initrd-end = <...>;
};
```

The start is inclusive and the end is exclusive:

```text
[initrd-start, initrd-end)
size = end - start
```

Addresses use the root node's address-cell width. Validate:

- end is greater than start without overflow
- interval matches the loaded/authenticated image
- no overlap with kernel output, FDT capacity, firmware, or reservations
- Linux can address the region
- relocation updates both data and properties

Do not encode a byte size in `linux,initrd-end`.

If booting without an initrd, remove stale start/end properties together. A failed load must not leave a previous warm-boot range.

## `rng-seed`

`rng-seed` is a byte array supplied by the bootloader to add entropy. Requirements:

- use a cryptographically appropriate trusted RNG
- generate fresh bytes for every boot
- do not log, persist, reuse, or derive it from public identity
- fail according to the platform's entropy policy
- ensure later exposure does not retain a reusable copy

The seed is handoff data, not proof that the entire random subsystem is fully initialized.

## `kaslr-seed`

`kaslr-seed` is a 64-bit value intended specifically for kernel address randomization. The schema warns that it may reveal information about KASLR offsets and must not be reused for other purposes. On EFI paths with an RNG protocol, the Linux EFI stub can overwrite a bootloader-supplied value.

Use independent derived outputs or independent randomness according to the platform design; do not put the same bytes in `rng-seed` and `kaslr-seed`. Never include seed values in a DT dump shipped from production diagnostics.

## Other Handoff Properties

The current `/chosen` schema includes properties for:

- boot source
- crash-kernel usable memory and ELF core headers
- kexec state
- IMA measurement-log transfer
- UEFI system table and memory-map data
- architecture/platform-specific handoffs

Add only standardized or documented binding properties. `/chosen` is not a general dumping ground for board variables.

For physical address/range properties, follow schema-defined root address/size cell encoding rather than copying a one-cell example.

## Mutation Order

Recommended reasoning:

1. select and authenticate boot configuration
2. load/verify/relocate kernel and initrd
3. establish final working-FDT address/capacity
4. construct canonical command-line policy
5. write initrd and console handoff
6. generate/write fresh seeds as late as practical
7. validate `/chosen` and interval map
8. checkpoint final tree without exposing seeds
9. jump to the kernel without another untracked writer

EFI or architecture code may perform later authorized changes; include those in the provenance model.

## Diagnose

| Symptom | Check |
|---|---|
| kernel ignores initrd | cell width, property names, exclusive end, overlap |
| wrong root filesystem | bootargs source/duplicates/slot policy |
| no early console | alias/path, UART status, parameters, kernel config |
| KASLR repeats | RNG source, seed freshness, later overwrite path |
| final hash changes every boot | expected seed/bootargs data; use redacted semantic comparison |
| stale initrd on failed load | cleanup and warm-boot buffer policy |

## Authoritative References

- [Upstream `/chosen` schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/chosen.yaml)
- [Devicetree Specification: `/chosen`](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html#chosen-node)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux kernel parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [U-Boot `bootm` command](https://docs.u-boot.org/en/latest/usage/cmd/bootm.html)

## Continue

Proceed to [MAC Addresses, Serial Numbers, And Board Identity](mac-addresses-serial-numbers-and-board-identity.md).
