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

A typical root begins as follows:

```dts
/ {
        model = "Example Systems Atlas Controller Rev C";
        compatible = "example,atlas-revc", "example,atlas";
        #address-cells = <2>;
        #size-cells = <2>;
};
```

The root `compatible` is an ordered string list, from the most specific compatible implementation to broader compatible implementations. Linux uses it for early machine selection and platform matching. Add a fallback only when software written for that fallback can really operate the newer machine; a shared vendor or similar marketing name is not sufficient.

`model` is a human-readable product description. It is valuable in logs and inventory, but it is not a substitute for a stable compatible string. Changing punctuation in `model` should not alter driver behavior.

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

If `status` is absent, the node is normally treated as available. Consequently, a SoC `.dtsi` often declares integrated devices as `disabled`, and a board DTS enables only the wired instances:

```dts
&uart2 {
        status = "okay";
};
```

Availability is effectively hierarchical during normal Linux platform population: a disabled bus is not instantiated and its children are not walked as available platform devices. Setting a child to `okay` does not make a disabled parent bus operational:

```dts
soc {
        status = "disabled";

        serial@1000 {
                status = "okay"; /* Cannot make the disabled ancestor available. */
        };
};
```

Treat `status` as hardware/ownership policy, not as a convenient way to make a driver stop probing while hiding the actual defect. A node can be available but still fail because clocks, pins, power, or its driver are missing.

## `/aliases`: Short Paths, Not Source Labels

The `/aliases` node maps a name to an absolute node path:

```dts
/ {
        aliases {
                serial0 = &uart2;
                ethernet0 = "/soc/ethernet@30000000";
        };
};
```

In DTS source, `&uart2` is resolved by the compiler to the full path stored in the property. The resulting DTB does not preserve that expression as an alias-to-label relationship.

Keep the concepts separate:

| Construct | Scope | Purpose |
|---|---|---|
| `uart2:` label | source composition | target a node while compiling |
| phandle | compiled tree | identify a node numerically |
| `/soc/serial@...` path | tree structure | identify a node by location |
| `serial0` alias | logical tree | provide a short, named path |

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
