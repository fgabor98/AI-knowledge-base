---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Device Tree Artifact Provenance And Diagnosis Lab

This lab follows one board description through Kbuild, packaging, FIT selection, overlay application, a U-Boot fixup, and Linux handoff. Several stages are wrong. Your job is to preserve evidence and identify each first divergence rather than repeatedly editing DTS until the symptom changes.

## Incident

Falcon revision B should expose an SPI temperature sensor. After an update:

- Linux reports model `Acme Falcon revision A`
- the sensor node is absent
- U-Boot prints an overlay error but still boots
- the engineer insists the newly built revision-B DTB contains the node
- a firmware helper sometimes crashes while adding `/chosen/acme,boot-slot`

The system uses an A/B root filesystem and one FIT image containing kernel, two base DTBs, and one overlay.

## Source Layout

```text
arch/arm64/boot/dts/acme/
  Makefile
  ax9.dtsi
  falcon-common.dtsi
  falcon-reva.dts
  falcon-revb.dts
  falcon-temp.dtso
packaging/fit/falcon.its
packaging/build-fit.sh
```

Makefile excerpt:

```make
dtb-$(CONFIG_ARCH_ACME) += falcon-reva.dtb
dtb-$(CONFIG_ARCH_ACME) += falcon-revb.dtb
dtb-$(CONFIG_ARCH_ACME) += falcon-temp.dtbo
```

Revision B excerpt:

```dts
/dts-v1/;

#include "ax9.dtsi"
#include "falcon-common.dtsi"

/ {
        model = "Acme Falcon revision B";
        compatible = "acme,falcon-revb", "acme,falcon";
};

&spi2 {
        status = "okay";
};
```

Overlay excerpt:

```dts
/dts-v1/;
/plugin/;

&expansion_spi {
        temperature-sensor@0 {
                compatible = "acme,temp100";
                reg = <0>;
                spi-max-frequency = <1000000>;
        };
};
```

`falcon-common.dtsi` labels SPI2 as `expansion_spi`. Release bases are supposed to retain symbols.

## Build And Packaging Environments

The engineer builds:

```bash
make O=out/debug ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  W=1 arch/arm64/boot/dts/acme/falcon-revb.dtb
```

The packaging job exports:

```text
KBUILD_OUTPUT=out/release
FIT_OUTPUT=artifacts/falcon.itb
```

`packaging/build-fit.sh` copies from `${KBUILD_OUTPUT}`. `out/release` was last built two weeks ago, before the temperature expansion interface was enabled and before symbol generation was added to the base build.

## FIT Configuration

Conceptual `falcon.its` fragment:

```dts
configurations {
        default = "conf-reva";

        conf-reva {
                kernel = "kernel";
                fdt = "fdt-reva";
        };

        conf-revb {
                kernel = "kernel";
                fdt = "fdt-revb";
                fdt-overlays = "fdt-temp";
        };
};
```

Board revision comes from a CRC-protected EEPROM. The failing unit reports a CRC error. Current policy silently selects the FIT default.

## U-Boot Log

```text
Board EEPROM CRC invalid
Using default FIT configuration: conf-reva
Working FDT loaded at 0x88000000, size 0x0000a420
Applying falcon-temp.dtbo
failed on fdt_overlay_apply(): FDT_ERR_NOTFOUND
Continuing boot
Added /chosen/acme,boot-slot = "B"
Booting Linux ...
```

Assume the script tries the overlay even though the selected configuration is revision A.

## Collected Hashes

```text
out/debug/.../falcon-revb.dtb             D_DEBUG
out/release/.../falcon-revb.dtb           D_STALE
FIT extracted fdt-revb                    D_STALE
FIT extracted fdt-reva                    D_REVA
U-Boot working FDT immediately after load D_REVA
U-Boot pre-handoff FDT                    D_HANDOFF
```

The exact hash differs between `D_REVA` and `D_HANDOFF` because `/chosen/acme,boot-slot` was added.

## Firmware Helper

```c
int add_boot_slot(void *fdt, const char *slot)
{
        int chosen;
        int len;
        const char *old_bootargs;

        chosen = fdt_path_offset(fdt, "/chosen");
        old_bootargs = fdt_getprop(fdt, chosen, "bootargs", &len);

        fdt_setprop_string(fdt, chosen, "acme,boot-slot", slot);
        log_info("old bootargs: %s", old_bootargs);

        return 0;
}
```

