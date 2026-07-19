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

```dts
&usb0 {
        dr_mode = "host";
        phys = <&usb2_phy0>;
        phy-names = "usb2-phy";
        vbus-supply = <&reg_usb_vbus>;
        status = "okay";
};
```

Typical `dr_mode` values are `host`, `peripheral`, and `otg`. They describe supported/fixed wiring and controller role, not an arbitrary preference. A host port sources VBUS through board power hardware; a peripheral senses/consumes VBUS. Driving VBUS from both ends is unsafe.

The exact placement of `vbus-supply`, PHY handles, and role properties depends on the controller/wrapper schema. Some SoCs split USB2 and USB3 PHYs or represent the DWC core below a glue node.

## Dual-Role And Type-C

A dual-role Type-C port may use a connector node controlled by a Type-C/PD device:

```dts
connector {
        compatible = "usb-c-connector";
        label = "USB-C";
        power-role = "dual";
        data-role = "dual";
        try-power-role = "source";
};
```

This is only a fragment; real ports may use graph endpoints to connect the connector, controller, orientation switch, and alternate-mode mux. A Type-C controller or firmware often owns role negotiation. Do not simultaneously use incompatible ID/VBUS GPIO role detection and a PD controller unless the binding defines their cooperation.

Power role, data role, and USB controller role are related but not identical. USB-PD can swap power and data roles independently under supported policy. The role-switch framework communicates the negotiated data role to the controller.

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
