---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Standard Platform Tree Lab

## Goal

Audit the standard nodes of the real TI AM642 SK DTB—machine identity, aliases, `/chosen`, CPUs, memory, reserved memory, and cross-cutting relationships—then inject failures into copies of the blob and reason from the final DTB.

## Inputs

- `k3-am642-sk.dtb` built from mainline Linux as in the [First Device Tree Lab](../foundations/first-device-tree-lab.md#step-10-build-the-real-board-dtb)
- `arch/arm64/boot/dts/ti/k3-am642-sk.dts`, `k3-am642.dtsi`, `k3-am64.dtsi`, `k3-am64-ti-ipc-firmware.dtsi`
- `dtc`, `fdtget`, `fdtput`, `fdtdump`

## Step 1: Capture Identity

```sh
D=k3-am642-sk.dtb
dtc --version
sha256sum "$D"
dtc -I dtb -O dts -o sk.final.dts "$D"
```

Record any warnings. A mainline DTB compiling cleanly still says nothing about whether this board's firmware will later modify it.

## Step 2: Audit Root, Aliases, And Chosen

```sh
fdtget $D / compatible
fdtget $D / model
fdtget -p $D /aliases
fdtget $D /aliases serial2
fdtget $D /chosen stdout-path
fdtget -p $D /chosen
```

Answer:

1. Which compatible is most specific, and which is the SoC fallback?
2. Which SoC UART is `serial0`, and which is the console? Why are they different?
3. Why does the built `/chosen` contain only `stdout-path` and no `bootargs`?
4. Would changing only `model` affect compatible matching?

## Step 3: Decode CPU, Memory, And Device Cells

| Property | Governing node | Address cells | Size cells | Raw cells | Meaning |
|---|---|---:|---:|---|---|
| `/cpus/cpu@0/reg` | | | | | |
| `/memory@80000000/reg` | | | | | |
| `/reserved-memory/optee@9e800000/reg` | | | | | |
| `/bus@f4000/serial@2800000/reg` | | | | | |

```sh
fdtget -tx $D /cpus/cpu@0 reg
fdtget -tx $D /memory@80000000 reg
fdtget -tx $D /reserved-memory/optee@9e800000 reg
fdtget -tx $D /bus@f4000/serial@2800000 reg
```

Expected reasoning: CPU `reg` has one identifier and no size; root children and `/reserved-memory` children use two-plus-two cells; `bus@f4000` children also use two-plus-two cells before its identity `ranges` window.

## Step 4: Build The Reservation Map

```sh
for n in $(fdtget -l $D /reserved-memory); do
        printf '%-20s %s\n' "$n" "$(fdtget -tx $D /reserved-memory/$n reg)"
done
fdtdump $D 2>/dev/null | grep -i memreserve
```

Convert each entry into a half-open interval `[base, base + size)` and check it against RAM `[0x80000000, 0x100000000)`. Confirm:

- the 24 MiB OP-TEE carveout ends exactly where the first R5F region begins (`0xa0000000`): adjacent, not overlapping
- the ten remote-core regions from `k3-am64-ti-ipc-firmware.dtsi` and the board file tile `0xa0000000`–`0xa5000000` in 1 MiB DMA + 15 MiB firmware pairs, followed by an 8 MiB `rtos_ipc_memory_region`
- the DTB header reservation block is empty; this board uses only `/reserved-memory`

## Step 5: Decode Relationships

Resolve each phandle and decode the arguments through the provider:

```sh
fdtget -tx $D /bus@f4000/serial@2800000 interrupts
fdtget -tx $D /bus@f4000/serial@2800000 power-domains
fdtget -tx $D /bus@f4000/r5fss@78000000/r5f@78000000 memory-region
fdtget -tx $D /bus@f4000/r5fss@78000000/r5f@78000000 mboxes
fdtget -tx $D /bus@f4000/cdns-usb@f900000/usb@f400000 phys
```

Explain separately what `memory-region` tells the R5F remoteproc driver and what the OP-TEE carveout, which has no consumer phandle, accomplishes. Then explain why AM64x SDHCI nodes have no `dma-coherent` while the same IP on AM65x does.

## Step 6: Break Ancestor Availability

```sh
cp $D a.dtb
fdtput -t s a.dtb /bus@f4000/r5fss@78000000 status disabled
fdtget a.dtb /bus@f4000/r5fss@78000000/r5f@78000000 status
```

The core still says `okay`. Answer:

- Is the core locally available?
- Is it effectively available, given that the cluster driver populates its cores?
- Which runtime symptom would you expect, and where would you look first?

## Step 7: Break A Cell Contract

```sh
cp $D m.dtb
fdtput -t x m.dtb /memory@80000000 reg 80000000 80000000
dtc -I dtb -O dts m.dtb >/dev/null
```

`dtc` reports `reg_format: ... property has invalid length (8 bytes) (#address-cells == 2, #size-cells == 2)`. Split the two cells according to the root two-address/two-size contract and prove why the entry is incomplete. Explain why early boot code that reads `/memory` might fail long before any driver prints an error.

## Step 8: Create A Reservation Overlap

```sh
cp $D o.dtb
fdtput -t x o.dtb /reserved-memory/optee@9e800000 reg 0 9f000000 0 1800000
```

Calculate the new OP-TEE interval and show its overlap with `memory@a0000000`. `dtc` does not check reserved-memory overlaps; identify which evidence would reveal the conflict:

- the logical `/reserved-memory` tree
- early kernel memory logs (`OF: reserved mem: ...`)
- `/proc/iomem`
- OP-TEE's own memory configuration

## Step 9: Simulate Firmware Mutation

U-Boot writes `/chosen/bootargs` (and often memory, MAC addresses, and initrd bounds) into the working FDT before booting Linux. Reproduce one such change on a copy:

```sh
cp $D firmware.dtb
fdtput -t s firmware.dtb /chosen bootargs \
        "console=ttyS2,115200n8 root=/dev/mmcblk1p2 rw rootwait"
sha256sum $D firmware.dtb
dtc -s -I dtb -O dts -o before.dts $D
dtc -s -I dtb -O dts -o after.dts firmware.dtb
diff -u before.dts after.dts
```

State which artifact is analogous to the bootloader handoff. This is why reviewing the DTS cannot prove the runtime command line.

## Completion Criteria

The lab is complete when you can provide:

- tool version, commands, warnings, and DTB hashes
- a correct cell-decoding table
- calculated RAM and reservation intervals
- resolved alias and phandle targets
- an explanation of effective availability through ancestors
- evidence for each deliberate failure and its correction
- a diff showing a boot-time `/chosen` mutation

## Authoritative References

- [Devicetree Specification: standard nodes](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)
- [Devicetree Specification: cells, phandles, and interrupts](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)

## Continue

Proceed to [Addressing And Bus Modeling](../addressing-and-bus-modeling.md).
