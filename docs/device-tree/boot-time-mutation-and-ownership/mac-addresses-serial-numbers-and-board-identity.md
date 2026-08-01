---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# MAC Addresses, Serial Numbers, And Board Identity

Per-unit identity often cannot live in a shared compiled DTB. Firmware may read fuses, EEPROM, NVMEM, secure provisioning services, or environment and inject values before Linux boots. The difficult part is not setting a property; it is precedence, validity, persistence, privacy, and avoiding two independent consumers of the same source.

## Inventory Identity Sources

For each value, document:

| Value | Candidate sources |
|---|---|
| Ethernet MAC | dedicated EEPROM, SoC fuse, NIC EEPROM, environment, generated local address |
| Wi-Fi/Bluetooth address | module NVMEM, vendor calibration store, secure service |
| serial number | product EEPROM, secure manufacturing database, fuse-derived public ID |
| board revision/SKU | straps, EEPROM, OTP, trusted manifest |

Do not assume one base MAC can be incremented safely for every interface. Address allocation rules belong to manufacturing/product policy.

## Define Precedence Once

Example policy:

```text
valid authenticated per-interface provisioning
  -> valid hardware/NIC permanent address
  -> valid protected bootloader environment
  -> generated locally administered address for explicit recovery only
  -> disable interface / fail provisioning
```

The right order is product-specific. Implement it in one owner and expose the selected source in diagnostics. If U-Boot injects a final MAC, Linux should not independently choose a different NVMEM source unless the binding defines intentional precedence.

## Validate MAC Addresses

At minimum reject:

- all-zero
- all-ones/broadcast
- multicast bit set for an ordinary interface address
- malformed length
- values outside assigned product policy
- duplicate addresses among active interfaces

Locally administered addresses can be valid when explicitly generated, but must have the local bit set and a collision strategy. Hashing a public serial without a secret does not guarantee uniqueness or privacy.

## Standard Ethernet Properties

Generic Ethernet bindings define properties such as `mac-address` and `local-mac-address`; driver precedence and historical behavior can vary. Follow the current generic binding and the target driver's firmware-node handling.

If bootloader code writes an address:

- target the correct MAC node, not PHY/MDIO child
- use exactly six bytes
- update the intended standard property
- check whether an existing valid property should be preserved
- avoid injecting one value into multiple ports

Test multi-port and disabled-interface variants.

## Prefer Described NVMEM When Appropriate

If Linux can safely and consistently read a stable NVMEM cell, describing the provider/cell relationship may be better than copying the value into DT. Boot-time injection can still be needed when:

- source is available only to firmware
- secure firmware mediates access
- Linux driver lacks the source interface
- the value must be normalized across stages
- the hardware source is powered down or inaccessible later

Do not model both without a precedence contract.

## Root `serial-number`

The root-node schema defines `serial-number` as the device serial. It is a public OS-visible identifier, not a secret. Consider:

- stable formatting and character set
- whether leading zeros matter
- privacy in logs, telemetry, and support bundles
- replacement-board/service policy
- relationship to cryptographic device identity

Never put private keys, authentication tokens, or raw secret fuse contents in DT. A human-readable serial is not a cryptographic identity.

## Board Revision And Compatible

Boot firmware can select a base/overlay from board identity. Avoid rewriting `compatible` casually after other stages already used it. `compatible` participates in machine/driver matching and is a stable hardware ABI.

Preferred approaches:

- choose the correct base DTB for materially different boards
- apply a reviewed revision overlay for population/wiring changes
- use a standard product property only when a binding defines it
- keep diagnostic revision data separate from compatibility matching

If code mutates `model` or `compatible`, document which earlier consumers already made decisions from the old value.

## Identity Trust

CRC-protected EEPROM detects corruption, not forgery. Decide whether identity controls:

- only cosmetic model/serial reporting
- hardware voltage/pin configuration
- boot image or overlay selection
- licensing/features
- verified-boot policy
- network authorization

The higher the impact, the stronger the authenticity and rollback requirements. An attacker-controlled board ID can redirect selection to another valid signed configuration.

## Warm Boot And Replacement

Clear or overwrite identity properties deterministically on every boot. A failed EEPROM read must not preserve a stale valid address in reused RAM.

For replaceable modules:

- identify hot-plug versus boot-only discovery
- bind address to module or baseboard as intended
- handle module absence and replacement
- prevent duplicate cached data
- define whether Linux may rediscover after boot

Static DT mutation is a boot snapshot, not a hotplug protocol.

## Evidence

Record source and validation result without necessarily logging the full value:

```text
eth0 source=secure-provisioning validation=valid
eth1 source=module-nvmem validation=valid
board-id source=eeprom auth=verified revision=4
serial source=manufacturing-record present=yes
```

In development, compare U-Boot's selected values with the final DTB and Linux interface state:

```sh
ip -br link
tr -d '\0' </sys/firmware/devicetree/base/serial-number
hexdump -C /sys/firmware/devicetree/base/soc/ethernet@*/local-mac-address
```

Paths vary. Avoid exporting sensitive identifiers unnecessarily.

## Authoritative References

- [Linux generic Ethernet controller binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/net/ethernet-controller.yaml)
- [Upstream root-node schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/root-node.yaml)
- [Linux NVMEM subsystem](https://docs.kernel.org/driver-api/nvmem.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)

## Continue

Proceed to [Overlay Order, Composition, And Conflict Ownership](overlay-order-composition-and-conflict-ownership.md).
