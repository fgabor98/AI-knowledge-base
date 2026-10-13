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
  </sys/firmware/devicetree/base/bus@f4000/serial@2800000/compatible
```

Use newline replacement for lists. `tr -d '\0'` would concatenate `"ti,am64-uart"` and `"ti,am654-uart"` into the false single string `ti,am64-uartti,am654-uart`.

Verify termination and hidden bytes when evidence matters:

```bash
hexdump -Cv /sys/firmware/devicetree/base/model
```

Do not use `echo $(cat property)`; command substitution drops NUL bytes and performs shell parsing/word splitting.

## Boolean Properties

A boolean is true by presence and normally has zero bytes:

```bash
prop=/sys/firmware/devicetree/base/bus@f4000/mmc@fa10000/non-removable

if [[ -e "$prop" ]]; then
        printf '%s\n' 'non-removable present'
else
        printf '%s\n' 'non-removable absent'
fi
```

An empty read is not false; it is the encoding of a present boolean. Confirm the binding permits the property.

## 32-Bit Cells

DT cells are big-endian. Display exact bytes grouped in fours:

```bash
prop=/sys/firmware/devicetree/base/bus@f4000/mmc@fa00000/reg
size=$(wc -c <"$prop")
printf 'length=%s\n' "$size"
xxd -p -c 4 "$prop"
```

Each eight-hex-digit line is one 32-bit cell in byte order. Avoid native-endian integer output from tools that silently interpret host byte order.

Where possible, capture/reconstruct a DTB and use:

```bash
fdtget -tx live.dtb /bus@f4000/mmc@fa00000 reg     # 0 fa00000 0 260 0 fa08000 0 134
```

Then group cells by binding context.

## Multi-Cell Integers

Combine multi-cell addresses and sizes as described in [Decode Cells Mechanically](../addressing-and-bus-modeling/cell-counts-reg-and-unit-addresses.md#decode-cells-mechanically). Shell arithmetic may be signed or width-limited, so use a language with explicit integer handling and retain the raw cells in evidence. A decoded child address is not a CPU physical address until every parent `ranges` is applied.

## Byte Arrays

```bash
hexdump -Cv \
  /sys/firmware/devicetree/base/bus@f4000/ethernet@8000000/ethernet-ports/port@1/mac-address
```

For exact compact hex:

```bash
xxd -p \
  /sys/firmware/devicetree/base/bus@f4000/ethernet@8000000/ethernet-ports/port@1/mac-address
```

Do not interpret arbitrary byte arrays as text. Redact secrets and privacy-sensitive identifiers according to field-log policy.

## Phandle Arrays And Interrupts

Display raw cells in big-endian order, then apply the [standard decoding algorithm](../provider-consumer-relationships.md#the-core-decoding-algorithm), matching each phandle against provider `phandle`/`linux,phandle` properties. For `interrupts`, first find the effective `interrupt-parent` through inheritance. Building a live diagnostic DTB and using `fdtget` or decompilation makes phandle lookup practical. Numeric phandles are local to this runtime tree, and the first interrupt cell is not “IRQ 42” until the controller binding says so.

## Status And Availability

```bash
node=/sys/firmware/devicetree/base/bus@f4000/mmc@fa00000

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
