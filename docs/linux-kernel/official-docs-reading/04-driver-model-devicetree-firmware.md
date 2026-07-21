---
status: active
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# 4. Driver Model, Device Tree, And Firmware

Official sections: [Driver APIs](https://docs.kernel.org/driver-api/index.html),
[Open Firmware and Devicetree](https://docs.kernel.org/devicetree/index.html), and
[Firmware](https://docs.kernel.org/firmware-guide/index.html)

Knowledge-guide companion: [Stage 4](knowledge-guide-companion.md#stage-4-driver-model-device-tree-and-firmware)

## Device And Driver Model

- [ ] **P0** [Driver model overview](https://docs.kernel.org/driver-api/driver-model/overview.html)
- [ ] **P0** [Device model binding](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [ ] **P0** [Bus types](https://docs.kernel.org/driver-api/driver-model/bus.html)
- [ ] **P0** [Devices](https://docs.kernel.org/driver-api/driver-model/device.html)
- [ ] **P0** [Drivers](https://docs.kernel.org/driver-api/driver-model/driver.html)
- [ ] **P0** [Device drivers infrastructure](https://docs.kernel.org/driver-api/infrastructure.html)
- [ ] **P0** [Device links](https://docs.kernel.org/driver-api/device_link.html)
- [ ] **P0** [Platform devices and drivers](https://docs.kernel.org/driver-api/driver-model/platform.html)
- [ ] **P0** [Devres: managed device resources](https://docs.kernel.org/driver-api/driver-model/devres.html)
- [ ] **P0** [Device classes](https://docs.kernel.org/driver-api/driver-model/class.html)
- [ ] **P0** [Uevents](https://docs.kernel.org/driver-api/driver-model/overview.html)
- [ ] **P1** [Component helper for aggregate drivers](https://docs.kernel.org/driver-api/component.html)

## Device Tree Concepts And Kernel API

- [ ] **P0** [Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [ ] **P0** [Kernel Devicetree API](https://docs.kernel.org/devicetree/kernel-api.html)
- [ ] **P0** [Dynamic resolvers and overlays](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [ ] **P0** [Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [ ] **P1** [Devicetree changesets](https://docs.kernel.org/devicetree/kernel-api.html#devicetree-changesets)
- [ ] **P1** [Devicetree unittest](https://docs.kernel.org/devicetree/of_unittest.html)

## Binding Schema Work

- [ ] **P0** [Writing Devicetree schemas](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [ ] **P0** [Writing Devicetree bindings in JSON schema](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [ ] **P0** [Submitting Devicetree binding patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [ ] **P0** Read the common property schemas used by the project's nodes.
- [ ] **P0** Read the exact binding schema for every peripheral used by one board.
- [ ] **P0** Validate a project binding with `dt_binding_check`.
- [ ] **P0** Validate the board DTBs with `dtbs_check`.
- [ ] **P1** Compare upstream and TI-vendor bindings for one changed peripheral.

## Firmware Loading And Platform Firmware

- [ ] **P0** [Firmware search paths](https://docs.kernel.org/driver-api/firmware/fw_search_path.html)
- [ ] **P0** [Firmware request API](https://docs.kernel.org/driver-api/firmware/request_firmware.html)
- [ ] **P0** [Built-in firmware](https://docs.kernel.org/driver-api/firmware/built-in-fw.html)
- [ ] **P1** [Firmware API fallback mechanism](https://docs.kernel.org/driver-api/firmware/fallback-mechanisms.html)
- [ ] **P1** [Firmware security notes](https://docs.kernel.org/driver-api/firmware/index.html)

## Board-Level Exercise

- [ ] Pick one AM335x/AM62x/AM64x peripheral and trace `compatible` to match table and `probe()`.
- [ ] Trace every referenced clock, reset, regulator, GPIO, pinctrl state, IRQ, DMA channel, and power domain to its provider.
- [ ] Prove the probe order and identify where deferred probe can occur.
- [ ] Inspect device, driver, modalias, supplier, and consumer links in sysfs.
- [ ] Exercise bind/unbind only after verifying that the driver supports safe teardown.
