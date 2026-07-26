---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# TPL, SPL, SRAM Budgets, And Multi-DTB Selection

TPL and SPL run with less memory, fewer drivers, and less recovery capability than U-Boot proper. Their DT design is inseparable from the complete image and runtime memory budget. A DTB that fits in flash can still collide with BSS, stack, heap, or the next-stage load buffer in SRAM.

## Draw The Early Memory Map

For each phase, record half-open intervals:

```text
ROM load window
phase text + read-only data
initialized data
BSS
malloc arena
global data
stack and worst-case growth
phase DTB
bloblist/handoff structures
next-stage image buffer
hardware-reserved SRAM
```

Use linker-map symbols and binman output rather than estimating from file size. A binary may omit BSS bytes that occupy runtime memory. Appending a DTB immediately after a binary without accounting for BSS can let zero-initialization overwrite it; binman's phase-specific entries include mechanisms for this layout.

## Code Size And Runtime Size Are Different

Track at least:

| Budget | Evidence |
|---|---|
| boot-media image limit | ROM/SoC format and packaged image size |
| load window | ROM destination and authentication header rules |
| linked region | ELF/map addresses for text, data, and BSS |
| runtime allocations | malloc, stack high-water, driver-model objects |
| DTB expansion | whether the phase edits its tree |
| next-stage load | compressed and decompressed destinations |

A successful link only proves static sections fit the linker script. It does not prove runtime stack and heap safety.

## Keep The Phase Tree Minimal But Complete

Retain the dependency closure needed to:

1. identify the board when required
2. initialize foundational clocks, pins, power, and timers
3. initialize enough RAM for the next phase
4. reach the selected boot medium
5. authenticate and load the next image
6. transfer a defined handoff state

Console is valuable during development but can be optional or policy-controlled in production. Network, USB, filesystem, and shell support add size and input surface. Make recovery requirements explicit rather than accreting features.

## One Early DTB Versus Several

A single small SPL DTB is often preferable across similar boards if early hardware is compatible. Multi-DTB support is justified when SPL must know materially different pre-RAM details such as:

- DRAM topology or timing not discoverable safely
- boot-media pin or controller differences
- PMIC/regulator differences
- early board wiring needed before U-Boot proper
- SoC/package variants sharing one binary

Do not package multiple trees merely because Linux has multiple board DTBs. Differences that matter only after DRAM can wait for U-Boot proper or OS selection.

## SPL Multi-DTB Selection

U-Boot can package multiple SPL DTBs and invoke platform selection logic. Depending on configuration, SPL may start with a generic tree and switch after obtaining board identity.

The selection input must be available before the selected tree's devices are required. Examples include:

- SoC identification registers
- straps
- OTP/fuses
- an EEPROM reachable through a generic early path
- a signed handoff from an earlier stage

Avoid a circular dependency:

```text
need selected DTB to configure I2C
  -> need I2C to read EEPROM board ID
  -> need board ID to select DTB
```

Break it with a compatible generic early configuration, immutable identity, or a prior trusted stage.

## Selection Is A Trust Decision

Choosing the wrong early DTB can:

- apply incompatible DRAM timing
- enable a voltage rail at the wrong level
- select an unsafe pin mux
- load from an unintended device
- choose a weaker verification policy

Validate board IDs, handle unknown revisions safely, and include selection metadata in measured or verified state where the security design requires it. Environment variables from writable storage are usually unsuitable as the sole authority for safety-critical pre-RAM selection.

## Handoff Between Phases

Define what each phase passes:

- selected board identity and confidence/source
- control-DTB choice
- DRAM banks and training results
- boot reason and reset cause
- verified-boot state
- firmware component locations
- console/log buffer
- hardware ownership already established

U-Boot can use bloblist or platform-specific structures for cross-phase data. Avoid rediscovering mutable hardware identity differently in every stage.

If the next phase uses another DTB, decide which runtime discoveries must be reflected in it and who owns that mutation. The following roadmap module treats mutation provenance in depth.

## Budget Regression Controls

Automate thresholds for:

```sh
size build/spl/u-boot-spl
size build/tpl/u-boot-tpl
stat -c '%s %n' build/spl/u-boot-spl.bin build/spl/u-boot-spl.dtb
```

Also archive linker maps and binman map files. Set both absolute hardware limits and smaller warning budgets to preserve future security and recovery headroom.

Test worst-case configuration, not only the smallest board. Multi-DTB FIT growth, key insertion, debug strings, and new providers can push an image beyond a ROM read limit without changing application logic.

## Diagnose Early Failures

When there is no console:

1. verify ROM loaded the expected bytes at the expected address
2. inspect authentication/boot-status registers
3. confirm entry point and phase image format
4. use a GPIO or trace point at carefully chosen milestones
5. inspect link and package maps for overlap
6. confirm the phase DTB magic, total size, and location
7. compare the filtered tree with required driver closure
8. check stack/heap and BSS clearing

Do not begin by adding a large debug subsystem to an image already near its SRAM limit.

## Authoritative References

- [U-Boot generic SPL framework](https://docs.u-boot.org/en/latest/develop/spl.html)
- [U-Boot Devicetree Control: SPL/TPL and multi-DTB](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot binman entry documentation](https://docs.u-boot.org/en/latest/develop/package/entries.html)
- [U-Boot bloblist documentation](https://docs.u-boot.org/en/latest/develop/bloblist.html)

## Continue

Proceed to [FIT Configurations, DTB Selection, And Verified Boot](fit-configurations-dtb-selection-and-verified-boot.md).
