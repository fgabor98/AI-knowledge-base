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

A clock provider declares `#clock-cells`; a consumer lists clock specifiers:

```dts
clock_controller: clock-controller@8000 {
        compatible = "example,trainer-clocks";
        #clock-cells = <1>;
};

serial@1000 {
        clocks = <&clock_controller 3>,
                 <&clock_controller 7>;
        clock-names = "bus", "core";
};
```

The provider binding defines IDs 3 and 7. A shared `dt-bindings/clock/...h` header may expose readable constants:

```dts
clocks = <&clock_controller TRAINER_CLK_UART_BUS>,
         <&clock_controller TRAINER_CLK_UART_CORE>;
```

The macro improves readability but still compiles to provider-defined cells.

Clock relationships describe which signals feed the consumer. They do not by themselves define a desired rate, parent selection, or enable policy. Properties such as `assigned-clocks`, `assigned-clock-parents`, and `assigned-clock-rates` have distinct binding semantics and must be used deliberately.

## Reset Relationships

Reset controllers advertise `#reset-cells`:

```dts
reset_controller: reset-controller@9000 {
        compatible = "example,trainer-resets";
        #reset-cells = <1>;
};

serial@1000 {
        resets = <&reset_controller 2>;
        reset-names = "core";
};
```

The argument can identify a line, bank/bit pair, or composite reset control depending on the provider binding. It does not universally equal a register bit.

The Linux reset framework distinguishes exclusive, shared, and pulse/reset controls. Device Tree describes the hardware connection; the consumer driver and binding determine the safe operation model.

## Power-Domain Relationships

A provider can expose one or more power domains:

```dts
power_controller: power-controller@a000 {
        compatible = "example,trainer-power";
        #power-domain-cells = <1>;
};

serial@1000 {
        power-domains = <&power_controller 4>;
};
```

The specifier selects the provider-defined domain. Some providers use zero cells because the provider node represents one domain; others use multiple cells for domain and performance-state or hierarchy information if their binding defines it.

Power-domain membership is not the same as a regulator supply. A domain often controls shared isolation, clocks, or switches around several integrated blocks, while a regulator models an electrical supply rail.

## One Node, Several Provider Classes

Hardware can combine controls:

```dts
system_controller: system-controller@8000 {
        #clock-cells = <2>;
        #reset-cells = <1>;
        #power-domain-cells = <1>;
};

accelerator@4000 {
        clocks = <&system_controller 2 6>;
        resets = <&system_controller 9>;
        power-domains = <&system_controller 3>;
};
```

The clock tuple has two arguments, while reset and power-domain tuples have one. The same phandle target does not imply a shared namespace.

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

Consumer names connect binding entries to framework lookups:

```c
bus = devm_clk_get(dev, "bus");
rst = devm_reset_control_get_exclusive(dev, "core");
```

If a binding defines only one unnamed clock or reset, the driver may request by index or `NULL` name. Multiple resources should normally have explicit, stable semantic names.

Power domains are often attached by platform/framework code rather than manually looked up by an ordinary peripheral driver. A driver should follow its subsystem and binding model instead of parsing phandles ad hoc.

## Dependency Sequencing Is Not Encoded By Property Order

This source order:

```dts
resets = <&reset_controller 2>;
clocks = <&clock_controller 3>;
power-domains = <&power_controller 4>;
```

does not prescribe “reset, then clock, then power.” Property order is style. Frameworks, bindings, and drivers implement safe sequencing. If hardware requires a special sequence, model all hardware dependencies completely and use the appropriate subsystem rather than relying on DTS line order.

## Debugging Checklist

For a failed clock, reset, or domain:

1. inspect the runtime consumer property
2. resolve the phandle to the runtime provider
3. confirm the matching `#*-cells`
4. decode arguments using the provider binding/header
5. verify provider `status` and ancestors
6. confirm provider compatible and kernel configuration
7. inspect provider probe logs and framework debug data
8. confirm the consumer uses the binding-defined name
9. distinguish absent, invalid, deferred, and permission errors

## Common Errors

- Treating clock and reset IDs as one shared namespace.
- Assuming a numeric ID is a hardware register bit.
- Using clock assignment properties as a substitute for listing consumed clocks.
- Describing a voltage rail as a power domain or vice versa.
- Believing property order controls power-up sequencing.
- Adding delays in the consumer to hide a provider registration problem.
- Omitting a real reset because the bootloader happened to deassert it.

## Exercises

1. Decode three properties targeting one system controller with different cell counts.
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
