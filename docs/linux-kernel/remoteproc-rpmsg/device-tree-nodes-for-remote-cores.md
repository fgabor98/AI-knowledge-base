---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Device Tree Nodes For Remote Cores

## What Problem Does This Solve?

Remoteproc Device Tree nodes describe how Linux integrates a remote core into
the SoC. They are not only "firmware name" nodes. They connect the remoteproc
driver to memory, resets, clocks, power domains, mailboxes, interrupts, and
sometimes bootloader handoff state.

A broken remoteproc node can fail in many ways:

```text
node disabled
  -> no remoteproc device

wrong memory-region order
  -> firmware loads into wrong carveout

wrong mailbox channel
  -> RPMsg messages never arrive

missing power domain
  -> core never starts

wrong firmware-name
  -> request_firmware fails
```

The binding for the specific remoteproc driver is the source of truth. This
page teaches how to read and review those nodes.

## Node Responsibilities

A remote-core node may describe:

- compatible string for driver matching
- MMIO register regions
- remote core memory windows
- reserved memory carveouts
- firmware name
- resets and reset names
- clocks and clock names
- power domains
- mailbox channels
- interrupts
- IOMMU or DMA constraints
- boot method or attach/start policy
- vendor-specific integration details

Not every node has every property. Required and optional properties are binding
specific.

## Generic Shape

Example skeleton:

```dts
r5f0: r5f@41000000 {
    compatible = "vendor,soc-r5f";
    reg = <0x0 0x41000000 0x0 0x10000>;
    resets = <&reset 12>;
    reset-names = "core";
    clocks = <&clk 34>;
    clock-names = "core";
    power-domains = <&power 5>;
    mboxes = <&mailbox 0>, <&mailbox 1>;
    mbox-names = "tx", "rx";
    memory-region = <&r5f_code>, <&r5f_vring0>, <&r5f_vring1>, <&r5f_buffer>;
    firmware-name = "vendor/r5f0-fw.elf";
    status = "okay";
};
```

This is illustrative, not a binding. Real bindings define exact property names,
region order, and allowed values.

## `compatible`

The `compatible` property selects the remoteproc platform driver.

Example:

```dts
compatible = "vendor,soc-r5f";
```

Review:

- Is the compatible string documented in a YAML binding?
- Is the kernel driver enabled?
- Is the fallback compatible appropriate?
- Does the SoC variant match the actual hardware?

Wrong compatible strings often lead to missing devices or subtly wrong resource
handling.

## `reg`

`reg` describes MMIO regions or core-local memory windows, depending on binding.

Example:

```dts
reg = <0x0 0x41000000 0x0 0x10000>,
      <0x0 0x41010000 0x0 0x10000>;
reg-names = "control", "iram";
```

Review:

- Do unit address and `reg` match?
- Are address and size cells correct for the parent bus?
- Does `reg-names` order match the binding?
- Are memory windows separate from `/reserved-memory` carveouts?

Remoteproc drivers may use `reg` for control registers, local memories, or
mailbox/status blocks. Do not infer meaning without the binding.

## `memory-region`

`memory-region` connects a remoteproc node to `/reserved-memory` regions.

Reserved memory:

```dts
reserved-memory {
    #address-cells = <2>;
    #size-cells = <2>;
    ranges;

    r5f_code: r5f-code@9c000000 {
        reg = <0x0 0x9c000000 0x0 0x100000>;
        no-map;
    };

    r5f_vring0: r5f-vring0@9c100000 {
        reg = <0x0 0x9c100000 0x0 0x4000>;
        no-map;
    };

    r5f_vring1: r5f-vring1@9c104000 {
        reg = <0x0 0x9c104000 0x0 0x4000>;
        no-map;
    };

    r5f_buffer: r5f-buffer@9c108000 {
        compatible = "shared-dma-pool";
        reg = <0x0 0x9c108000 0x0 0x100000>;
        no-map;
    };
};
```

Consumer:

```dts
memory-region = <&r5f_code>, <&r5f_vring0>, <&r5f_vring1>, <&r5f_buffer>;
memory-region-names = "code", "vring0", "vring1", "buffer";
```

