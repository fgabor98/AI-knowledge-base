---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Root Identity, Availability, And Aliases

The root node answers “what machine is this?”, `status` answers “may the operating system use this node?”, and `/aliases` provides short path names. They solve different problems and must not be treated as interchangeable identity mechanisms.

## Root Contract

The SolidRun HummingBoard-T root (`k3-am642-hummingboard-t.dts`, with cell counts inherited from `k3-am64.dtsi`):

```dts
/ {
        model = "SolidRun AM642 HummingBoard-T";
        compatible = "solidrun,am642-hummingboard-t", "solidrun,am642-sr-som", "ti,am642";
        #address-cells = <2>;
        #size-cells = <2>;
};
```

The root `compatible` is an ordered string list, from the most specific compatible implementation to broader compatible implementations. Linux uses it for early machine selection and platform matching. Here the list runs board, SoM, SoC. Add a fallback only when software written for that fallback can really operate the newer machine; a shared vendor or similar marketing name is not sufficient.

`model` is a human-readable product description. The PCIe and USB3 variants of the same board change only `model` ("... with PCI-E", "... with USB-3.1 Gen 1") and keep the compatible list, because the machine identity is unchanged; only the use of one SERDES lane differs. Changing `model` must not alter driver behavior.

The root cell-count properties govern addresses and sizes in direct children such as `/memory` and `/reserved-memory`. They are part of the encoding contract, not declarations of the CPU's register width.

## Designing A Compatible List

Use these questions during review:

1. Does the first string name the exact board or product revision whose integration differences matter?
2. Is every later string a real backward-compatible programming model?
3. Is the vendor prefix registered and consistently used?
4. Would removing a fallback expose a hidden software dependency?

Do not encode software versions, deployment roles, or serial numbers in `compatible`. Those do not describe a hardware programming interface.

## Availability Through `status`

`status` describes a node's operational availability. Important standard values include:

- `okay` or `ok`: operational and available
- `disabled`: not currently operational, but could become operational
- `reserved`: operational, but reserved for another software component
- `fail` or `fail-...`: a serious detected fault

If `status` is absent, the node is normally treated as available. Consequently, a SoC `.dtsi` declares integrated devices as `disabled`, and a board DTS enables only the wired instances. The AM642 SK uses three different values for UARTs and GPIO (`k3-am642-sk.dts`):

```dts
&main_uart0 {
        status = "okay";
};

&main_uart1 {
        /* main_uart1 is reserved for firmware usage */
        status = "reserved";
};

/* mcu_gpio0 and mcu_gpio_intr are reserved for mcu firmware usage */
&mcu_gpio0 {
        status = "reserved";
};
```

`reserved` tells Linux the hardware exists and is in use by another execution environment—here the R5F or M4F firmware—so Linux must not claim it.

Availability is effectively hierarchical during normal Linux platform population: a disabled parent is not instantiated and its children are not walked as available devices. In `k3-am64-main.dtsi` both the R5F cluster and its cores are disabled; `k3-am64-ti-ipc-firmware.dtsi` must enable both levels:

```dts
&main_r5fss0 {
        status = "okay";
};

&main_r5fss0_core0 {
        mboxes = <&mailbox0_cluster2 &mbox_main_r5fss0_core0>;
        memory-region = <&main_r5fss0_core0_dma_memory_region>,
                        <&main_r5fss0_core0_memory_region>;
        status = "okay";
};
```

Setting only `main_r5fss0_core0` to `okay` would not make the disabled `r5fss@78000000` cluster operational, so its driver would never populate the core.

Treat `status` as hardware/ownership policy, not as a convenient way to make a driver stop probing while hiding the actual defect. A node can be available but still fail because clocks, pins, power, or its driver are missing.

## `/aliases`: Short Paths, Not Source Labels

The `/aliases` node maps a name to an absolute node path. The AM642 SK, and an EVM overlay that writes a literal path:

```dts
/* k3-am642-sk.dts */
aliases {
        serial0 = &mcu_uart0;
        serial1 = &main_uart1;
        serial2 = &main_uart0;
        mmc0 = &sdhci0;
        mmc1 = &sdhci1;
        ethernet0 = &cpsw_port1;
};

/* k3-am642-evm-icssg1-dualemac.dtso */
aliases {
        ethernet1 = "/icssg1-eth/ethernet-ports/port@1";
};
```

Note that `serial2` is the SoC's `main_uart0`: alias numbers follow board and firmware conventions, not SoC instance numbers.

In DTS source, `&main_uart0` is resolved by the compiler to the full path stored in the property; the DTB does not preserve an alias-to-label relationship. [Tree Anatomy And Vocabulary](../foundations/tree-anatomy-and-vocabulary.md#aliases) compares labels, phandles, paths, and aliases.

Aliases are used by firmware and some Linux subsystems, and may influence instance numbering. That behavior is subsystem-specific. Do not promise that `/aliases` alone creates a permanent userspace device name; use an explicit userspace naming policy where a stable ABI is required.

Alias names contain lowercase letters, digits, and hyphens. Prefer conventional class-plus-number names such as `serial0`, `ethernet0`, and `i2c1` when the ecosystem defines them. Avoid aliases that merely duplicate every label without a consumer.

## Diagnostic Workflow

For the wrong machine, missing device, or unexpected numbering:

```sh
tr -d '\0' </sys/firmware/devicetree/base/compatible
tr -d '\0' </sys/firmware/devicetree/base/model
find /sys/firmware/devicetree/base/aliases -maxdepth 1 -type f -print
```

Then decompile the exact booted or deployed DTB and check:

- the ordered root compatible list
- the target node and every ancestor's `status`
- the alias property's final absolute path
- whether firmware selected a different DTB than the build intended

String-list properties contain NUL separators, so a plain `cat` can make multiple values look concatenated.

## Review Traps

- Adding a generic fallback merely to make existing software match.
- Using `model` in application logic.
- Enabling a leaf while leaving its parent bus disabled.
- Assuming an absent `status` means disabled.
- Treating a source label as runtime metadata.
- Renaming aliases without checking bootloader scripts and userspace expectations.

## Authoritative References

- [Devicetree Specification: root node and `/aliases`](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)
- [Devicetree Specification: `compatible`, `model`, and `status`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux DeviceTree kernel API](https://docs.kernel.org/devicetree/kernel-api.html)

## Next Step

Continue with [CPUs, Topology, And Memory](cpus-topology-and-memory.md).
