---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Clocks, Resets, And Power Domains

These resources frequently come from one SoC system controller, but they remain separate provider domains with separate bindings, cell counts, frameworks, and lifecycles.

## Clock Relationships

A clock provider declares `#clock-cells`; a consumer lists clock specifiers. On AM64x almost every clock comes from the TI-SCI firmware provider (`k3-am64-main.dtsi`):

```dts
icssg0: icssg@30000000 {
        clocks = <&k3_clks 81 0>,  /* icssg0_core_clk */
                 <&k3_clks 81 3>,  /* icssg0_iep_clk */
                 <&k3_clks 81 16>, /* icssg0_rgmii_mhz_250_clk */
                 /* ... */
                 <&k3_clks 81 20>; /* icssg0_iclk */
        assigned-clocks = <&k3_clks 81 0>;
        assigned-clock-parents = <&k3_clks 81 2>;
};
```

The provider binding (`ti,k2g-sci-clk`) defines the cells as TI-SCI device ID 81 and clock IDs 0, 3, 16, …; the IDs themselves come from the SoC's TI-SCI firmware documentation. K3 sources use raw numbers with comments. Other SoC families provide headers under `include/dt-bindings/clock/` with named constants; either way the source compiles to provider-defined cells.

Clock relationships describe which signals feed the consumer. They do not by themselves define a desired rate, parent selection, or enable policy. The `assigned-clocks`/`assigned-clock-parents` pair above, and `assigned-clock-rates = <60000000>;` on the AM64x ADC (`tscadc0`), have distinct binding semantics; see [Clock Trees, Consumers, And Assignments](../clocks-resets-regulators-and-power/clock-trees-consumers-and-assignments.md#consumption-versus-assignment).

## Reset Relationships

Reset controllers advertise `#reset-cells`. AM64x has both a firmware reset provider and a local one in the SERDES wrapper:

```dts
k3_reset: reset-controller {
        compatible = "ti,sci-reset";
        #reset-cells = <2>;
};

serdes_wiz0: wiz@f000000 {
        #reset-cells = <1>;

        serdes0: serdes@f000000 {
                resets = <&serdes_wiz0 0>;
                reset-names = "torrent_reset";
        };
};

main_r5fss0_core0: r5f@78000000 {
        resets = <&k3_reset 121 1>;
};
```

For `k3_reset` the arguments are a TI-SCI device ID and a reset mask; for the WIZ wrapper a single local reset index. Neither universally equals a register bit.

The Linux reset framework distinguishes exclusive, shared, and pulse/reset controls. Device Tree describes the hardware connection; the consumer driver and binding determine the safe operation model.

## Power-Domain Relationships

`k3_pds` uses two cells: a TI-SCI device ID and an ownership flag defined in `dt-bindings/soc/ti,sci_pm_domain.h`:

```dts
main_uart0: serial@2800000 {
        power-domains = <&k3_pds 146 TI_SCI_PD_EXCLUSIVE>;
};

crypto: crypto@40900000 {
        power-domains = <&k3_pds 133 TI_SCI_PD_SHARED>;
};
```

The SA2UL crypto engine is marked shared because system firmware and OP-TEE also use it; upstream commit `49611f43e035` (“Do not exclusively claim SA2UL”, AM65x) explains that an exclusive request from Linux fails, and AM64x took the shared flag from the start. Other providers use zero cells because the provider node represents one domain, or one cell for a domain index.

Power-domain membership is not the same as a regulator supply. A domain often controls shared isolation, clocks, or switches around several integrated blocks, while a regulator models an electrical supply rail.

## Acquisition And Probe Ordering

A consumer may fail or defer when:

- the provider node is absent or disabled
- its parent bus is unavailable
- the provider driver is not enabled
- the provider driver has not registered its clocks, resets, or domains yet
- a specifier references an unimplemented output
- the consumer property violates its binding

Linux can infer supplier links from many firmware relationships, but `-EPROBE_DEFER` remains a normal signal that a required supplier is expected later. Repeated permanent deferral requires tracing the supplier rather than increasing probe priority.

## Names And Driver APIs

Clock and reset lookups use the consumer names described in [Specifier Decoding And Resource Names](specifier-decoding-and-resource-names.md#names-and-linux-lookup). If a binding defines only one unnamed clock or reset, the driver may request by index or `NULL` name.

Power domains are often attached by platform/framework code rather than looked up by the peripheral driver. A driver should follow its subsystem and binding model instead of parsing phandles ad hoc.

## Dependency Sequencing Is Not Encoded By Property Order

The AM64x R5F cluster places `power-domains = <&k3_pds 119 TI_SCI_PD_EXCLUSIVE>;` on the `r5fss` node and `resets = <&k3_reset 121 1>;` on each core node. Neither the node structure nor property order prescribes “power, then reset.” Property order is style. Frameworks, bindings, and drivers (here `ti_k3_r5_remoteproc.c` together with TI-SCI firmware) implement safe sequencing. If hardware requires a special sequence, model all hardware dependencies completely and use the appropriate subsystem rather than relying on DTS line order.

## Debugging Checklist

For a failed clock, reset, or domain:

1. decode the runtime consumer entry with the [decoding worksheet](specifier-decoding-and-resource-names.md#a-mechanical-decoding-worksheet)
2. verify provider `status`, ancestors, compatible, and kernel configuration
3. inspect provider probe logs and framework debug data
4. confirm the consumer uses the binding-defined name
5. distinguish absent, invalid, deferred, and permission errors

## Common Errors

- Treating clock and reset IDs as one shared namespace.
- Assuming a numeric ID is a hardware register bit.
- Using clock assignment properties as a substitute for listing consumed clocks.
- Describing a voltage rail as a power domain or vice versa.
- Believing property order controls power-up sequencing.
- Adding delays in the consumer to hide a provider registration problem.
- Omitting a real reset because the bootloader happened to deassert it.

## Exercises

1. Decode `clocks`, `resets`, and `power-domains` entries that all target children of the AM64x `dmsc` node, and explain what the second cell means in each.
2. Explain why `assigned-clock-rates` is not equivalent to `clocks`.
3. Distinguish a reset line, reset control, and reset provider.
4. List evidence that separates disabled provider from unregistered provider.
5. Explain why a consumer must not rely on bootloader clock state.

## References And Next Step

- [Linux Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)
- [Linux reset controller API](https://docs.kernel.org/driver-api/reset.html)
- [Linux device power-management domains](https://docs.kernel.org/driver-api/pm/devices.html#device-power-management-domains)
- [Linux binding design guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)

Continue with [GPIO And Interrupt Relationships](gpio-and-interrupt-relationships.md).
