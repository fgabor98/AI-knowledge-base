---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# RAM Discovery, Reservations, And Memory Fixups

Boot firmware can discover installed DRAM, train it, exclude failed regions, and reserve memory for itself or secure components. Linux needs one coherent final view. Updating only `/memory` while ignoring reservation mechanisms can expose firmware-owned bytes to the page allocator.

## Four Related Views

Keep these separate:

| Mechanism | Meaning |
|---|---|
| `/memory` nodes | physical RAM presented as available hardware |
| `/reserved-memory` children | named regions with binding-defined use/attributes |
| FDT memory reservation map | flat list of physical ranges the client must not use |
| `/chosen/linux,usable-memory-range` | additional Linux usable-range limitation, mainly crash kernels |

Architecture firmware tables or command-line limits can add other views. The final Linux memory map is their intersection and policy, not a copy of one `reg`.

## Installed, Trained, And Usable RAM

Possible inputs include:

- board population straps or EEPROM
- memory-controller discovery
- SPD
- DRAM training firmware
- ECC initialization and failure data
- secure-firmware allocation
- bad-memory testing
- product SKU limits

Define which source is authoritative and whether a disagreement aborts boot. A mutable environment variable must not enlarge RAM beyond hardware discovery.

Represent only addressable, successfully initialized RAM. If DRAM has holes or multiple banks, encode multiple `reg` tuples using root `#address-cells` and `#size-cells`.

## Correct Cell Encoding

For a root with two address and two size cells:

```dts
memory@80000000 {
        device_type = "memory";
        reg = <0x0 0x80000000 0x0 0x40000000>,
              <0x1 0x00000000 0x0 0x80000000>;
};
```

This is illustrative. Mutator code must construct big-endian cells with appropriate helpers and check address-width overflow. Truncating a 64-bit size into one cell can create a plausible but dangerous smaller/larger range.

Prefer one generic helper with tests over board-local hand encoding.

## Reservations Must Track Ownership

For each excluded range, record:

- physical base and size
- current owner
- reason
- normal/secure access attributes
- whether it must be named for a driver
- whether speculative mapping is forbidden
- lifetime across boot, suspend, kexec, and reset

Use `/reserved-memory` when a described consumer or attributes such as `no-map`/pool semantics matter. Use the FDT reservation map according to platform/client conventions for generic do-not-use ranges. Some platforms use both for related purposes; avoid accidental inconsistent duplicates.

Neither mechanism configures a hardware firewall. Secure firmware must enforce secure ownership separately.

## Dynamic Secure-Firmware Reservations

Trusted firmware might allocate runtime memory based on installed DRAM, feature set, or secure payloads. It can:

- modify the FDT before passing it onward
- return reservation data through a defined call/handoff
- supply a prebuilt adjusted tree

Normal-world code should serialize exactly the authorized ranges and reject out-of-RAM, overlapping, misaligned, or overflowing results. It should not “round down” a secure reservation to make schema or alignment checks pass.

If secure firmware already wrote a reservation, U-Boot should verify/preserve it rather than add a second conflicting representation.

## DRAM Size Versus Board DTS

Several strategies exist:

- one DTS describes maximum topology and firmware shrinks sizes
- separate base DTBs describe stable SKU populations
- one base contains placeholder memory populated entirely by firmware
- earlier firmware supplies the complete tree

Choose based on what is stable and trustworthy. Mutation is appropriate for discovered size; materially different bank wiring or reserved-layout ABI may justify separate board/SKU descriptions.

Do not let one generic base erase product distinctions needed before discovery.

## Crash Kernels And Usable Memory

`/chosen/linux,usable-memory-range` limits where a crash-dump kernel may use memory. It does not create RAM; bytes must also be valid under `/memory` or another platform memory map. Its tuple uses root address and size cell counts.

Crash handoff can also carry an ELF core-header range. Those regions must be reserved from the crash kernel while remaining readable according to crash policy. Test them separately from ordinary boot.

## Mutation Order

A robust order is often:

1. obtain and validate physical DRAM discovery
2. construct canonical bank intervals
3. obtain secure/firmware reservations
4. validate reservations against their allowed address domains
5. update `/memory`
6. update named reserved-memory nodes and/or reservation map
7. add crash-specific limits when applicable
8. validate no prohibited overlaps
9. compare intended map with the serialized final tree

The platform can require another order, but validation should operate on a canonical interval model before encoding.

## Linux Evidence

```sh
dmesg | grep -Ei 'memory|reserved|cma|memblock|efi'
cat /proc/iomem
cat /proc/meminfo
ls -l /sys/firmware/devicetree/base/reserved-memory
```

Early boot lines are important because later allocator summaries collapse detail. Compare the final pre-handoff DTB with Linux's live tree and `/proc/iomem`.

## Deliberate Failure Checks

- bank end overflows physical address width
- two banks overlap
- reservation begins inside one bank and ends outside it
- zero-sized or wraparound reservation
- secure region omitted on warm boot
- firmware reports more RAM than the SKU maximum
- crash usable range includes only holes/reservations
- different stages use decimal/hex or bytes/MiB inconsistently

Abort or enter safe recovery; never “repair” untrusted intervals silently.

## Authoritative References

- [Devicetree Specification: memory node](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html#memory-node)
- [Upstream memory-node schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/memory.yaml)
- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)
- [Upstream `/chosen` schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/chosen.yaml)
- [Linux boot-time memory management](https://docs.kernel.org/core-api/boot-time-mm.html)

## Continue

Proceed to [`/chosen`, Boot Arguments, Initrd, Console, And Seeds](chosen-bootargs-initrd-console-and-seeds.md).