The caller passes a tightly packed build DTB in a buffer whose mapped length equals the file length.

## Lab Objectives

Produce:

1. the source-to-handoff checkpoint ledger
2. exact commands to inspect preprocessing and Kbuild provenance
3. binary queries proving each artifact's identity and symbol state
4. overlay reproduction and failure classification
5. a safe U-Boot inspection/capture plan
6. a corrected `libfdt` helper design
7. root causes and recovery policy
8. CI assertions preventing recurrence

## Task 1: Freeze Evidence

Before rebuilding, list every artifact and log to copy immutably. Include the FIT, extracted components, both output trees, overlay, U-Boot log/environment, EEPROM bytes/CRC result, pre-handoff capture, and Linux evidence.

Explain why running the packaging job or `fdtput` on the failing DTB before capture would destroy provenance.

## Task 2: Trace The Build Rule

Write commands to:

- find the revision-B source and Makefile target
- run the exact target with `V=1`
- locate generated `.cmd` records
- identify all DTSI/generated prerequisites
- capture the exact preprocessor invocation
- produce a diagnostic preprocessed DTS
- confirm `-@` or equivalent symbol generation was used
- run ordinary, `W=1`, `W=2`, and schema checks at appropriate scopes

State why the debug output proves nothing about `${KBUILD_OUTPUT}` used by packaging.

## Task 3: Inspect Binary Artifacts

For each of `D_DEBUG`, `D_STALE`, `D_REVA`, and `D_HANDOFF`, query or inspect:

- root `model`
- root `compatible`
- `/soc/spi@.../status`
- `__symbols__/expansion_spi`
- `/chosen/acme,boot-slot`
- memory reservation block

Choose `fdtdump`, `fdtget`, or `dtc` for each question and explain the choice.

Create a table:

| Artifact | Model | SPI status | symbol exported | boot slot | Interpretation |
|---|---|---|---|---|---|
| D_DEBUG |  |  |  |  |  |
| D_STALE |  |  |  |  |  |
| D_REVA |  |  |  |  |  |
| D_HANDOFF |  |  |  |  |  |

## Task 4: Prove Packaging Divergence

Use FIT tooling appropriate to the installed U-Boot tools to list and extract `fdt-reva`, `fdt-revb`, and `fdt-temp`. Record component hashes and compare with both build trees.

Do not assume component index numbers from documentation; obtain them from the actual image listing and local `dumpimage --help`/`mkimage` version.

Identify the first checkpoint where the engineer's intended `D_DEBUG` chain diverges.

## Task 5: Reproduce Overlay Behavior

Run host-side composition against:

1. `D_DEBUG` revision B
2. `D_STALE` revision B
3. `D_REVA`

Use pristine inputs for each:

```bash
fdtoverlay -i base.dtb -o merged.dtb falcon-temp.dtbo
```

Before applying, inspect overlay `__fixups__` and each base's `__symbols__`. Classify failures as compile, external resolution, merge, schema, or hardware conflict.

Explain why ignoring `FDT_ERR_NOTFOUND` and booting is unsafe even if the base remained usable in this observed run.

## Task 6: Inspect U-Boot Correctly

Design a command sequence to:

- display U-Boot version and boot-selection variables
- identify control and working FDT addresses
- validate working header and model
- inspect `expansion_spi` target/symbol evidence where supported
- inspect `/chosen` before and after the helper
- obtain totalsize and hash the used blob
- capture the exact pre-handoff FDT

State which commands must never be used on the control FDT in this diagnosis and why.

## Task 7: Review The Helper

Find every defect in `add_boot_slot()`:

- validation
- path lookup
- property lookup and string bounds
- capacity
- return-code handling
- pointer lifetime
- error reporting
- failure atomicity
- trust/range checking of `slot`

Sketch a safe caller/helper sequence. Do not assume `slot` is a valid C string or approved value merely because its type is `const char *`.

## Task 8: Produce The Root-Cause Chain

Separate at least:

- build output correctness
- packaging input selection
- board/FIT configuration selection
- overlay/base compatibility
- boot-script failure policy
- helper memory safety

For each, state owner, correction, and regression test.

## Task 9: Build CI Gates

Define gates for:

- source/build provenance
- warning and schema validation
- base symbol contract
- FIT component identity
- EEPROM failure policy
- every supported base/overlay composition
- U-Boot pre-handoff assertions
- `libfdt` unit/error tests
- release manifest and rollback