Use `memory-region-names` only when the binding defines or accepts it. Some
remoteproc bindings rely on strict phandle order instead.

Review:

- Do all phandles resolve?
- Are regions large enough?
- Is order correct?
- Are names correct if used?
- Are regions `no-map` or `reusable` according to binding?
- Do addresses match firmware linker/resource table expectations?

## `firmware-name`

Some remoteproc bindings allow:

```dts
firmware-name = "vendor/r5f0-fw.elf";
```

Review:

- Does the binding define this property?
- Is the file installed under `/lib/firmware/vendor/r5f0-fw.elf`?
- Is the firmware available early enough?
- Is the name product/board specific enough?
- Is the update flow clear?

If a driver uses a built-in default firmware name, Device Tree may not need this
property.

## Mailboxes

Remote cores commonly use mailbox channels for notifications.

Example:

```dts
mboxes = <&mailbox0 2>, <&mailbox0 3>;
mbox-names = "tx", "rx";
```

Review:

- Does the mailbox provider probe?
- Are channel indices correct?
- Do `mbox-names` match the binding and driver?
- Which direction is each channel?
- Are mailbox interrupts routed and powered?
- Does firmware expect the same notify IDs?

RPMsg failures with a running remote core often come from mailbox mistakes.

## Interrupts

Some remoteproc drivers use explicit interrupts in addition to or instead of
mailboxes.

Example:

```dts
interrupt-parent = <&gic>;
interrupts = <GIC_SPI 120 IRQ_TYPE_LEVEL_HIGH>;
```

Review:

- Is the interrupt controller correct?
- Is the trigger type correct?
- Is the interrupt wake-capable if needed?
- Is the interrupt shared with another block?
- Does the remote core clear the source or does Linux?

Do not guess interrupt flags. Use the SoC binding, interrupt controller binding,
and firmware expectation.

## Resets

Remoteproc drivers often need resets:

```dts
resets = <&reset 12>, <&reset 13>;
reset-names = "core", "bus";
```

Review:

- Are reset lines exclusive or shared?
- Does the driver support all listed reset names?
- Is reset controlled by Linux, bootloader, or secure firmware?
- Does attach mode require avoiding reset assertion?

Reset handling is especially dangerous for bootloader-started or safety-related
cores.

## Clocks And Power Domains

Remote cores may need clocks and power domains:

```dts
clocks = <&clk 34>, <&clk 35>;
clock-names = "core", "bus";
power-domains = <&power 5>;
```

Review:

- Are required providers enabled?
- Are clock names in binding order?
- Does power domain remain on for attached cores?
- Can the core run during Linux suspend?
- Are mailbox and shared-memory paths in retained domains?

Power-domain errors can look like firmware crashes or mailbox timeouts.

## Child Nodes And Core Clusters

Some SoCs describe a remote subsystem as a parent node with child core nodes.

Conceptual example:

```dts
r5fss@41000000 {
    compatible = "vendor,soc-r5fss";
    power-domains = <&power 5>;

    r5f0: r5f@0 {
        compatible = "vendor,soc-r5f";
        reg = <0>;
        memory-region = <&r5f0_code>, <&r5f0_buffer>;
        firmware-name = "vendor/r5f0.elf";
    };

    r5f1: r5f@1 {
        compatible = "vendor,soc-r5f";
        reg = <1>;
        memory-region = <&r5f1_code>, <&r5f1_buffer>;
        firmware-name = "vendor/r5f1.elf";
    };
};
```

Bindings may model lockstep/split mode, shared TCMs, or cluster-level controls.
Do not flatten or split nodes without following the binding.

## Boot Mode And Attach Policy

Some bindings include properties that describe whether Linux should start,
attach to, or leave a core alone. Names are platform-specific.

Review questions:

- Does the binding distinguish split mode and lockstep mode?
- Does the node describe a core already started by bootloader?
- Is the firmware memory reserved even if Linux only attaches?
- Does the driver support detach?
- Is the remote core safe to stop from sysfs?

