---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Compatible Evolution And Stable ABI Decisions

Device Trees and kernels are often updated independently. Compatible-string design must let a new kernel interpret old DTBs correctly and prevent an old kernel from driving new hardware unsafely.

## Start From Observable Hardware Behavior

Introduce a new compatible when software must distinguish an implementation to operate it correctly or expose its hardware accurately. TI's K3 R5F subsystem is a real case: the same Cortex-R5F cluster IP got `ti,am654-r5fss`, `ti,j721e-r5fss`, `ti,j7200-r5fss`, `ti,am64-r5fss`, `ti,am62-r5fss`, and `ti,j721s2-r5fss` because the integrations differ in TCM size behavior, ECC auto-initialization, and supported cluster modes (AM64x adds a Single-CPU mode, AM62x has one core). Typical reasons include:

- changed register layout or field semantics
- new mandatory clock, reset, regulator, PHY, or power sequence
- an erratum requiring a distinct quirk
- changed interrupt, DMA, coherency, or endianness behavior
- feature/capability differences not safely discoverable at runtime
- incompatible binding property requirements

Do not introduce one solely because:

- the Linux driver was refactored
- the board DTS moved files
- the product acquired a new marketing name
- firmware or kernel versions changed
- a feature already discoverable from hardware is now supported by software

## Decision Matrix

| Hardware change | Compatible strategy | Fallback? |
|---|---|---|
| no software-visible difference | reuse existing compatible | not applicable |
| strict superset; old programming path remains safe | new specific compatible | old compatible may follow |
| new optional feature, safe when ignored | often new specific compatible or existing feature property, per binding | only if old behavior remains safe |
| mandatory sequence or semantic change | new compatible | generally no old fallback |
| new erratum old driver cannot avoid | new compatible | no unsafe fallback |
| board revision changes only wiring described by existing properties | update board/root identity as needed; peripheral compatible may stay | depends on whole-board compatibility |

The binding maintainer decides whether a feature belongs in compatible-specific match data, a discoverable capability, or a hardware property. Avoid properties that merely select driver code paths such as `vendor,use-new-driver`.

## Compatibility In Both Directions

Test four combinations:

| DTB | Kernel | Question |
|---|---|---|
| old | old | Does the released baseline still work? |
| old | new | Did the new driver preserve the old binding contract? |
| new | new | Are new features and requirements represented correctly? |
| new | old | Does a fallback provide safe reduced functionality, or correctly avoid binding? |

The last case determines whether a fallback is valid. “Probe runs” is not a sufficient result; exercise interrupt load, DMA, suspend/resume, error recovery, and reset paths affected by the change.

## Schema-Compatible Lists

A schema should constrain valid sequences explicitly. The K3 UART entries in `Documentation/devicetree/bindings/serial/8250_omap.yaml`:

```yaml
properties:
  compatible:
    oneOf:
      - enum:
          - ti,am3352-uart
          - ti,am4372-uart
          - ti,am654-uart
          - ti,dra742-uart
          - ti,omap2-uart
          - ti,omap3-uart
          - ti,omap4-uart
      - items:
          - enum:
              - ti,am64-uart
              - ti,j721e-uart
          - const: ti,am654-uart
```

This documents that AM64x and J721E UARTs must fall back to `ti,am654-uart`, and rejects a reversed list or `"ti,am64-uart"` alone. When variants have different requirements, conditional schema rules make those differences testable. `ti,k3-r5f-rproc.yaml` restricts the cluster mode per SoC:

```yaml
allOf:
  - if:
      properties:
        compatible:
          enum:
            - ti,am64-r5fss
    then:
      properties:
        ti,cluster-mode:
          enum: [0, 2]

  - if:
      properties:
        compatible:
          enum:
            - ti,am654-r5fss
            - ti,j7200-r5fss
            - ti,j721e-r5fss
            - ti,j721s2-r5fss
    then:
      properties:
        ti,cluster-mode:
          enum: [0, 1]
```

Only add a fallback if it remains safe without the new kernel understanding the new requirements. The R5F compatibles have no fallbacks to each other: a driver that believed an AM64x cluster was an AM65x one would try LockStep mode (`1`), which AM64x does not have.

## Match Data Versus DT Properties

Driver match data is appropriate for facts implied by the compatible. `drivers/remoteproc/ti_k3_r5_remoteproc.c`:

```c
struct k3_r5_soc_data {
        bool tcm_is_double;
        bool tcm_ecc_autoinit;
        bool single_cpu_mode;
        bool is_single_core;
        /* ... */
};

static const struct k3_r5_soc_data am64_soc_data = {
        .tcm_is_double = true,
        .tcm_ecc_autoinit = true,
        .single_cpu_mode = true,
        .is_single_core = false,
        /* ... */
};
```

A DT property is appropriate for board- or instance-specific configuration not implied by the SoC. `ti,cluster-mode` is that kind of property: whether a given device runs the AM64x cluster in Split (`0`) or Single-CPU (`2`) mode is per-device configuration (the driver's default comment ties it to the “most common efuse configurations”), while whether Single-CPU mode exists at all is SoC data. The probe function combines both and rejects impossible combinations:

```c
if  ((cluster->mode == CLUSTER_MODE_SINGLECPU && !data->single_cpu_mode) ||
     (cluster->mode == CLUSTER_MODE_SINGLECORE && !data->is_single_core))
        return dev_err_probe(dev, -EINVAL,
                             "Cluster mode = %d is not supported on this SoC\n",
                             cluster->mode);
```

Do not duplicate the same truth in both match data and an unconstrained property. Conflicting sources lead to ambiguous ownership and unsafe defaults.

## Root And Board Evolution

Board revisions need a new root compatible when software may need to distinguish them, even if most peripheral nodes are unchanged. A revision-specific first string can fall back to the earlier board only when old board-level behavior remains safe.

Examples requiring care include:

- power sequencing changed outside a peripheral's own binding
- boot storage or console moved
- GPIO polarity or regulator topology changed
- a secure-world interface or reserved-memory layout changed
- an expansion connector changed electrical capability

Existing properties may fully describe some changes, allowing common drivers without a new peripheral compatible. Root identity can still change for inventory, firmware policy, or early platform behavior.

## Deprecation And Migration

Once deployed, a misspelled or poorly designed compatible or property may still be ABI. `8250_omap.yaml` still documents `ti,hwmods` with `deprecated: true` and the note that it “should not be used on other platforms,” because old OMAP2/3 DTBs carry it. Silently changing a string in DTS and driver together passes same-tree testing but breaks independent upgrades. [Backward Compatibility, Deprecation, And Migration](../binding-design-and-stable-abi/backward-compatibility-deprecation-and-migration.md#incorrect-compatible) gives the migration procedure.

## Senior Review Questions

- What software-visible fact makes the new identity necessary?
- Is that fact per-implementation or per-board instance?
- Can hardware report it reliably without firmware data?
- What exact reduced behavior will an old fallback driver use?
- Which old-DTB/new-kernel and new-DTB/old-kernel combinations are supported?
- Does schema encode valid list order and variant requirements?
- Are bootloader-owned DTBs and field-upgrade sequencing included in testing?
- What is the lifetime commitment for already shipped strings?

## Authoritative References

- [Linux binding design DOs and DON'Ts](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Devicetree Specification: compatible-list semantics](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux SoC maintainer guidance](https://docs.kernel.org/process/maintainer-soc.html)

## Next Step

Continue with [From Device Tree Nodes To Linux Devices](from-device-tree-nodes-to-linux-devices.md).