## Reference Analysis

### Evidence Freeze

Preserve read-only copies and hashes of:

```text
source commit and dirty status
out/debug and out/release target DTBs plus .cmd records
FIT image and signature metadata
extracted FIT DTBs/DTBO
boot script/environment and expanded variables
EEPROM raw bytes, expected format, and CRC result
U-Boot version/configuration/log
working FDT immediately after selection
working FDT after overlays
pre-handoff FDT after all fixups
Linux early log and live-tree capture
firmware helper binary/source version
```

Repackaging can overwrite the evidence that `${KBUILD_OUTPUT}` pointed at stale output. In-place mutation erases the original hash and prior value.

### Build Provenance Commands

```bash
rg --files arch/arm64/boot/dts/acme | rg 'falcon-revb\.dts$'
rg -n 'falcon-revb\.dtb|falcon-temp\.dtbo' arch/arm64/boot/dts/acme

make O=out/debug ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1 \
  arch/arm64/boot/dts/acme/falcon-revb.dtb

find out/debug/arch/arm64/boot/dts -name '*.cmd' -print
rg -n 'falcon-revb|falcon-common|ax9\.dtsi' \
  out/debug/arch/arm64/boot/dts -g '*.cmd'

make O=out/debug ARCH=arm64 W=1 dtbs
make O=out/debug ARCH=arm64 W=2 dtbs
make O=out/debug ARCH=arm64 dt_binding_check
make O=out/debug ARCH=arm64 W=1 dtbs_check
```

Copy the exact preprocessor command from `V=1` or generated command record and redirect only its diagnostic output to `falcon-revb.preprocessed.dts`. Search it for the model, SPI target, sensor contract, and expanded constants.

Inspect `V=1` for symbol flags and prove the built DTB contains `/__symbols__/expansion_spi`. The debug build is correct only at C2 in `out/debug`; packaging explicitly consumes a different C2 tree, `out/release`.

### Binary Inspection Plan

Focused strings/cells:

```bash
fdtget artifact.dtb / model
fdtget artifact.dtb / compatible
fdtget artifact.dtb /soc/spi@2000000 status
fdtget artifact.dtb /__symbols__ expansion_spi
fdtget artifact.dtb /chosen acme,boot-slot
```

Use exact paths obtained from `fdtget -l` or a dump; do not guess the SPI path. A missing optional diagnostic property is an expected nonzero result to classify, not an empty string.

Use `fdtdump` for header/reservation map and full symbol inspection. Use sorted `dtc` decompilation for broad semantic diff. Use `fdtget` for precise assertions.

Expected table:

| Artifact | Model | SPI status | symbol exported | boot slot | Interpretation |
|---|---|---|---|---|---|
| D_DEBUG | revision B | okay | yes | absent | intended current build |
| D_STALE | older revision B identity | old state | no | absent | stale packaging input |
| D_REVA | revision A | base-defined | no/old contract | absent | FIT default selected |
| D_HANDOFF | revision A | unchanged by failed overlay | no/old contract | B | selected wrong base, then helper mutation |

Exact old SPI state depends on the preserved artifact; query rather than assume.

### Packaging Divergence

List the FIT and extract each component with the installed U-Boot tooling. After extraction:

```bash
sha256sum \
  out/debug/arch/arm64/boot/dts/acme/falcon-revb.dtb \
  out/release/arch/arm64/boot/dts/acme/falcon-revb.dtb \
  extracted-fdt-revb.dtb
```

`extracted-fdt-revb` equals `D_STALE`, so the first divergence from intended C2 occurs at packaging input selection. Fix the build contract so packaging receives an explicit immutable artifact path/hash from the same job, not a mutable ambient `${KBUILD_OUTPUT}`.

### Overlay Reproduction

Expected:

- current symbol-bearing `D_DEBUG` revision B: external target resolves; merged result still needs schema/hardware validation
- `D_STALE`: missing `expansion_spi` symbol, so external resolution fails
- `D_REVA`: missing/unsupported expansion ABI, so product policy should reject before application; generic resolution likely fails

Inspect:

```bash
fdtdump falcon-temp.dtbo
fdtdump candidate-base.dtb
```

The overlay's `__fixups__` names `expansion_spi`; the failing bases lack the matching `__symbols__` property. `FDT_ERR_NOTFOUND` is a resolution/target error.

