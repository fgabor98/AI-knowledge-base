---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Compatible Contracts And Fallback Chains

A compatible string names a hardware programming-model contract. An ordered list says that the node is best described by the first string and can also be driven as each later, less-specific implementation.

## Ordered From Specific To General

Every AM64x UART carries two strings (`k3-am64-main.dtsi`):

```dts
main_uart0: serial@2800000 {
        compatible = "ti,am64-uart", "ti,am654-uart";
        reg = <0x00 0x02800000 0x00 0x100>;
        /* ... */
};
```

This asserts more than family resemblance. It says software implementing the `ti,am654-uart` contract—introduced for the first K3 SoC, AM65x—can safely operate the AM64x UART. Mainline's `8250_omap` driver relies on exactly that promise: its match table has no `ti,am64-uart` entry at all, so every AM64x UART binds through the fallback.

The first entry should identify the actual implementation, even when no driver matches it today; it lets a future kernel add an AM64x-specific quirk without changing deployed DTBs. Later entries are fallbacks in decreasing specificity. The AM64x I2C controllers follow the same pattern with `"ti,am64-i2c", "ti,omap4-i2c"`, binding through the OMAP4 entry of `i2c-omap`.

## What Compatibility Must Cover

A safe fallback normally preserves everything old software relies on:

- register layout, width, access semantics, and reset values used by the old driver
- interrupt behavior and acknowledgement sequence
- clock, reset, power, DMA, and PHY expectations
- required initialization and ordering constraints
- old binding properties and their meanings
- behavior when new features are ignored

Matching register offsets alone is insufficient. If the new device needs an additional reset sequence, has inverted interrupt semantics, or corrupts data unless a new quirk is applied, the old driver may not be safe.

## Exact Strings, Not Patterns

Compatible strings are literal values conventionally written as `vendor,device`. They do not support wildcards such as `ti,am6*-uart`, semantic-version comparisons, or substring matching. The vendor prefix must be registered in `Documentation/devicetree/bindings/vendor-prefixes.yaml`.

Use hardware identity, not deployment or software policy:

```text
good: ti,am64-uart                 (SoC integration of an IP block)
good: ti,am654-uart                (programming model introduced on AM65x)
bad:  ti,production-uart           (invented: deployment role)
bad:  ti,linux-6-12-uart           (invented: software version)
bad:  ti,uart-driver-v3            (invented: driver identity)
```

The contract must remain meaningful to other operating systems and future kernels; U-Boot's K3 UART support reads the same strings.

## Root Versus Device Compatible

The root node identifies the complete machine; a peripheral node identifies one hardware block. From the PHYTEC phyBOARD-Electra and the AM64x SoC files:

```dts
/ {
        compatible = "phytec,am642-phyboard-electra-rdk",
                     "phytec,am64-phycore-som", "ti,am642";
};

main_r5fss0: r5fss@78000000 {
        compatible = "ti,am64-r5fss";
};
```

The same list principles apply, but consumers differ. Root compatibles can select early platform behavior and describe board/SoC compatibility; device compatibles normally match a bus driver and binding. Do not copy a SoC root compatible onto each peripheral or put a peripheral IP compatible in the root list.

A board fallback to a SoC string is appropriate only where the architecture ecosystem defines that relationship and generic SoC support can boot the board safely. Board identity and SoC identity are not automatically interchangeable.

## How Linux Uses A List

Linux OF matching compares a node's compatible strings with a driver's `struct of_device_id` table. The OF matching helpers prefer a table entry matching an earlier, more-specific string in the node's list.

That preference operates within a candidate match table. It is not a global arbitration system for competing drivers. The driver core checks registered drivers according to the bus model; two unrelated drivers should not claim overlapping compatible contracts and expect list specificity to choose the intended one.

## Fallbacks Enable Old Software

`8250_omap` attaches different quirk data to different contracts (`drivers/tty/serial/8250/8250_omap.c`):

```c
static const struct of_device_id omap8250_dt_ids[] = {
        { .compatible = "ti,am654-uart", .data = &am654_platdata, },
        { .compatible = "ti,omap2-uart" },
        { .compatible = "ti,omap3-uart" },
        { .compatible = "ti,omap4-uart", .data = &omap4_platdata, },
        { .compatible = "ti,am3352-uart", .data = &am33xx_platdata, },
        { .compatible = "ti,am4372-uart", .data = &am33xx_platdata, },
        { .compatible = "ti,dra742-uart", .data = &omap4_platdata, },
        {},
};
```

The AM64x node's fallback lets this table bind with `am654_platdata`. If AM64x ever needed different behavior, a kernel could add `{ .compatible = "ti,am64-uart", .data = &am64_platdata }`, and every deployed AM64x DTB would pick it up without modification because the specific string was there from the start.

This only works if AM654 behavior is a safe subset. Note what the K3 UART did **not** do: it does not list `ti,omap4-uart` as a further fallback, even though the IP is an OMAP-family 8250. `am654_platdata` enables `UART_HAS_EFR2`, `UART_HAS_RHR_IT_DIS`, and `UART_RX_TIMEOUT_QUIRK`, behavior the OMAP4 path would not apply. A fallback must never be added merely to make an old kernel bind; an explicit non-match is safer than silent misprogramming.

## Generic Compatibles

Some standardized devices have legitimate generic contracts defined by their bindings. The AM642 SK uses `fixed-clock` (`serdes_refclk`), `regulator-fixed`, `gpio-leds`, and `simple-bus`. Generic strings can also appear as fallbacks after a specific one; the AM64x control-module node reads:

```dts
main_conf: syscon@43000000 {
        compatible = "ti,j721e-system-controller", "syscon", "simple-mfd";
};
```

`syscon` and `simple-mfd` describe how the block is accessed and populated, while the specific string documents the actual programming model. Do not invent a generic value such as `generic-uart`, or use `syscon` alone to avoid documenting the real block.

Generic fallbacks can constrain future evolution. Once deployed, old software may bind on that promise. Review them as long-lived ABI, not as convenience labels.

## Review Method

For each list:

1. Find the schema that permits the exact sequence.
2. Identify the real hardware named by the first string.
3. For every fallback, name the old driver behavior expected to work and compare registers, required resources, errata, and initialization ordering.
4. Test both update directions (see [Compatibility In Both Directions](compatible-evolution-and-stable-abi-decisions.md#compatibility-in-both-directions)).
5. Remove any fallback whose safety cannot be demonstrated.

## Common Failures

- Listing sibling devices that share a vendor but are not backward-compatible.
- Ordering a generic fallback before the actual implementation.
- Adding a fallback solely to suppress “no driver” symptoms.
- Treating marketing family names as programming contracts.
- Using a software version or board role in a compatible string.
- Allowing two drivers to claim the same compatible and relying on registration order.

## Authoritative References

- [Devicetree Specification: `compatible`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux binding design guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux DeviceTree matching APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)

## Next Step

Continue with [Compatible Evolution And Stable ABI Decisions](compatible-evolution-and-stable-abi-decisions.md).
