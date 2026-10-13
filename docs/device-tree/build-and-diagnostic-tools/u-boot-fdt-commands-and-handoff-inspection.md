---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# U-Boot `fdt` Commands And Handoff Inspection

U-Boot's `fdt` command can inspect both the control FDT and the OS-bound working FDT ([Control, Working, SPL, And Linux Device Trees](../u-boot-and-bootloader-device-tree/control-working-spl-and-linux-device-trees.md)). Most Linux handoff diagnosis belongs on the working tree.

## Identify Both Trees

Current U-Boot command families support querying the working FDT and selecting the control FDT with `-c`:

```text
=> fdt addr
Working fdt: ...

=> fdt addr -c
Control fdt: ...
```

Exact output and options depend on the built U-Boot version. Use:

```text
=> help fdt
```

Record version/build identity before comparing behavior with documentation.

## Set The Working FDT Deliberately

```text
=> fdt addr ${fdt_addr_r}
=> fdt header
=> fdt print / model
=> fdt print / compatible
```

Do not assume `${fdt_addr_r}` currently contains the selected DTB. Prove load command, source device/path, byte count, header, and root identity.

Environment variables are policy inputs, not evidence by themselves. Expand them in logs:

```text
=> printenv fdtfile fdt_addr_r fdtoverlay_addr_r
```

## Capacity And Address Safety

`fdt addr` can associate a length with the working FDT and `fdt resize` can add capacity, but neither proves adjacent RAM is free. Plan the interval as described in [U-Boot Working-FDT Resize](../boot-time-mutation-and-ownership/libfdt-capacity-relocation-and-failure-atomicity.md#u-boot-working-fdt-resize), and never pass environment-controlled sizes or addresses without range validation.

## Read Before Write

```text
=> fdt print /chosen
=> fdt print /soc/serial@1000
=> fdt list /soc
```

Focused getters can place values or addresses in environment variables on supported versions:

```text
=> fdt get value serial_status /soc/serial@1000 status
=> printenv serial_status
```

Property data may be cells, strings, or bytes; U-Boot's textual rendering is not a binding decoder. Use host tools on a captured blob for exact multi-cell analysis.

## Controlled Working-Tree Writes

Common command families can:

- set a property
- create a node
- remove a property/node
- update `/chosen` and memory data
- apply an overlay

Before any write:

```text
expected prior value
authorized owner/function
validated source of new value
capacity available
failure behavior
postcondition and log event
```

Boot scripts must check command status. A partially prepared tree must not be passed to Linux because the shell continued after an error.

## Overlay Application

`fdt apply ${overlay_addr_r}` merges an overlay into the working FDT after `fdt resize`. Follow the authentication, ordering, and failure rules in [Bootloader Overlay Application And Working-FDT Safety](../u-boot-and-bootloader-device-tree/bootloader-overlay-application-and-working-fdt-safety.md); never apply Linux product overlays to the control FDT.

## Capture The Pre-Handoff FDT

The best evidence is a byte copy of the exact used extent immediately before boot. Product mechanisms vary:

- save from memory to removable storage
- upload with a network command
- expose through a debug/recovery command
- hash in place and save a development checkpoint earlier

Determine totalsize from the FDT header using supported U-Boot commands. Copy/hash only the validated blob extent, not the whole padded/reserved buffer, unless the release definition explicitly covers padding.

Record:

```text
working FDT address and validated capacity
header totalsize
base name/hash
ordered overlay names/hashes
mutation log
pre-handoff used-blob hash
capture method and output hash
```

## Compare Host And U-Boot Results

Host compose the same base and overlays:

```bash
fdtoverlay -i base.dtb -o host-merged.dtb overlay-a.dtbo overlay-b.dtbo
```

Compare host merged, U-Boot post-overlay, and U-Boot pre-handoff trees. Only the [expected boot-time changes](../foundations/source-and-binary-artifacts.md#what-the-bootloader-can-change)—memory, `/chosen`, MACs and identity, firmware reservations—should differ, and each needs an owner.

## Common U-Boot Diagnostic Traps

- inspecting the control FDT while debugging Linux handoff
- pointing `fdt addr` at an overlay or stale load buffer
- changing control-FDT properties and expecting driver-model reprobe
- growing totalsize into an unreserved adjacent image
- ignoring a failed `fdt apply` and booting the buffer
- trusting `${fdtfile}` without proving the executed boot path
- hashing allocated padding instead of the used blob
- capturing before final boot command adds `/chosen` data
- comparing an SPL DTB, U-Boot control DTB, and Linux DTB as if interchangeable

## Handoff Checklist

- Which U-Boot build and boot command path ran?
- Which address is control, and which is working?
- Which exact base artifact was loaded from where?
- Was header/totalsize valid before mutation?
- Were all buffer intervals nonoverlapping?
- Which overlays applied in what order?
- Which mutations occurred after overlays?
- What exact used blob and hash reached Linux?
- Does the captured artifact match early/runtime Linux evidence?

## Authoritative References

- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Devicetree control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [U-Boot environment command](https://docs.u-boot.org/en/latest/usage/cmd/env.html)

## Continue

Proceed to [`libfdt` Programming, Capacity, And Error Discipline](libfdt-programming-capacity-and-error-discipline.md).