The script must stop normal boot or load a fully defined compatible recovery composition. U-Boot documents that a failed overlay application can invalidate involved blobs; observed survival once is not a safety guarantee.

### U-Boot Inspection Plan

Version-dependent conceptual sequence:

```text
=> version
=> printenv fdtfile fdt_addr_r fdtoverlay_addr_r
=> fdt addr -c
=> fdt header
=> fdt addr ${fdt_addr_r}
=> fdt header
=> fdt print / model
=> fdt print / compatible
=> fdt print /__symbols__ expansion_spi
=> fdt print /chosen
```

Use `help fdt` to select supported focused getters/header-field commands. Derive the validated used size, hash exactly that interval with the platform's hash command, then save/upload that interval through a product-supported method immediately before Linux handoff.

Do not use `fdt set`, `fdt rm`, `fdt mknode`, `fdt resize`, or `fdt apply` against the control FDT. U-Boot driver model can retain pointers/state derived from it; changing bytes is not safe reprobe.

### Safe Helper Design

Defects:

- no containing-buffer length or header/full validation
- negative `chosen` offset passed onward
- `old_bootargs` and `len` not checked
- `%s` assumes bounded NUL termination
- no capacity expansion despite tightly packed input
- mutation return ignored
- `old_bootargs` pointer used after structural mutation can invalidate it
- no validation that `slot` is bounded and one of approved values
- always returns success
- mutates original rather than a disposable/publish-on-success buffer

Safe sequence:

```text
validate caller-provided buffer length and FDT structure
validate slot length/content against {"A", "B"}
open immutable source into a separately allocated larger destination
look up /chosen and check result
read old bootargs; validate/copy bounded data if logging is authorized
set acme,boot-slot; check return and log fdt_strerror on failure
discard all prior pointers/offsets and re-look up /chosen
read back exact property and validate postcondition
on any failure discard destination
on success validate final tree, then publish destination atomically
```

If `/chosen` may legitimately be absent, create it under a documented policy and check each operation; otherwise fail.

### Root Causes And Owners

| Layer | Root cause | Owner/correction | Regression test |
|---|---|---|---|
| build | debug build is correct but not release input | build/release: pass explicit artifact manifest | package input hash must equal same-job C2 hash |
| packaging | stale `out/release` embedded | packaging: remove ambient output-tree dependency | extract every FIT component and compare hash |
| selection | invalid EEPROM silently falls to rev A | boot policy: authenticated recovery or hard stop | corrupt CRC selects only defined safe recovery |
| overlay | attempted on base without exported ABI | manifest/boot script: gate by base ABI | wrong-base negative composition test |
| failure policy | continued after mandatory apply error | boot script: discard/reload and recover | injected apply failure never boots uncertain tree |
| helper | unchecked capacity/errors and stale pointer | firmware: defensive working-copy implementation | no-space, missing-node, bad-string, and mutation tests |

### CI Gates

- build DTBs/DTBOs from a clean declared output tree with `V=1` evidence
- run required `W=1`/`W=2`, `dt_binding_check`, and `dtbs_check` scopes
- assert base model, compatible, critical statuses, and exported symbol surface
- produce a machine-readable C2 artifact manifest with hashes
- consume only manifest paths in FIT packaging
- extract FIT components and compare to manifest hashes
- test valid EEPROM identities and every invalid/corrupt path
- host-compose every supported base/overlay order and reject every unsupported one
- schema-validate and resource-check merged trees
- compare U-Boot pre-handoff assertions with host expectations
- unit-test helper errors, capacity, pointer invalidation, and publish-on-success
- test A/B rollback with compatible FIT/base/overlay/kernel sets

## Completion Criteria

You have completed the lab when you can identify the packaging step—not the DTS edit—as the first intended-artifact divergence, then independently diagnose wrong FIT selection, overlay incompatibility, unsafe continuation, and the `libfdt` lifetime bug.

## Authoritative References

- [Linux Kbuild documentation](https://docs.kernel.org/kbuild/)
- [Linux Devicetree schema testing](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [U-Boot FIT documentation](https://docs.u-boot.org/en/latest/usage/fit/index.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree compiler and `libfdt` source](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Next Step

Continue with [Runtime Inspection](../runtime-inspection.md), applying the same checkpoint discipline to `/proc/device-tree`, sysfs, device-model state, and driver probe evidence.
