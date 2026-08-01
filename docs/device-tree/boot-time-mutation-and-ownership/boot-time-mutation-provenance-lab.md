---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Boot-Time Mutation Provenance Lab

This lab follows one Linux DTB through trusted-firmware reservations, a board-revision overlay, per-unit identity injection, RAM discovery, and `/chosen` preparation. It produces checkpoint artifacts and a mutation manifest that explain every difference between the built base and Linux's live tree.

## Objectives

By the end, you should be able to:

- define owners, inputs, preconditions, and failure policy for every mutation
- calculate safe DTB capacity and all boot-image intervals
- reconcile installed RAM and secure reservations
- encode initrd and console handoff correctly
- inject identity without ambiguous precedence
- apply overlays transactionally in canonical order
- compare deterministic hardware state separately from ephemeral seed data
- bisect deliberate failures to the responsible stage

## Platform Contract

Assume:

- the base DTS describes a maximum 1 GiB DRAM bank at `0x80000000`
- this unit discovers 768 MiB, ending at `0xb0000000`
- trusted firmware owns the top 16 MiB: `[0xaf000000, 0xb0000000)`
- Board revision 4 requires one authenticated `rev4.dtbo`
- a secure provisioning service supplies one Ethernet MAC and public serial
- U-Boot loads a 32 MiB initrd at `[0x98000000, 0x9a000000)`
- the final working-FDT buffer is `[0x97000000, 0x97080000)` (512 KiB)
- `serial0` is the intended boot console
- a trusted hardware RNG supplies independent `rng-seed` and `kaslr-seed` values
- U-Boot and every mutation input are authenticated under the product boot chain

Addresses and interfaces are illustrative.

## Step 1: Define The Base

Relevant base fragment:

```dts
/ {
        #address-cells = <2>;
        #size-cells = <2>;
        model = "Example Product";
        compatible = "example,product", "example,soc";

        aliases {
                serial0 = &uart0;
                ethernet0 = &ethernet0;
        };

        chosen {
                stdout-path = "serial0:115200n8";
        };

        memory@80000000 {
                device_type = "memory";
                reg = <0x0 0x80000000 0x0 0x40000000>;
        };

        reserved-memory {
                #address-cells = <2>;
                #size-cells = <2>;
                ranges;
        };
};

&ethernet0 {
        status = "okay";
};
```

The maximum memory description is a product convention; the validated boot fixup narrows it to detected RAM before Linux receives the tree.

## Step 2: Build The Mutation Manifest

| ID | Owner | Input | Target | Failure |
|---|---|---|---|---|
| `DTM-100` | trusted-firmware bridge | authenticated reservation response | `/reserved-memory/secure@af000000` | abort normal boot |
| `DTM-120` | product composition | signed board manifest | apply `rev4.dtbo` | recovery |
| `DTM-200` | memory-init authority | training result | `/memory@80000000/reg` | abort |
| `DTM-220` | provisioning bridge | secure service | MAC and root serial | provisioning recovery |
| `DTM-300` | boot policy | signed boot config | `/chosen/bootargs` | recovery |
| `DTM-320` | image loader | verified initrd placement | initrd start/end | boot without initrd only if policy allows |
| `DTM-340` | entropy handoff | trusted RNG | seed properties | security-policy failure |

Define allowed paths for every ID. Any change outside them fails the postcondition.

## Step 3: Establish Checkpoints

```text
C0 base.dtb from release build
C1 verified base copied/opened at working-FDT buffer
C2 secure reservation serialized
C3 rev4 overlay applied
C4 RAM and identity fixed
C5 deterministic bootargs/initrd/console complete
C6 seeds added immediately before handoff
C7 Linux live tree
```

Save packed development copies at `C0` through `C5`. At `C6`, save only a protected/redacted artifact and record a final hash in the secure diagnostic channel.

## Step 4: Prove Capacity And Intervals

Assume measured worst cases:

```text
base DTB totalsize        82 KiB
rev4 overlay growth       21 KiB
secure/memory fixups       3 KiB
identity and /chosen       5 KiB
future bounded margin     64 KiB
required                 175 KiB
reserved capacity        512 KiB
```

The 512 KiB interval is sufficient. Now prove it does not overlap:

```text
kernel output      [0x80200000, 0x85000000)
working FDT        [0x97000000, 0x97080000)
overlay input      [0x97100000, 0x97110000)
initrd             [0x98000000, 0x9a000000)
secure reservation [0xaf000000, 0xb0000000)
```

Also include FIT source, U-Boot relocation/malloc/stack, and DMA writers from the actual platform.

## Step 5: Serialize Secure Memory

Expected result:

```dts
reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        secure@af000000 {
                reg = <0x0 0xaf000000 0x0 0x01000000>;
                no-map;
        };
};
```

Validate:

- base and size equal the authenticated response
- end is exactly `0xb0000000` without overflow
- region lies within discovered DRAM
- no other normal-world region overlaps it
- hardware security configuration covers the same interval

The `no-map` property does not create the security boundary.

## Step 6: Apply The Revision Overlay

Assume revision 4 changes a GPIO-controlled peripheral:

