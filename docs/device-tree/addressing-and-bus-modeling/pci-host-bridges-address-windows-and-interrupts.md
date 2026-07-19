---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# PCI Host Bridges, Address Windows, And Interrupts

A PCI host bridge connects several distinct namespaces: CPU-visible controller registers, PCI configuration space, PCI I/O space, prefetchable and non-prefetchable memory windows, DMA requester addresses, legacy INTx pins, and MSI messages. Device Tree models these with different properties; none is a universal “PCI base address.”

## Host-Bridge Skeleton

```dts
pcie@4010000000 {
        compatible = "pci-host-ecam-generic";
        device_type = "pci";
        #address-cells = <3>;
        #size-cells = <2>;
        bus-range = <0x00 0xff>;
        reg = <0x00000040 0x10000000 0x0 0x10000000>;

        ranges = <0x01000000 0x0 0x00000000
                  0x00000000 0x3f000000
                  0x00000000 0x00010000>,
                 <0x02000000 0x0 0x40000000
                  0x00000000 0x40000000
                  0x00000000 0x40000000>;
};
```

Assume the root has two address and two size cells. The host node's `reg` is therefore root-encoded: ECAM at CPU physical `0x4010000000`, size `0x10000000`. Its `ranges` entries instead use three PCI child-address cells, two parent-address cells, and two size cells.

This example is illustrative and must not be copied without the host-controller binding, actual apertures, and architecture rules.

## PCI Child Address Encoding

The first PCI address cell is not simply the high 32 bits of an address. It contains address-space and attribute bits plus encoded bus/device/function and register information as defined by the PCI bus binding. The remaining cells complete the PCI address.

Common space-code patterns in `ranges` include:

- `0x01000000`: PCI I/O space
- `0x02000000`: 32-bit PCI memory space
- `0x03000000`: 64-bit PCI memory space
- additional bits: relocatable, prefetchable, and aliased attributes where defined

Decode the bit fields; do not concatenate all three cells into a 96-bit number.

The example maps:

- PCI I/O `[0, 0x10000)` to parent/CPU `[0x3f000000, 0x3f010000)`
- PCI memory `[0x40000000, 0x80000000)` to parent/CPU `[0x40000000, 0x80000000)`

PCI I/O space is a separate resource type even if the host maps accesses through a CPU MMIO aperture.

## Configuration Space And `bus-range`

`reg`/`reg-names` describe host-controller resources required by its binding, which may include ECAM or implementation-specific control blocks. `bus-range` states which PCI bus numbers the host owns. For generic ECAM, configuration-space size and bus coverage must agree with the ECAM layout.

Do not place the host controller's own control registers in PCI `ranges`; they are consumed by the host and belong in its parent-encoded `reg`. `ranges` describes windows forwarded to transactions below the host bridge.

## Enumeration And Child Nodes

PCI configuration-space enumeration discovers vendor/device IDs, BAR requirements, bridges, and capabilities. A node for every PCI function is not normally required. Fixed PCI child nodes are used when firmware must attach non-discoverable platform information.

PCI unit addresses and child `reg` values encode bus/device/function and register/space information according to the PCI binding. They are not the BAR address eventually assigned by Linux. Avoid pinning discoverable resources in DT unless the binding and boot architecture require it.

Validate runtime topology with:

```sh
lspci -nn
lspci -t
lspci -vv
cat /proc/iomem
```

Compare host windows, assigned BARs, bridge windows, and the domain/bus numbering Linux selected.

## Legacy INTx Routing

PCI functions expose interrupt pins INTA–INTD. Board wiring and bridge swizzling route those pins to a parent interrupt controller. A host bridge or interrupt nexus commonly provides:

- `#interrupt-cells`
- `interrupt-map-mask`
- `interrupt-map`

The child match key combines the PCI child unit address—typically enough bus/device/function bits selected by the mask—with the interrupt pin. Each row then names the parent and its specifier. A correct mask is critical: masking too many device/function bits can send several slots to the same unintended route, while matching irrelevant register bits can prevent any row from matching.

Keep three identifiers separate:

1. PCI interrupt pin number in the child domain
2. parent-controller hardware interrupt
3. dynamically allocated Linux IRQ

Bridge swizzling may transform INTx pins at every bridge. Test multiple slots and functions, not only the first enumerated endpoint.

## MSI And MSI-X

MSI/MSI-X use memory writes rather than physical INTx pins. Device Tree can associate a host or requester range with an MSI controller through `msi-parent` or `msi-map`/`msi-map-mask`. The mapping may depend on PCI requester IDs.

MSI routing also interacts with IOMMU requester identity and interrupt-remapping security. Verify that:

- requester-ID mapping covers every subordinate bus and function
- aliases created by bridges are handled
- the MSI doorbell address is reachable by the device's DMA path
- interrupt remapping is enforced where isolation depends on it
- INTx fallback works when MSI is disabled

## DMA Through PCI

PCI outbound CPU windows in `ranges` and inbound device-to-memory translation are independent. `dma-ranges`, host-controller hardware, and IOMMU mappings may constrain DMA. A BAR working through a memory window says nothing about whether that endpoint can reach all RAM.

For endpoint assignment or untrusted devices, review ACS, IOMMU grouping, requester aliases, peer-to-peer paths, and firmware-programmed bypasses. Device Tree alone cannot turn hardware without adequate isolation boundaries into a secure topology.

## Senior Review Checklist

- Does `reg` describe host resources while `ranges` describes forwarded child windows?
- Are I/O, 32-bit memory, 64-bit memory, and prefetchable attributes correct?
- Do windows match bridge hardware and avoid RAM/reserved-resource conflicts?
- Does ECAM size agree with `bus-range`?
- Are PCI domain and bus-number assumptions stable across firmware versions?
- Does INTx mapping account for slot/function masking and bridge swizzling?
- Do MSI and IOMMU requester maps cover the same aliases and hierarchy?
- Are hotplug, resource reassignment, 64-bit BARs, and fallback interrupt modes tested?

## Authoritative References

- [Devicetree Specification: address and interrupt mapping](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux generic ECAM PCI host binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/pci/host-generic-pci.yaml)
- [Linux PCI driver documentation](https://docs.kernel.org/PCI/pci.html)
- [Linux DeviceTree PCI translation APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux PCI sysfs ABI](https://docs.kernel.org/PCI/sysfs-pci.html)

## Next Step

Apply the model in the [Address Translation And Bus Modeling Lab](address-translation-and-bus-modeling-lab.md).
