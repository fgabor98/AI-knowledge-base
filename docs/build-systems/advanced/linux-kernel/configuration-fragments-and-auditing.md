---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Configuration Fragments and Auditing

## What Problem Does This Solve?

Kernel configuration fragments let you maintain board, product, feature, and debug configuration as small reviewable files instead of one large `.config`.

For embedded Linux work, fragments are essential because one product kernel often combines:

- vendor SoC defaults
- board enablement
- product-specific features
- storage and networking requirements
- debug options
- security options
- temporary bring-up options

The risk is that fragments can silently fail to produce the final configuration you expected. Auditing closes that gap.

## Core Concepts

- base defconfig
- config fragment
- final `.config`
- `olddefconfig`
- `merge_config.sh`
- unmet dependency
- symbol rename
- fragment ordering
- product configuration policy
- requested vs effective config

## Mental Model

Fragments are requests. Kconfig produces the result.

```text
base defconfig
+ board fragment
+ product fragment
+ debug fragment
-> merge requested symbols
-> Kconfig dependency resolution
-> final .config
```

The final `.config` is the source of truth for a specific build. Fragments are the maintained input.

## Why Full `.config` Files Are Hard To Maintain

A full `.config` is useful for debugging one build, but it is usually a poor long-term product input.

Problems:

- it contains thousands of inherited defaults
- it changes heavily between kernel versions
- it hides the actual product decisions
- it is difficult to review
- it makes BSP upgrades noisy
- it can accidentally pin old defaults that should change

Prefer small fragments for product intent and archive full `.config` as a release artifact.

## Fragment Categories

### Board Fragment

Board fragments enable hardware required by one board:

```text
CONFIG_MMC=y
CONFIG_MMC_SDHCI=y
CONFIG_SPI=y
CONFIG_I2C=y
CONFIG_PINCTRL_SINGLE=y
```

### Product Fragment

Product fragments enable product behavior:

```text
CONFIG_EXT4_FS=y
CONFIG_OVERLAY_FS=y
CONFIG_IPV6=y
CONFIG_NETFILTER=y
```

### Debug Fragment

Debug fragments should be easy to add and remove:

```text
CONFIG_DYNAMIC_DEBUG=y
CONFIG_DEBUG_FS=y
CONFIG_FUNCTION_TRACER=y
```

### Production Hardening Fragment

Production fragments can disable debug behavior and enable security features:

```text
# CONFIG_DEBUG_FS is not set
CONFIG_MODULE_SIG=y
CONFIG_STRICT_DEVMEM=y
```

Keep debug and production fragments separate so the build profile is visible.

## Merging Fragments Manually

The kernel tree provides `scripts/kconfig/merge_config.sh`.

Example:

```sh
make O=build ARCH=arm64 defconfig
scripts/kconfig/merge_config.sh -O build build/.config board.cfg product.cfg debug.cfg
make O=build ARCH=arm64 olddefconfig
```

Important options:

- `-O <dir>` writes output into an output directory
- `-m` merges only, without running make
- `-r` reports redundant settings

The exact options available can vary by kernel version, so check:

```sh
scripts/kconfig/merge_config.sh -h
```

## Auditing Requested Vs Final Config

After merging, check the final `.config`.

```sh
grep '^CONFIG_EXT4_FS' build/.config
grep '^CONFIG_MMC' build/.config
grep '^# CONFIG_DEBUG_FS is not set' build/.config
```

For one symbol:

```sh
scripts/config --file build/.config --state CONFIG_EXT4_FS
```

If a requested option is missing, inspect dependencies:

```sh
make O=build ARCH=arm64 menuconfig
```

Then use search in menuconfig:

```text
/
CONFIG_SYMBOL_NAME
```

Menuconfig shows dependencies, selected-by relationships, and prompt visibility.

## Common Failure: Option Requested But Missing

Example fragment:

```text
CONFIG_SOME_DRIVER=y
```

Final `.config`:

```text
# CONFIG_SOME_DRIVER is not set
```

Likely causes:

- dependency is unset
- symbol has no prompt and cannot be directly selected
- symbol name changed in this kernel version
- architecture does not support the option
- later fragment overrides it
- Kconfig choice selected another mutually exclusive option

Debug order:

1. Search for the symbol in Kconfig files.
2. Check dependencies in `menuconfig`.
3. Check merge logs.
4. Check whether the symbol exists in this kernel version.
5. Check whether another fragment overrides it.

## Common Failure: Fragment Ordering

Later fragments can override earlier fragments.

```text
base.cfg
board.cfg
product.cfg
production.cfg
```

If `debug.cfg` enables `CONFIG_DEBUG_FS=y` but `production.cfg` disables it, the final result depends on order.

Make ordering explicit in documentation, CI scripts, Yocto metadata, or build wrappers.

## Yocto Kernel Fragments

In Yocto, fragments are typically added through kernel recipes or `.bbappend` files.

Common pattern:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://board.cfg"
SRC_URI += "file://product.cfg"
```

Important checks:

```sh
bitbake -e virtual/kernel | grep '^SRC_URI='
bitbake virtual/kernel -c menuconfig
bitbake virtual/kernel -c diffconfig
```

The final config usually lives under the kernel work/build directory, not beside your fragment.

Audit:

- whether the fragment is included in `SRC_URI`
- whether the right machine is selected
- whether the expected kernel provider is selected
- whether a later metadata layer overrides the fragment
- whether `do_kernel_configme` and related tasks ran after changes

## TI Processor SDK Considerations

TI Processor SDK Linux may combine TI kernel branches, machine configuration, defconfig choices, and Yocto metadata.

For Sitara-style systems, always identify:

- SDK release
- selected machine
- kernel provider
- kernel branch and commit
- base defconfig
- TI-supplied fragments or defaults
- product fragments
- final `.config`

Do not assume a fragment is active just because it exists in a layer.

## Fragment Ownership Policy

A practical ownership model:

| Fragment Type | Owner | Contents |
| --- | --- | --- |
| vendor | SoC/BSP vendor | SoC enablement defaults |
| board | BSP/platform team | board buses, PMIC, storage, pinctrl dependencies |
| product | product team | filesystems, networking, product features |
| debug | developers | tracing, debugfs, dynamic debug, diagnostics |
| production | release owner | hardening, disabled debug features, signing policy |

This keeps review focused. A patch that changes a production hardening fragment should not be hidden inside unrelated board bring-up work.

## CI Checks

Useful CI checks:

- build final `.config`
- verify required symbols
- verify forbidden symbols
- fail on fragment merge warnings
- archive final `.config`
- archive fragment list and order
- compare config diff against previous release

Example policy check:

```sh
grep -q '^CONFIG_EXT4_FS=y' build/.config
grep -q '^# CONFIG_DEBUG_INFO_BTF is not set' build/.config
```

For more structured checks, keep expected symbols in files:

```text
required.config
forbidden.config
```

## Debugging Checklist

- Confirm the base defconfig.
- Confirm every fragment path.
- Confirm fragment order.
- Confirm the selected `ARCH`.
- Confirm the selected kernel source and branch.
- Check merge warnings.
- Check final `.config`.
- Search Kconfig dependencies.
- Check whether the symbol exists in this kernel version.
- Check whether a later layer or fragment overrides your request.

## Common Mistakes

- Treating fragments as guaranteed assignments.
- Reviewing requested config but not final config.
- Mixing debug and production settings in one file.
- Carrying a full old `.config` across kernel upgrades.
- Forgetting that Kconfig defaults can change between releases.
- Assuming Yocto included a fragment without inspecting `SRC_URI` or final `.config`.

## Related Topics

- [Kconfig and Defconfig](kconfig-and-defconfig.md)
- [Debugging Kernel Builds](debugging-kernel-builds.md)
- [Vendor Kernel Patch Management](vendor-kernel-patch-management.md)
- [Kernel Release Artifacts](kernel-release-artifacts.md)

## References

- Linux kernel Kconfig documentation
- Linux kernel `merge_config.sh`
- Yocto Project kernel development documentation