```dts
/dts-v1/;
/plugin/;

&expansion_device {
        reset-gpios = <&gpio2 11 GPIO_ACTIVE_LOW>;
        status = "okay";
};
```

Authenticate it, confirm base/revision compatibility, apply it to the disposable working copy, check the return code, and assert:

- only approved paths changed
- GPIO 2.11 has no other active consumer
- providers and ancestors remain available
- final composed tree passes schema checks

Record base, overlay, and `C3` hashes.

## Step 7: Narrow RAM And Inject Identity

Expected memory tuple:

```dts
memory@80000000 {
        device_type = "memory";
        reg = <0x0 0x80000000 0x0 0x30000000>;
};
```

This describes 768 MiB; the secure 16 MiB remains inside it but reserved separately.

Provisioning writes:

```dts
/ {
        serial-number = "EXAMPLE-000042";
};

&ethernet0 {
        local-mac-address = [00 1a 2b 3c 4d 5e];
};
```

Validate source authentication, format, six-byte unicast MAC, uniqueness, and existing-property policy. Use development values only; production artifacts must protect identifiers according to privacy policy.

## Step 8: Prepare `/chosen`

After verified image placement:

```dts
chosen {
        stdout-path = "serial0:115200n8";
        bootargs = "root=PARTUUID=... ro rootwait";
        linux,initrd-start = <0x0 0x98000000>;
        linux,initrd-end = <0x0 0x9a000000>;
};
```

The end is exclusive, so size is:

```text
0x9a000000 - 0x98000000 = 0x02000000 = 32 MiB
```

Assert one `root=` policy, valid alias target, root cell width, and no interval overlap. If initrd loading fails, remove both initrd properties before any permitted no-initrd fallback.

Add independent seed properties only after deterministic checkpoint `C5`. Do not print them in the lab report.

## Step 9: Capture Evidence

Host/replay environment:

```sh
sha256sum base.dtb rev4.dtbo
fdtoverlay -i base.dtb -o c3-merged.dtb rev4.dtbo
dtc -I dtb -O dts -o c3-merged.dts c3-merged.dtb
```

U-Boot:

```text
=> fdt addr
=> fdt header
=> fdt print /memory@80000000
=> fdt print /reserved-memory
=> fdt print /chosen
=> fdt print /soc/ethernet@...
```

Export exactly `totalsize` bytes from the final handoff in a controlled development build. Redact seeds and sensitive identifiers from general logs.

Linux:

```sh
dtc -I fs -O dts -o linux-live.dts /sys/firmware/devicetree/base
cat /proc/cmdline
cat /proc/iomem
ip -br link
dmesg | grep -Ei 'Machine model|reserved memory|Kernel command line'
```

Compare every checkpoint semantically using the manifest's allowed paths.

## Step 10: Diagnose Deliberate Faults

### Fault A: Trusted Firmware Reports Size `0x02000000`

The secure region becomes `[0xaf000000, 0xb1000000)`, extending beyond detected DRAM. Reject the response or boot. Do not truncate a secure reservation.

### Fault B: RAM Fixup Runs Before Discovery Validation

A corrupted training result reports 4 GiB. Range and SKU-bound validation must happen before `fdt_setprop`; later reservations cannot make invented RAM safe.

### Fault C: Overlay Application Returns `FDT_ERR_NOSPACE`

Discard the working base and overlay according to U-Boot's invalidation warning. Fix capacity calculation and replay from `C1`; do not continue to identity fixups.

### Fault D: MAC Exists In Both Base And Provisioning

Apply the documented precedence. If the base contains a valid production value unexpectedly, flag the release input rather than silently overwriting it.

### Fault E: Initrd End Is Written As `0x02000000`

That is a size, not the exclusive end address. Linux will see an invalid/wrapped interval. Correct it to `0x9a000000` with two root address cells.

### Fault F: Final DTB Hash Changes, Hardware Did Not

Seeds and possibly bootargs/slot state vary per boot. Compare deterministic checkpoint `C5` and a redacted semantic diff. Never make seeds deterministic to stabilize a test.

### Fault G: Linux Reports 768 MiB But Uses Secure Top 16 MiB

The `/memory` fixup succeeded, but reservation serialization or early consumption failed. Compare `C6`, Linux live `/reserved-memory`, early memblock logs, `/proc/iomem`, and hardware firewall range.

### Fault H: Warm Boot Retains Old Initrd Properties

The failed loader reused a prior working buffer. Recreate from a trusted base/checkpoint and remove paired transient properties on failure.

## Exit Review

The lab is complete when you can provide:

- mutation ownership manifest
- C0–C6 checkpoint strategy and protected evidence
- full capacity and memory interval proof
- canonical RAM/reservation model
- overlay compatibility and resource-conflict result
- identity precedence and validation record
- `/chosen` encoding and initrd arithmetic
- deterministic and ephemeral comparison strategy
- Linux runtime reconciliation
- root cause for every deliberate fault

## Authoritative References

- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Upstream `/chosen` schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/chosen.yaml)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)

## Continue

Proceed to [Binding Design And Stable ABI](../binding-design-and-stable-abi.md).
