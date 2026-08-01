---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# U-Boot `fdt` Commands And Handoff Inspection

U-Boot can expose two different trees: the control FDT used by U-Boot's driver model and the working FDT prepared for the operating system. Most Linux handoff diagnosis belongs on the working tree. Editing the control tree after U-Boot has cached pointers does not safely reconfigure U-Boot.

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

The `fdt addr` command can associate a length with the working FDT in current U-Boot, and `fdt resize` can expand capacity according to version-specific semantics. Neither proves adjacent RAM is free.

Before growth:

- validate current FDT header/totalsize
- reserve a nonoverlapping destination interval
- copy/open the base into that interval
- account for overlays and later fixups
- check kernel, initrd, decompression, stack, and firmware regions

Example flow, with product-specific addresses omitted:

```text
=> fdt addr ${fdt_addr_r}
=> fdt header
=> fdt resize ${fdt_extra_space}
```

Do not pass arbitrary environment-controlled size/address values without range validation.

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

Conceptual flow:

```text
=> fdt addr ${fdt_addr_r}
=> fdt resize ${overlay_space}
=> fdt apply ${overlay_addr_r}
=> fdt print /soc/spi@2000000
```

Load and authenticate all selected overlays first, validate compatibility/order, and work on a disposable/reloadable base copy. U-Boot overlay documentation warns failed application can invalidate the involved blobs.

Applying an overlay to the control FDT is not equivalent to dynamically reprobling U-Boot devices. Apply Linux product overlays to the OS-bound working FDT unless the platform architecture explicitly defines otherwise.

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

Compare host merged, U-Boot post-overlay, and U-Boot pre-handoff trees. Expected later differences may include:

- `/memory` discovery
- `/chosen` bootargs, initrd, console, and seeds
- MAC addresses and serial identity
- secure firmware reservations

Every other difference needs an owner.

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
