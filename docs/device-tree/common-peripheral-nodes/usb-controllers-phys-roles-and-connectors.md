---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# USB Controllers, PHYs, Roles, And Connectors

USB integration can include a controller core, wrapper, PHY, connector, VBUS switch, role detector, Type-C/PD controller, orientation switch, and mux. Correct DT gives each component one owner and connects their relationships.

## Fixed Host Or Peripheral Role

AM62x splits each USB instance into a TI wrapper and a Synopsys DWC3 core. From `k3-am62-main.dtsi` (trimmed):

```dts
usbss1: dwc3-usb@f910000 {
        compatible = "ti,am62-usb";
        clocks = <&k3_clks 162 3>;
        clock-names = "ref";
        ti,syscon-phy-pll-refclk = <&usb1_phy_ctrl 0x0>;
        power-domains = <&k3_pds 179 TI_SCI_PD_EXCLUSIVE>;
        ranges;
        status = "disabled";

        usb1: usb@31100000 {
                compatible = "snps,dwc3";
                reg = <0x00 0x31100000 0x00 0x50000>;
                interrupts = <GIC_SPI 226 IRQ_TYPE_LEVEL_HIGH>, /* irq.0 */
                             <GIC_SPI 226 IRQ_TYPE_LEVEL_HIGH>; /* irq.0 */
                interrupt-names = "host", "peripheral";
                maximum-speed = "high-speed";
                dr_mode = "otg";
        };
};
```

The AM62x SK makes USB1 a fixed host (`k3-am62x-sk-common.dtsi`):

```dts
main_usb1_pins_default: main-usb1-default-pins {
        pinctrl-single,pins = <
                AM62X_IOPAD(0x0258, PIN_OUTPUT, 0) /* (F18/E16) USB1_DRVVBUS */
        >;
};

&usbss1 {
        status = "okay";
        ti,vbus-divider;
};

&usb1 {
        dr_mode = "host";
        pinctrl-names = "default";
        pinctrl-0 = <&main_usb1_pins_default>;
};
```

Typical `dr_mode` values are `host`, `peripheral`, and `otg`. The SoC default is `otg` because the silicon supports both roles; the board narrows it to `host` because this port is wired host-only. A host port sources VBUS; here the controller's own `USB1_DRVVBUS` pin controls it, so there is no `vbus-supply` regulator. `ti,vbus-divider` tells the wrapper that VBUS reaches the SoC's sense pin through a 1/3 divider. Driving VBUS from both ends of a cable is unsafe.

There is no `phys` property. The USB2 PHY is integrated into the wrapper, and `ti,syscon-phy-pll-refclk` points at the control register that tells it the reference clock. The placement of `vbus-supply`, PHY handles, and role properties always depends on the controller and wrapper schemas.

## Dual-Role And Type-C

The SK's USB0 is a Type-C port managed by a TI TPS6598x PD controller on `main_i2c0`:

```dts
typec_pd0: tps6598x@3f {
        compatible = "ti,tps6598x";
        reg = <0x3f>;

        connector {
                compatible = "usb-c-connector";
                label = "USB-C";
                self-powered;
                data-role = "dual";
                power-role = "sink";
                port {
                        usb_con_hs: endpoint {
                                remote-endpoint = <&usb0_hs_ep>;
                        };
                };
        };
};

&usb0 {
        usb-role-switch;

        port {
                usb0_hs_ep: endpoint {
                        remote-endpoint = <&usb_con_hs>;
                };
        };
};
```

The TPS6598x owns negotiation. It reports the data role through the role-switch framework, and the graph endpoint tells it which controller to switch. `usb-role-switch` makes DWC3 register as that switch instead of detecting the role from ID or VBUS pins. Do not also add ID/VBUS GPIO detection unless the binding defines how the two cooperate.

Power role, data role, and controller role are related but separate. This port is a power `sink` that never sources VBUS, `self-powered` says the board does not run from that VBUS, and its data role is `dual`, so it can be a USB device to a PC or a host to a flash drive. USB-PD can swap power and data roles independently under supported policy.

## PHYs And Signal Paths

The USB PHY handles analog signaling, calibration, reference clocks, resets, and sometimes charger detection. `phys` selects the PHY instance; it does not replace the controller's own clocks or supplies.

For SuperSpeed Type-C, orientation can swap high-speed lanes. A switch/mux must route them based on connector orientation and alternate mode. Graph endpoints may be necessary because a phandle alone cannot express all port-to-port topology; that mechanism is treated in the next roadmap module.

## VBUS, Overcurrent, And Suspend

Host VBUS may be a regulator-backed load switch with enable delay, current limit, and overcurrent interrupt. Its upstream supply belongs in the regulator graph. Verify that the USB controller and regulator do not both request the raw enable GPIO.

Wake requires an always-on detection path. The main controller or PHY can be suspended while a separate wake detector remains active. Test remote wake, cable attach, detach, role swaps, and overcurrent from every supported sleep state.

## Enumeration Boundary

Ordinary removable USB devices enumerate dynamically and should not be permanent DT children. DT describes fixed onboard devices only when a binding requires non-discoverable properties or a non-removable topology, often using USB port nodes. The USB descriptor remains the primary identity for enumerated functions.

## Runtime Diagnosis

```sh
lsusb -t
lsusb -v
ls -l /sys/class/usb_role /sys/class/typec 2>/dev/null
dmesg | grep -Ei 'usb|xhci|dwc|type.?c|vbus|phy'
```

Separate failures by layer: no VBUS, no attach detection, PHY/link-training errors, enumeration/descriptor failure, driver binding, or gadget configuration. A power meter and oscilloscope may be necessary before protocol traces are meaningful.

## Authoritative References

- [Linux USB driver API](https://docs.kernel.org/driver-api/usb/usb.html)
- [Linux USB Type-C connector class](https://docs.kernel.org/driver-api/usb/typec.html)
- [Linux generic USB DRD schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/usb/usb-drd.yaml)
- [Linux USB connector schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/connector/usb-connector.yaml)

## Continue

Proceed to [PCIe Host Bridges, Windows, And Enumeration](pcie-host-bridges-windows-and-enumeration.md).
