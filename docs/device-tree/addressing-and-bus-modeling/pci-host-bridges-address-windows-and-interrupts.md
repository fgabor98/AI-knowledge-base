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

The AM64x PCIe root complex (`k3-am64-main.dtsi`, clocks and power trimmed):

```dts
pcie0_rc: pcie@f102000 {
        compatible = "ti,am64-pcie-host", "ti,j721e-pcie-host";
        reg = <0x00 0x0f102000 0x00 0x1000>,
              <0x00 0x0f100000 0x00 0x400>,
              <0x00 0x0d000000 0x00 0x00800000>,
              <0x06 0x00000000 0x00 0x00001000>; /* ECAM (4 KB) */
        reg-names = "intd_cfg", "user_cfg", "reg", "cfg";
        device_type = "pci";
        #address-cells = <3>;
        #size-cells = <2>;
        bus-range = <0x0 0xff>;
        msi-map = <0x0 &gic_its 0x0 0x10000>;
        ranges = <0x01000000 0x00 0x00001000 0x06 0x00001000 0x00 0x00100000>, /* IO (1 MB) */
                 <0x02000000 0x00 0x00101000 0x06 0x00101000 0x00 0xffeff000>; /* 32-bit Non-Prefetchable MEM (4 GB - 1 MB - 4 KB) */
        dma-ranges = <0x02000000 0x0 0x0 0x0 0x0 0x00000010 0x0>;
        /* ... */
};
```

The parent `bus@f4000` has two address and two size cells, so the four `reg` entries are parent-encoded: wrapper registers, user configuration, the Cadence core at `0x0d000000`, and a 4 KiB configuration window at `0x6_0000_0000`. The `ranges` entries instead use three PCI child-address cells, two parent-address cells, and two size cells.

This node only makes sense together with the `ti,j721e-pci-host.yaml` binding and the AM64x memory map; do not copy its apertures to another SoC.

## PCI Child Address Encoding

The first PCI address cell is not simply the high 32 bits of an address. It contains address-space and attribute bits plus encoded bus/device/function and register information as defined by the PCI bus binding. The remaining cells complete the PCI address.

Common space-code patterns in `ranges` include:

- `0x01000000`: PCI I/O space
- `0x02000000`: 32-bit PCI memory space
- `0x03000000`: 64-bit PCI memory space
- additional bits: relocatable, prefetchable, and aliased attributes where defined

Decode the bit fields; do not concatenate all three cells into a 96-bit number.

The AM64x example maps:

- PCI I/O `[0x1000, 0x101000)` to CPU `[0x6_0000_1000, 0x6_0010_1000)`
- 32-bit PCI memory `[0x101000, 0x1_0000_0000)` to CPU `[0x6_0010_1000, 0x7_0000_0000)`

Both windows live in the 4 GiB “PCIe DAT1” region at `0x6_0000_0000` that `bus@f4000` forwards, directly after the 4 KiB configuration window.

PCI I/O space is a separate resource type even if the host maps accesses through a CPU MMIO aperture.

## Configuration Space And `bus-range`

`reg`/`reg-names` describe host-controller resources required by its binding, which may include ECAM or implementation-specific control blocks. `bus-range` states which PCI bus numbers the host owns. For generic ECAM, configuration-space size and bus coverage must agree with the ECAM layout: 1 MiB per bus.

The AM64x node is a useful counterexample. Its `cfg` window is commented “ECAM” but is only 4 KiB while `bus-range` covers 256 buses. That is valid for this controller because the Cadence host driver (`pcie-cadence-host.c`) reprograms outbound address-translation region 0 with the target bus and devfn before every configuration access, so one 4 KiB window serves all functions. Follow the controller binding and driver, not the generic ECAM rule, when a host is not `pci-host-ecam-generic`.

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

The Arm Juno PCIe host shows a complete four-row map with mask `<0 0 0 7>`; it is decoded cell by cell in [Interrupt Nexus Mapping](../standard-nodes-and-properties/cross-cutting-standard-relationships.md#interrupt-nexus-mapping).

The child match key combines the PCI child unit address—typically enough bus/device/function bits selected by the mask—with the interrupt pin. Each row then names the parent and its specifier. A correct mask is critical: masking too many device/function bits can send several slots to the same unintended route, while matching irrelevant register bits can prevent any row from matching.

Keep three identifiers separate:

1. PCI interrupt pin number in the child domain
2. parent-controller hardware interrupt
3. dynamically allocated Linux IRQ

Bridge swizzling may transform INTx pins at every bridge. Test multiple slots and functions, not only the first enumerated endpoint.

## MSI And MSI-X

MSI/MSI-X use memory writes rather than physical INTx pins. Device Tree can associate a host or requester range with an MSI controller through `msi-parent` or `msi-map`/`msi-map-mask`. The AM64x root complex maps all 65536 requester IDs one-to-one onto GIC ITS device IDs: `msi-map = <0x0 &gic_its 0x0 0x10000>;` (RID base, controller, ITS ID base, length).

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
