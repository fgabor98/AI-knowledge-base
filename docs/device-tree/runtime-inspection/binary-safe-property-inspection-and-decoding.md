---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Binary-Safe Property Inspection And Decoding

Files under the live Device Tree export contain raw property bytes. Text tools can hide NUL separators, byte order, embedded zeros, and empty booleans. Select the decoder from the binding, not from what the output happens to resemble.

## Classify Before Reading

For a target property, record:

```text
node path
property name
binding/schema
expected type and cardinality
parent #address-cells/#size-cells if relevant
provider #*-cells if phandle array
unit and valid range
```

Only then choose text, hex, or cell decoding.

## Single Strings And String Lists

Single string:

```bash
tr -d '\0' </sys/firmware/devicetree/base/model
printf '\n'
```

Ordered string list:

```bash
tr '\0' '\n' \
  </sys/firmware/devicetree/base/soc/device@48000000/compatible
```

Use newline replacement for lists. `tr -d '\0'` would concatenate `"acme,axc200"` and `"acme,axc100"` into a false single string.

Verify termination and hidden bytes when evidence matters:

```bash
hexdump -Cv /sys/firmware/devicetree/base/model
```

Do not use `echo $(cat property)`; command substitution drops NUL bytes and performs shell parsing/word splitting.

## Boolean Properties

A boolean is true by presence and normally has zero bytes:

```bash
prop=/sys/firmware/devicetree/base/soc/device@48000000/dma-coherent

if [[ -e "$prop" ]]; then
        printf '%s\n' 'dma-coherent present'
else
        printf '%s\n' 'dma-coherent absent'
fi
```

An empty read is not false; it is the encoding of a present boolean. Confirm the binding permits the property.

## 32-Bit Cells

DT cells are big-endian. Display exact bytes grouped in fours:

```bash
prop=/sys/firmware/devicetree/base/soc/device@48000000/reg
size=$(wc -c <"$prop")
printf 'length=%s\n' "$size"
xxd -p -c 4 "$prop"
```

Each eight-hex-digit line is one 32-bit cell in byte order. Avoid native-endian integer output from tools that silently interpret host byte order.

Where possible, capture/reconstruct a DTB and use:

```bash
fdtget -tx live.dtb /soc/device@48000000 reg
```

Then group cells by binding context.

## Multi-Cell Integers

For `#address-cells = <2>` and `#size-cells = <2>`:

```text
reg = <addr_hi addr_lo size_hi size_lo>

address = (addr_hi << 32) | addr_lo
size    = (size_hi << 32) | size_lo
```

Use overflow-safe arithmetic in scripts/programs. Shell arithmetic may be signed or width-limited. For large values, use a language/runtime with explicit integer handling and retain the raw cells in evidence.

Apply `ranges` translations at each parent bus. A correctly decoded child address is not automatically a CPU physical address.

## Byte Arrays

```bash
hexdump -Cv \
  /sys/firmware/devicetree/base/soc/ethernet@10000000/local-mac-address
```

For exact compact hex:

```bash
xxd -p \
  /sys/firmware/devicetree/base/soc/ethernet@10000000/local-mac-address
```

Do not interpret arbitrary byte arrays as text. Redact secrets and privacy-sensitive identifiers according to field-log policy.

## Phandle Arrays

For a consumer property such as `clocks`:

1. display raw cells in big-endian order
2. take the first cell as a phandle
3. find the provider node with matching `phandle`/`linux,phandle`
4. read provider `#clock-cells`
5. consume that many argument cells
6. repeat for the next entry
7. pair entries positionally with `clock-names`

Build a live diagnostic DTB and use `fdtget`/decompilation for practical phandle lookup. Numeric phandles are local to this runtime tree and are not stable release IDs.

Different providers in one property can use different argument counts.

## Interrupts Need Parent Context

For `interrupts`, determine effective `interrupt-parent` through inheritance, then use that controller's `#interrupt-cells` and binding. For `interrupts-extended`, each entry explicitly begins with a provider phandle and may have a different width.

Do not label the first integer “IRQ 42” until the controller binding says the specifier cell is a hardware interrupt number. Linux virtual IRQ numbers shown elsewhere can differ.

## Status And Availability

```bash
node=/sys/firmware/devicetree/base/soc/device@48000000

if [[ -e "$node/status" ]]; then
        tr -d '\0' <"$node/status"
        printf '\n'
else
        printf '%s\n' 'status absent (normally available under OF rules)'
fi
```

Linux's `of_device_is_available()` normally treats absent, `okay`, or `ok` as available and other values as unavailable. Availability still does not prove bus population or successful probe.

## Property Size Is Evidence

```bash
stat -c '%s' "$prop"
```

Check:

- string ends with NUL
- cell property length is divisible by four
- tuple count matches binding/provider
- byte array has exact expected length
- boolean is zero length

Malformed length can explain parser errors even when a hex dump looks plausible.

## Safe Collection Function Pattern

For field tooling, never embed unvalidated DT paths from external input. Use an allowlist of known paths/properties, resolve beneath the expected root, refuse symlink escape where relevant, and copy raw bytes before decoding.

Collect both:

```text
raw file hash/length/hex
decoded interpretation and binding used
```

This allows later correction if the first interpretation was wrong.

## Common Mistakes

- `cat`ting a string list and missing boundaries
- interpreting an empty boolean as absent/false
- decoding cells in host endianness
- treating every two-cell tuple the same
- confusing phandles with provider arguments
- using Linux IRQ numbers to reinterpret DT specifiers
- treating child bus address as final CPU address
- exposing seed or identity data in public logs
- guessing a property type without reading the binding

## Authoritative References

- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux Devicetree bindings](https://docs.kernel.org/devicetree/bindings/)

## Continue

Proceed to [Live-Tree Capture, Normalization, And Semantic Diffing](live-tree-capture-normalization-and-semantic-diffing.md).
