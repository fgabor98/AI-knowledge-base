---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Config Review Workflow

## What Problem Does This Solve?

Kernel config reviews prevent accidental feature loss, debug exposure, security regressions, and unsupported product behavior.

Without a review workflow, kernel configuration drifts through:

- vendor BSP updates
- fragments added for one board bring-up
- debug options left after investigation
- security options disabled to unblock a test
- initramfs and module policy changes
- renamed Kconfig symbols during kernel upgrades
- bootloader command-line changes

The result can still boot while violating product policy.

## Core Concepts

- defconfig
- fragments
- final `.config`
- dependency resolution
- config ownership
- diff review
- audit scripts
- release gates

## Mental Model

Review the final resolved config, not only requested fragments. Kconfig dependencies can silently change the result.

```text
requested policy:
  fragments, defconfig, command line expectations

effective build:
  final .config, modules, initramfs, command line, boot logs

review:
  compare requested policy to effective build
```

## Inputs To Review

Collect:

```text
kernel source commit
vendor patch level
ARCH and CROSS_COMPILE
base defconfig
fragment list and order
build profile name
final .config
module list
initramfs manifest
kernel command line
boot log
```

Do not review a `.config` without knowing which source tree and profile produced it.

## Fragment Categories

Keep fragments small and owned:

| Fragment | Owner | Examples |
| --- | --- | --- |
| vendor | BSP vendor | SoC defaults |
| board | platform team | buses, storage, pinctrl, PMIC |
| product | product team | filesystems, protocols, product features |
| debug | developers | tracing, sanitizers, debugfs |
| security | security/release owner | module signing, hardening, LSMs |
| recovery | reliability/support | watchdog, initramfs, rescue drivers |

Avoid large unlabeled fragments named `extra.cfg` or `fixes.cfg`.

## Review Final `.config`

The final `.config` is the truth for the kernel binary.

Useful checks:

```sh
grep '^CONFIG_EXT4_FS' build/.config
grep '^CONFIG_MODULE_SIG' build/.config
grep '^# CONFIG_DEBUG_FS is not set' build/.config
```

For kernel trees with helper scripts:

```sh
scripts/config --file build/.config --state CONFIG_EXT4_FS
```

When a symbol is missing, inspect dependencies with menuconfig search:

```text
/
CONFIG_SYMBOL_NAME
```

## Required And Forbidden Lists

Keep machine-readable policy files.

`required-production.config`:

```text
CONFIG_MODULE_SIG=y
CONFIG_MODULE_SIG_FORCE=y
CONFIG_HARDENED_USERCOPY=y
CONFIG_WATCHDOG=y
```

`forbidden-production.config`:

```text
CONFIG_DEBUG_FS=y
CONFIG_KASAN=y
CONFIG_KCSAN=y
```

CI can check these against final `.config`.

Example shell shape:

```sh
grep -q '^CONFIG_MODULE_SIG=y' build/.config
grep -q '^# CONFIG_DEBUG_FS is not set' build/.config
```

For larger projects, use a real config-check script that reports all failures at once.

## Diff Review

Compare final configs:

```sh
diff -u previous.config current.config
```

Classify every meaningful change:

```text
vendor update
board hardware change
product feature
debug-only change
security policy
dependency fallout
symbol rename
unintended regression
```

Do not approve a config diff as "BSP churn" without checking security, boot, filesystem, module, and debug symbols.

## Kconfig Dependency Failures

Requested:

```text
CONFIG_DEMO_DRIVER=y
```

Final:

```text
# CONFIG_DEMO_DRIVER is not set
```

Common causes:

- dependency unset
- wrong architecture
- driver hidden behind a bus option
- symbol renamed
- choice conflict
- later fragment override
- vendor patch changed Kconfig

Review merge logs and use menuconfig search to see dependencies.

## Command Line And Initramfs Review

Config review is incomplete without boot artifacts.

Review together:

```text
final .config
kernel command line
initramfs manifest
module list
firmware list
boot log
```

Example:

```text
CONFIG_MMC=m
root=/dev/mmcblk0p2 rootwait
initramfs lacks mmc module
-> boot failure
```

The `.config` alone does not reveal that failure.

## Security Review Gate

For production:

```text
module signing policy
debugfs policy
LSM active policy
namespace/userns policy
cgroup requirements
devmem policy
BPF/perf restrictions
kernel pointer exposure policy
panic/watchdog behavior
```

Security review should approve both enabled protections and intentional exceptions.

## Debug Review Gate

For debug profiles:

```text
which bug class this profile targets
expected overhead
diagnostic filesystems
runtime debug switches
artifact retention
production exclusion rule
```

A debug profile is valid when it is intentionally debug, not when it accidentally becomes production.

## Release Artifact Checklist

Archive:

```text
Image or vmlinuz
vmlinux
System.map
modules
Module.symvers
final .config
fragment list and order
kernel command line
initramfs image and manifest
device trees
boot log
source commit and patch set
toolchain identity
```

This reduces future "cannot reproduce" failures.

## CI Workflow

Minimum CI:

```text
build final .config
fail on merge warnings
check required symbols
check forbidden symbols
archive final .config
build kernel/modules/dtbs
build initramfs
boot smoke test when hardware or emulator is available
archive boot log and /proc/cmdline
```

Better CI:

```text
config diff classification
module signing load tests
watchdog controlled test
rootfs boot dependency test
LSM/cgroup runtime checks
debug profile exclusion from release artifacts
```

## Review Template

```text
Profile:
Kernel source:
Base defconfig:
Fragments in order:
Final .config artifact:
Command line:
Initramfs:
Module policy:
Security policy:
Watchdog policy:
Debug exposure:
Required symbols checked:
Forbidden symbols checked:
Config diff approved by:
Boot evidence:
Open exceptions:
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| option requested but absent | unmet dependency or rename | final `.config`, Kconfig search |
| production has debugfs | wrong fragment/profile | fragment order |
| boot breaks after config cleanup | root driver changed to module | initramfs and module policy |
| secure boot cannot load modules | signing/key workflow missing | dmesg and key policy |
| vendor upgrade changes behavior | defaults changed | config diff |
| review misses issue | only fragments reviewed | final artifacts |

## Practice Exercises

### Exercise 1: Symbol Gate

Create `required-production.config` and `forbidden-production.config`, then write a small check that reports all mismatches against final `.config`.

### Exercise 2: Diff Classification

Compare two final configs from different releases. Classify each major difference as:

```text
intended
dependency fallout
vendor default change
security-sensitive
debug leakage
unknown
```

### Exercise 3: Boot Artifact Review

Review a release bundle and confirm it contains every artifact needed to reproduce and debug the kernel.

## Debugging Checklist

- Check unmet dependencies.
- Check fragment ordering.
- Check vendor defconfig changes.
- Keep critical options under automated review.
- Review final `.config`, command line, and initramfs together.
- Archive boot logs and release artifacts.
- Separate debug, production, and service profiles.
- Treat unknown config diffs as blockers until classified.

## Related Topics

- [Configuration Fragments And Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [Debug Vs Production Configs](debug-vs-production-configs.md)
- [Module Signing And Hardening](module-signing-and-hardening.md)
- [Kernel Command Line Policy](kernel-command-line-policy.md)
- [Initramfs Options](initramfs-options.md)

## Official References

- [Kconfig Language](https://docs.kernel.org/kbuild/kconfig-language.html)
- [Kbuild](https://docs.kernel.org/kbuild/index.html)
