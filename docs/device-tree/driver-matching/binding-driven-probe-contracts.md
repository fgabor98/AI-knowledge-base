---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding-Driven Probe Contracts

The binding defines what the hardware description means. The driver implements that contract. Matching only selects a candidate implementation; `probe()` must validate and acquire the resources promised for the matched compatible.

## Three Sources Must Agree

```text
binding schema: what nodes are valid
Device Tree:    facts about this hardware instance
driver:         what software accepts and implements
```

Schema-valid but unsupported hardware can still fail if the driver lacks a feature. Driver-supported legacy DTBs can exist before schemas were strict. Nevertheless, new work should keep all three aligned and test them together.

## Required Properties

A schema's `required` list identifies properties every matching node must provide for that schema branch:

```yaml
required:
  - compatible
  - reg
  - interrupts
  - clocks
  - clock-names
```

The driver should not silently invent values for missing required hardware resources. It should use subsystem APIs, check errors, and return a meaningful failure.

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
        return dev_err_probe(dev, irq, "failed to get interrupt\n");

clk = devm_clk_get(dev, "core");
if (IS_ERR(clk))
        return dev_err_probe(dev, PTR_ERR(clk),
                             "failed to get core clock\n");
```

The API may distinguish absence, malformed data, unavailable providers, and deferred suppliers. Preserve that error instead of converting every failure to `-EINVAL`.

## Optional Does Not Mean Ignored

An optional property has binding-defined semantics when absent. Typical meanings include:

- hardware feature is not wired on this instance
- a documented default applies
- firmware already configured an immutable choice
- the driver may use a polling path instead of an interrupt

Use the appropriate optional API and distinguish absence from real errors:

```c
reset = devm_reset_control_get_optional_exclusive(dev, NULL);
if (IS_ERR(reset))
        return dev_err_probe(dev, PTR_ERR(reset),
                             "failed to get optional reset\n");
```

“Optional” does not mean the driver may ignore a present malformed property. Nor does a permissive driver make an actually required hardware resource optional in the binding.

## Compatible-Conditional Requirements

Variants can require different resources:

```yaml
allOf:
  - if:
      properties:
        compatible:
          contains:
            const: acme,ax200-uart
    then:
      required:
        - resets
```

The corresponding AX200 match data and probe path must acquire that reset. Review the fallback chain: if AX200 also claims AX100 compatibility but cannot work without AX200-aware reset handling, the fallback may be unsafe.

## Probe Deferral

`-EPROBE_DEFER` means a required supplier is expected but not ready. It is not a generic “try again” response for malformed DT data or permanent hardware failure.

Use `dev_err_probe()` to preserve the error and record a deferral reason. Linux can expose deferred devices and reasons through debugfs when enabled:

```sh
cat /sys/kernel/debug/devices_deferred
```

Repeated deferral is often caused by:

- provider node absent or disabled
- provider driver not enabled or not packaged
- wrong provider compatible
- bad phandle or specifier
- circular dependency
- provider probe failure hidden earlier in the log

Increasing initcall priority or adding arbitrary retries usually hides the dependency defect.

## Probe Failure Must Unwind

A negative return means the driver did not bind. Probe must release or undo everything acquired before the failure. Managed `devm_*` resources help with memory and registrations, but hardware actions can still need explicit rollback.

Examples:

- disable clocks and regulators enabled earlier
- reassert a reset if policy requires a safe state
- stop DMA and mask interrupts
- remove child devices or providers registered before failure
- clear partially programmed hardware state

A resource leak or active DMA after failed probe can make later reprobes nondeterministic.

## Successful Probe Is Not Functional Proof

Returning zero creates a driver binding and usually a `driver` symlink. It does not prove:

- interrupt polarity and routing work under load
- DMA addresses and coherency are correct
- external pins and supplies match the schematic
- suspend/resume or reset recovery works
- the device's protocol partner is present
- security isolation is enforced

Functional tests belong after the matching pipeline, with subsystem-specific evidence.

## Prevent Schema/Driver Drift

In review and CI:

1. validate the schema itself
2. run DT schema checks for every affected DTS
3. map each required property to a probe acquisition or framework consumer
4. map each optional property to documented absence semantics
5. map each compatible branch to match data and tests
6. test error unwinding and supplier deferral
7. boot old DTBs with the new driver

Do not define a property solely because the current driver wants a software switch. Bindings describe hardware, including features the present driver may not yet implement.

## Senior Review Checklist

- Can every probe lookup be traced to a binding property or discoverable resource?
- Does the driver preserve meaningful errno values?
- Are optional getters used only where absence is truly valid?
- Does `-EPROBE_DEFER` identify a plausible supplier?
- Are fallback compatibles consistent with conditional requirements?
- Is cleanup correct after failure at every acquisition step?
- Are unknown properties rejected by schema where appropriate?
- Do tests distinguish binding, probe, and runtime-functional success?

## Authoritative References

- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux binding design guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux `dev_err_probe` and driver infrastructure](https://docs.kernel.org/driver-api/infrastructure.html)
- [Linux driver model and probe returns](https://docs.kernel.org/driver-api/driver-model/driver.html)

## Next Step

Apply the pipeline in the [Driver Matching And Probe Diagnosis Lab](driver-matching-and-probe-diagnosis-lab.md).
