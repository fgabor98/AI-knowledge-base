---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Versioned Release Sets, Compatibility, And Rollback

A DTB is an ABI consumer and provider inside a larger boot contract. Production updates must activate a tested set of boot firmware, kernel, DTB/DTBO, modules, device firmware, initramfs, and userspace. Valid signatures do not make arbitrary cross-version combinations compatible.

## Define The Release Set

Give every tested composition an immutable identity:

```text
release_set: prod-42
security_version: 9
boot_firmware: tiboot3.bin, tispl.bin, u-boot.img (am62x_lpsk) sha256:...
kernel: 6.19.0
base_dtb: ti/k3-am62-lp-sk.dtb sha256:...
overlays:
  - ti/k3-am62x-sk-hdmi-audio.dtbo sha256:...
modules: modules.squashfs sha256:...
device_firmware: ti-sysfw, OP-TEE, TF-A (inside tispl.bin) sha256:...
initramfs: initramfs-42.cpio.gz sha256:...
rootfs: system-b-42 root-hash:...
board_compatibility: AM62-LP-SKEVM
```

The release-set ID must map to exact hashes, not mutable filenames like `latest.dtb`.

## Model Compatibility Directions

Compatibility is not automatically symmetric:

- an old DTB with a new kernel may work when bindings remain backward compatible
- a new DTB may use properties or compatibles an old kernel does not understand
- a new bootloader may add fixups an old kernel tolerates, while an old bootloader may omit data a new platform flow requires
- out-of-tree modules usually require a matching kernel build/ABI
- device firmware may depend on a driver and DT property set
- an overlay may depend on base-tree symbols or structure outside a stable ABI

Test declared combinations; do not infer them from version ordering.

## Use Compatibility Windows Deliberately

For staged updates, define a matrix:

| Boot firmware | Kernel set | DT set | Result |
|---|---|---|---|
| old | old | old | supported rollback baseline |
| old | new | new | required intermediate state? |
| new | old | old | required after firmware-first update? |
| new | new | new | target state |
| either | mixed old/new | explicit | reject or test |

If no safe intermediate combination exists, components must update atomically from the boot policy's perspective—for example by selecting all slot-local payloads through one signed configuration.

## Keep Slot-Local Components Together

A robust A/B layout commonly stores or addresses these per slot:

- signed boot container/configuration
- kernel
- DTB and overlays
- initramfs
- modules
- root filesystem integrity metadata

The active slot selector should choose a complete release set. Storing one global mutable DTB beside two kernels invites cross-slot mixing during partial update or rollback.

Some firmware and trust anchors cannot be duplicated. Treat them as compatibility-critical shared components and prove their forward/backward window before changing them.

## Distinguish Three Version Concepts

### Release version

Human/product identifier used for support and rollout.

### Compatibility version

Expresses a contract boundary such as overlay ABI, firmware protocol, or board support range.

### Security version

Monotonic policy value used to reject known-vulnerable authorized releases.

Do not derive anti-rollback solely from semantic version strings. Use an unambiguous integer under signed policy and protected minimum state.

## Separate Rollback Mechanisms

### Functional rollback

Return to the previously bootable slot when health checks fail. This preserves availability.

### Security anti-rollback

Reject releases below a protected security floor. This prevents booting a known-vulnerable but correctly signed image.

They can conflict: if the new release raises the protected floor before proving healthy, the device may be unable to boot its old slot. Advance the floor only after the trial slot is accepted and an authorized recovery version meets it; [Write Inactive, Verify, Then Select](field-updates-recovery-and-key-rotation.md#write-inactive-verify-then-select) and [Anti-Rollback Transaction](field-updates-recovery-and-key-rotation.md#anti-rollback-transaction) give the full sequence.

## Preserve Binding Compatibility And Gate Activation

Release sets inherit the binding ABI rules in [Backward Compatibility, Deprecation, And Migration](../binding-design-and-stable-abi/backward-compatibility-deprecation-and-migration.md) and must declare overlay/base compatibility. Authentication must not become an excuse to ship a lockstep-only DT ABI without understanding recovery and staged updates. Reaching userspace is insufficient for acceptance; run the [DT-aware health checks](field-updates-recovery-and-key-rotation.md#make-health-dt-aware) before the trial-attempt budget expires and never mark success early.

## Release-Set Acceptance Contract

```text
identity: exact manifest digest and signed configuration
board scope: model/revision/options
minimum boot firmware:
maximum/known incompatible firmware:
kernel-DT compatibility policy:
overlay ABI/version:
device firmware protocol:
trial attempt budget:
health checks:
commit point:
security-floor advancement point:
rollback target:
recovery target and key:
```

## Failure Classification

| Symptom | Likely release-set failure |
|---|---|
| signature succeeds, early kernel crash | authentic but incompatible DT/kernel or bad fixup |
| old slot boots with new global DTB and fails | slot components were mixed |
| trial boots, network absent, then marked good | health contract was incomplete |
| fallback rejected after floor update | anti-rollback advanced before recoverability was proven |
| one board revision fails | manifest scope or variant selection was wrong |
| overlay resolves but peripheral regresses | structural resolution was mistaken for semantic compatibility |

## Further Reading

- [Binding Design And Stable ABI](../binding-design-and-stable-abi.md)
- [Base Compatibility, Versioning, And Overlay ABI](../overlays-in-depth/base-compatibility-versioning-and-overlay-abi.md)
- [Field Updates, Recovery, And Key Rotation](field-updates-recovery-and-key-rotation.md)
