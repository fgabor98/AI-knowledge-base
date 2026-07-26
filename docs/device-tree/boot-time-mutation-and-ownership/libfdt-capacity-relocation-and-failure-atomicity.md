---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Libfdt Capacity, Relocation, And Failure Atomicity

A flattened tree is a bounded binary buffer. Adding properties, nodes, reservations, symbols, or overlays requires free space, and moving the blob requires a nonoverlapping destination acceptable to every later consumer. Structural correctness does not prove memory safety.

## Header Size Versus Buffer Capacity

The FDT header's `totalsize` describes the blob extent presented to libfdt. Build files are commonly tightly packed. A bootloader can open the tree into a larger buffer so later mutations have capacity.

Distinguish:

```text
used structured content
header totalsize visible to libfdt
actual reserved RAM interval
```

Increasing `totalsize` without reserving surrounding memory invites overwrite. Reserving a large RAM interval without opening the blob into it does not give libfdt usable space.

## Core Libfdt Operations

Conceptually:

- `fdt_check_header()` validates basic header compatibility
- `fdt_open_into()` moves/opens a tree into a destination buffer with a specified size
- `fdt_setprop()` and node APIs mutate structure
- `fdt_add_mem_rsv()` changes the header reservation map
- `fdt_pack()` removes unused space and produces a compact blob

Every return value matters. Common errors report invalid structure, nonexistent paths, bad offsets, or insufficient space. Never continue after `-FDT_ERR_NOSPACE` with the assumption that earlier operations remain a complete intended transaction.

Use the libfdt API documentation shipped with the `dtc` source version in the product; behavior and available helpers evolve.

## U-Boot Working-FDT Resize

U-Boot's `fdt addr <address> [length]` can select a working tree and expand it to a specified total size. `fdt resize [extra]` can add space for mutations.

Before expansion:

1. validate the source FDT and determine its current total size
2. choose a destination interval reserved for the complete worst case
3. prove no kernel, initrd, overlay, decompression, U-Boot, stack, or firmware range overlaps it
4. copy/open the tree using supported commands
5. inspect the new address and header

The special `fdt_high` no-copy values documented by U-Boot require the original tree to be writable, padded, accessible, and aligned; U-Boot discourages this because it bypasses normal relocation safeguards.

## Calculate Capacity

Budget:

```text
selected base totalsize
  + overlay growth for maximum supported set
  + new strings and phandles
  + board/firmware fixups
  + /chosen and reservation-map growth
  + bounded future margin
```

Measure actual worst-case merged results in CI, then add a documented margin. A fixed “64 KiB should be enough” becomes technical debt when product variants grow.

Also check cell/string amplification: repeated property names share the strings block, but new nodes, long bootargs, symbols, and overlay metadata can add unexpectedly.

## Relocation Timeline

The tree can move:

- from FIT or filesystem load buffer to working buffer
- during overlay preparation
- during `bootm` image relocation
- into an architecture-specific low-memory window
- into an EFI memory-map allocation

Record address and size after each step. A pointer cached by board code before relocation becomes stale. Mutators should obtain the current working-FDT pointer through the supported boot flow rather than a global address copied earlier.

## Memory Interval Audit

Use half-open intervals:

```text
FDT [start, start + capacity)
kernel source/output
initrd
FIT source
overlay inputs
firmware carveouts
U-Boot relocation/malloc/stack
```

Check both current and maximum sizes. Compressed kernels need source and destination ranges. DMA or firmware can write into a region even when CPU interval arithmetic shows no static image overlap.

## Atomicity And Rollback

Libfdt property updates are individual operations, not a product-level transaction. A sequence can fail halfway:

```text
set memory bank 0
set memory bank 1
add reservation
set chosen property -> NOSPACE
```

Safe patterns:

- calculate and reserve space before mutation
- work on a disposable copy of an authenticated base/checkpoint
- group validation before writes
- check every operation
- validate postconditions
- publish the new tree pointer only after completion
- discard and reload on failure

Overlay errors can invalidate both base and overlay blobs in U-Boot's documented flow. Keep pristine inputs.

## Packing And Hashing

Packing removes spare capacity and can change offsets. Do it only when no more mutations are expected or when creating a canonical checkpoint copy. Any cached node offsets or pointers must not survive structural mutations or packing.

For provenance:

- checkpoint a packed copy for stable hashing
- keep the live expanded buffer separate
- define whether hashes cover only structured blob bytes or a signed container
- never include uninitialized padding

The kernel only needs a valid blob in safe memory; unused bootloader capacity is not part of the hardware ABI.

## Failure Diagnosis

| Symptom | Likely cause |
|---|---|
| `FDT_ERR_NOSPACE` | insufficient opened capacity |
| header corrupt after large initrd | range overlap/relocation |
| property appears, later vanishes | mutation applied to stale/other tree |
| only maximum overlay set fails | capacity or phandle/string growth |
| warm boot differs | stale RAM buffer or uninitialized padding |
| Linux rejects FDT early | bad address, alignment, header, or overwritten blob |

Capture the first failing operation, current FDT address/header, and full interval map before retrying.

## Authoritative References

- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot environment variables including `fdt_high`](https://docs.u-boot.org/en/latest/usage/environment.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree compiler and libfdt source](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [U-Boot `bootm` command](https://docs.u-boot.org/en/latest/usage/cmd/bootm.html)

## Continue

Proceed to [RAM Discovery, Reservations, And Memory Fixups](ram-discovery-reservations-and-memory-fixups.md).
