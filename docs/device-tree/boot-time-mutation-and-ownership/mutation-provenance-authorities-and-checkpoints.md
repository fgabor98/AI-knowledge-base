---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Mutation Provenance, Authorities, And Checkpoints

A boot-time fixup is a transformation of an already selected tree. To debug or secure it, preserve the input, function, ordering, evidence, and output. Without checkpoints, a final property cannot tell you whether it came from source, overlay, firmware, board code, environment, or a previous stale buffer.

## Build A Mutation DAG

The transformation is often a directed acyclic graph rather than a single chain:

```text
base DTB hash ----------------------+
board identity record -> overlay A -+
module manifest ------> overlay B --+-> merged hardware tree
DRAM training result -------------->+-> memory fixup
secure firmware reservation list -->+-> reserved-memory fixup
boot configuration ---------------->+-> /chosen
RNG output ------------------------>+-> seed properties
```

Each arrow needs a defined interface. If two branches write the same property, define precedence or reject the combination.

## Separate Authorities

Typical authorities include:

- ROM or immutable boot firmware
- trusted firmware/system controller
- TPL/SPL
- U-Boot generic image code
- SoC code
- board code
- environment/boot script
- overlay selection policy
- Linux early boot

Authority is not the same as implementation location. Board code may call a generic helper, but product policy owns the value. Trusted firmware may report a reservation that U-Boot serializes; secure firmware owns the extent, while U-Boot owns correct encoding.

## Classify The Data

| Class | Examples | Expected persistence |
|---|---|---|
| immutable hardware | SoC revision, fused identity | device lifetime |
| assembled product | MAC assignment, module population | manufacturing/product lifetime |
| boot discovery | trained RAM banks, bad-memory exclusion | current boot or stable platform |
| boot policy | selected slot, command line, recovery mode | current boot |
| payload placement | initrd start/end | current boot |
| ephemeral security | RNG/KASLR seed | one-time |

Do not store an ephemeral value as if it were product identity. Do not reconstruct immutable identity from a mutable environment variable.

## Define Precondition And Postcondition

Every mutator should have a contract:

```text
precondition:
  valid writable FDT
  target node exists or creation is explicitly allowed
  expected property is absent or has an approved prior value
  input is validated and authorized
  sufficient capacity is available

postcondition:
  exact property/node value encoded with correct cells
  unrelated paths unchanged
  tree passes structural and semantic checks
  mutation event recorded
```

Checking the prior value catches double application and unexpected earlier writers.

## Idempotence And Retries

Boot flows can retry, fall back, or invoke a preparation path twice. Decide whether each transform is:

- idempotent: setting the same validated value twice is safe
- replace-only: a second value must overwrite an earlier value
- append-only: duplicate application would be wrong
- one-shot: seeds or ownership transfers must never repeat

Overlay application is generally not safely idempotent. Appending to `bootargs` without recognizing an earlier append creates duplicates. Seed generation and consumption require explicit one-time handling.

## Checkpoint Strategy

Useful checkpoints are:

1. compiled and authenticated base
2. post-selection base in RAM
3. after each overlay or logical overlay group
4. after firmware/board hardware fixups
5. after OS image preparation
6. immediately before architecture handoff
7. Linux early/live tree

At each checkpoint, record:

- FDT address, `totalsize`, and allocated capacity
- SHA-256 of the serialized used blob
- selected identity and inputs
- ordered mutation IDs and return codes
- dump/copy in development or CI

Hashing the entire reserved buffer produces unstable results because unused padding can contain old data. Hash the canonical used DTB extent or an explicitly packed artifact.

## Log Without Leaking

A production mutation log can use stable event IDs:

```text
DTM-010 base verified hash=...
DTM-120 overlay applied id=module-x version=3
DTM-210 memory updated banks=2 total=...
DTM-310 initrd range set size=...
DTM-900 final tree hash=...
```

Avoid printing random seeds, secret serial data, private provisioning records, or secure memory contents. Public MAC addresses and product serials may still be privacy-sensitive in exported logs.

## Failure Policy

Classify changes:

- **mandatory**: invalid memory map, secure reservation, required identity, or required overlay aborts normal boot
- **optional**: absent optional module can omit its overlay if absence is itself trustworthy
- **recoverable**: enter an authenticated recovery configuration
- **degradable**: disable a device with an explicit safe reason

Never continue with a partially applied overlay, truncated reservation, or unvalidated memory size. “Best effort” is not safe for ownership boundaries.

## Reproducibility Manifest

For a field issue, collect:

```text
bootloader and firmware versions
base DTB name and hash
ordered overlay names and hashes
board/module identity records and validation status
DRAM and secure-firmware results
environment/configuration inputs
mutation implementation version
final DTB hash
```

Sensitive raw values can be held in privileged diagnostics while public reports use normalized IDs.

## Authoritative References

- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Devicetree Control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot `bootm` command](https://docs.u-boot.org/en/latest/usage/cmd/bootm.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)

## Continue

Proceed to [Libfdt Capacity, Relocation, And Failure Atomicity](libfdt-capacity-relocation-and-failure-atomicity.md).
