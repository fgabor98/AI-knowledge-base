---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# `libfdt` Programming, Capacity, And Error Discipline

`libfdt` is the low-level C library used by firmware, bootloaders, host utilities, and other software to read and modify flattened Device Trees. Its API is deliberately small and allocation-agnostic. The caller owns buffer validity, capacity, lifetime, locking, policy, and failure recovery.

## Validate Before Traversal

Conceptual pattern:

```c
int ret;

ret = fdt_check_header(fdt);
if (ret != 0) {
        log_error("invalid FDT: %s", fdt_strerror(ret));
        return ret;
}
```

Header validation alone cannot prove that an untrusted pointer has a sufficiently large mapped buffer. Establish the containing buffer length through the transport/container and use stronger full-blob checks available in the product's `libfdt` version.

Never trust `fdt_totalsize()` from an unvalidated header to map, copy, hash, or allocate arbitrary memory.

## Find Nodes And Properties

```c
int node;
int len;
const fdt32_t *cells;

node = fdt_path_offset(fdt, "/bus@f0000/mmc@fa00000");
if (node < 0)
        return node;

cells = fdt_getprop(fdt, node, "reg", &len);
if (cells == NULL)
        return len;
```

For lookup functions, negative values encode `-FDT_ERR_*`. For `fdt_getprop()`, the length output contains a negative error when no property is returned. Check the contract of each function; do not apply one generic convention blindly.

Decode 32-bit cells:

```c
if (len < (int)sizeof(*cells))
        return -FDT_ERR_BADVALUE;

uint32_t first = fdt32_to_cpu(cells[0]);
```

Use binding-derived counts and overflow-safe arithmetic before combining addresses/sizes.

## Strings And Bounds

Property strings are stored inside bounded property data. Verify:

- nonnegative length
- required NUL termination within that length
- string-list iteration stays within the property
- expected count/cardinality
- copied destination length

Prefer `libfdt` string-list helpers available in the target version. A property pointer is not an unbounded C string merely because the binding says “string.”

## Traversal

Use library iterators rather than parsing structure tokens manually:

- subnode iteration helpers
- compatible search helpers
- property iteration helpers
- phandle lookup helpers
- reservation-map APIs

Check every returned offset. Node offsets are positions in the current flattened structure, not stable IDs.

## Pointer And Offset Invalidation

Pointers returned by `fdt_getprop()` refer into the blob. Structural mutation can move blocks and invalidate:

- property data pointers
- node offsets
- property offsets
- string-table pointers
- assumptions about totalsize/free space

Safe sequence:

```text
lookup -> copy/consume bounded data -> mutate -> discard offsets/pointers -> look up again
```

Never hold an internal pointer across `fdt_setprop()`, node addition/deletion, overlay application, open-into, or packing unless the exact API explicitly guarantees it.

A host test against the kernel's `scripts/dtc/libfdt` on the AM62 LP SK DTB shows both failure modes. Adding a string property to `/chosen` in the packed file returns `-3` (`FDT_ERR_NOSPACE`). After `fdt_open_into()` into a buffer 4 KiB larger, the same call succeeds, but a `stdout-path` pointer obtained from `fdt_getprop()` before the write no longer equals the pointer returned after it.

## Create Writable Capacity

Packed build artifacts often have no free space. Open into a validated larger destination:

```c
ret = fdt_open_into(src_fdt, dst_buf, dst_capacity);
if (ret != 0)
        return ret;
```

The destination interval must actually be allocated and nonoverlapping, sized from the worst-case budget in [Calculate Capacity](../boot-time-mutation-and-ownership/libfdt-capacity-relocation-and-failure-atomicity.md#calculate-capacity). Do not merely write a larger totalsize into a header.

## Mutations

Common operations include:

- `fdt_setprop()` and typed helpers
- `fdt_appendprop()` where list semantics permit
- `fdt_add_subnode()`
- `fdt_delprop()` and `fdt_del_node()`
- phandle/property helper functions
- reservation updates

Every mutation needs:

- exact target re-lookup
- expected prior state
- validated input and encoded length
- checked return status
- defined behavior for `FDT_ERR_NOSPACE`, not found, exists, and bad value
- postcondition verification

A sequence of successful individual writes is not an atomic product transaction.

## Error Reporting

```c
ret = fdt_setprop_u32(fdt, node, "temperature", 1000 * maxc);
if (ret != 0) {
        log_error("critical trip update failed: %s",
                  fdt_strerror(ret));
        return ret;
}
```

Preserve the numeric error and context. Common classes include:

- bad magic/version/structure/layout
- truncated blob
- node/property not found
- node/property already exists
- invalid phandle/value
- no capacity
- bad overlay/fixup

Do not continue with a default after a mandatory mutation fails.

## Failure Atomicity, Overlays, And Packing

Multi-step product mutation follows the disposable-copy pattern in [Atomicity And Rollback](../boot-time-mutation-and-ownership/libfdt-capacity-relocation-and-failure-atomicity.md#atomicity-and-rollback); rollback by writing “old values” is fragile because later operations may have moved nodes or failed before logging prior state. `fdt_overlay_apply()` can invalidate both base and overlay on failure, and a zero return proves only structural success. Call `fdt_pack()` only at the end and treat all earlier pointers/offsets as stale afterward; define whether release hashes cover the packed blob, deterministic padding, or a container entry ([Packing And Hashing](../boot-time-mutation-and-ownership/libfdt-capacity-relocation-and-failure-atomicity.md#packing-and-hashing)).

## Concurrency And Ownership

`libfdt` does not provide a universal product lock. If several firmware tasks, boot stages, or kernel callers can mutate one buffer, define serialization and publication explicitly. Readers must never traverse a buffer while another context structurally changes it without an appropriate higher-level contract.

One stage should own each property mutation. Multiple writers through shared helpers remain multiple authorities unless policy says otherwise.

## Review Checklist

- Is the containing buffer length trusted before header-derived access?
- Is every return code checked according to that function's contract?
- Are cells endian-converted and grouped by bindings?
- Are strings bounded by property length?
- Are offsets/pointers reacquired after mutations?
- Is writable capacity real and nonoverlapping?
- Is multi-step failure handled by discarding a working copy?
- Are overlay failures treated as buffer invalidation?
- Is the final blob validated, packed at the correct time, and hashed consistently?
- Is concurrent access serialized?

## Authoritative References

- [Devicetree compiler and `libfdt` source](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [`libfdt` public header](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/tree/libfdt/libfdt.h)
- [`libfdt` error definitions](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/tree/libfdt/libfdt.h)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)

## Continue

Proceed to [Diagnostic Workflow, Semantic Diffing, And CI Evidence](diagnostic-workflow-semantic-diffing-and-ci-evidence.md).