Ownership policy must match bootloader and product design.

## Binding Validation

Run binding checks when working in a kernel tree:

```sh
make dt_binding_check DT_SCHEMA_FILES=remoteproc
make dtbs_check
```

For a specific binding file:

```sh
make dt_binding_check DT_SCHEMA_FILES=Documentation/devicetree/bindings/remoteproc/vendor,soc-r5f.yaml
```

Exact paths and make targets depend on the kernel tree and installed schema
tools.

Binding validation catches:

- undocumented properties
- missing required properties
- wrong value type
- wrong number of phandles
- invalid `reg` shape
- invalid child node names

It does not prove the firmware, memory map, or mailbox protocol is correct.

## Runtime Inspection

Dump the running Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
```

Search remoteproc-related properties:

```sh
rg -n 'remoteproc|rproc|memory-region|firmware-name|mbox|mailbox|power-domains|resets' /tmp/running.dts
```

Check remoteproc devices:

```sh
ls /sys/class/remoteproc
cat /sys/class/remoteproc/remoteproc*/name
cat /sys/class/remoteproc/remoteproc*/state
```

Check provider state:

```sh
dmesg | grep -Ei 'defer|mailbox|reset|clock|power domain|remoteproc'
sudo cat /sys/kernel/debug/clk/clk_summary
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary
```

## Review Workflow

When reviewing a remoteproc node:

1. Open the YAML binding for the compatible string.
2. Verify `status = "okay"` only on hardware that is present and intended to be
   used.
3. Check every provider phandle: clocks, resets, power domains, mailboxes,
   interrupts, memory regions.
4. Map every memory region to address, size, owner, and firmware use.
5. Compare firmware name with rootfs package.
6. Compare mailbox names and channel IDs with firmware.
7. Determine lifecycle ownership: Linux start, attach, or firmware-owned.
8. Run `dtbs_check`.
9. Boot and compare runtime DTB with source DTS.
10. Start the core and verify logs, RPMsg devices, and trace buffers.

## Common Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| node disabled | no remoteproc instance | set `status = "okay"` for intended core |
| wrong compatible | driver does not bind or handles resources wrongly | use binding-documented string |
| missing memory region | firmware load or RPMsg setup fails | add required reserved-memory phandle |
| memory-region order wrong | wrong carveout assignment | follow binding order/names |
| firmware-name mismatch | request_firmware failure | align DTS and rootfs package |
| mailbox direction swapped | one-way or no RPMsg traffic | fix `mboxes`/`mbox-names` |
| reset asserted in attach mode | bootloader-started firmware dies | use correct ownership/mode property |
| power domain missing | start timeout or crash | add provider reference |
| unvalidated vendor property | future kernel rejects or ignores it | document in binding |

## Practice Exercises

1. Find a remoteproc node in a board DTS and open its YAML binding.
2. Draw the phandle graph for memory regions, mailboxes, resets, clocks, and
   power domains.
3. Dump the running DTB and compare the remoteproc node with source DTS.
4. Run `dtbs_check` for the board and record any remoteproc-related warnings.
5. Break one mailbox phandle in a lab DTS and observe the probe or RPMsg
   failure.

## Debugging Checklist

- Does the compatible string match a remoteproc driver?
- Is the node enabled intentionally?
- Do all provider phandles resolve?
- Are memory regions correct in address, size, order, and attributes?
- Does firmware name match the installed file?
- Are mailbox and interrupt routes correct?
- Are resets safe for the ownership model?
- Are clocks and power domains sufficient for start, runtime, and suspend?
- Does binding validation pass?
- Does runtime Device Tree match the source you think you booted?

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [Reserved Memory](reserved-memory.md)
- [Device Tree Binding Validation](../../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)

## Official References

- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Reserved Memory Binding](https://docs.kernel.org/devicetree/bindings/reserved-memory/reserved-memory.yaml)
- [Mailbox Framework](https://docs.kernel.org/driver-api/mailbox.html)
- [Devicetree Bindings Index](https://docs.kernel.org/devicetree/bindings/)
