---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Configuration And Tailoring

## What Problem Does This Solve?

Kernel configuration selects which drivers, subsystems, filesystems, network features, debug options, security policies, tracing facilities, and platform behaviors exist in a kernel build.

For driver development, configuration answers practical questions:

- Is the driver compiled at all?
- Is it built into the kernel image or built as a loadable module?
- Are its bus, clock, regulator, pinctrl, interrupt, DMA, or firmware dependencies enabled?
- Are debug features available for diagnosis?
- Will boot-critical storage and filesystem support be available before rootfs mount?
- Does the final `.config` match the requested defconfig or fragments?

The key point: source code existing in the tree does not mean it is active. Kconfig and Kbuild decide whether it becomes part of the build.

## Core Concepts

- Kconfig
- `CONFIG_*` symbol
- boolean symbol
- tristate symbol
- `y`, `m`, and unset
- dependency
- reverse dependency
- `select`
- `imply`
- choice
- default
- `.config`
- defconfig
- board defconfig
- config fragment
- `oldconfig`
- `olddefconfig`
- `menuconfig`
- `savedefconfig`
- generated `autoconf.h`
- built-in driver
- loadable module
- debug configuration
- production configuration

## Mental Model

Kconfig describes what can be selected. Configuration inputs request values. Kconfig resolves dependencies and writes the final `.config`. Kbuild uses the final `.config` to decide what to compile.

```text
Kconfig files
+ board defconfig
+ config fragments
+ interactive choices
-> dependency resolution
-> final .config
-> generated headers
-> Kbuild object selection
-> built-in objects and modules
```

The final `.config` is the truth used by the build.

A fragment saying:

```text
CONFIG_EXAMPLE_DRIVER=m
```

is only a request. If dependencies are missing, the final `.config` may not contain that value.

## Kconfig Symbol Values

Kernel config symbols commonly appear in three forms.

### Boolean

Boolean symbols are either enabled or disabled:

```text
CONFIG_EXAMPLE_FEATURE=y
# CONFIG_EXAMPLE_FEATURE is not set
```

Example use:

```c
#ifdef CONFIG_EXAMPLE_FEATURE
pr_info("feature enabled\n");
#endif
```

Use preprocessor checks carefully. In driver code, prefer normal subsystem interfaces where possible. Configuration conditionals are useful for optional code paths, but overusing them can make code hard to read and test.

### Tristate

Tristate symbols can be built in, modular, or disabled:

```text
CONFIG_EXAMPLE_DRIVER=y
CONFIG_EXAMPLE_DRIVER=m
# CONFIG_EXAMPLE_DRIVER is not set
```

Meaning:

| Value | Meaning |
| --- | --- |
| `y` | Built into the kernel image. |
| `m` | Built as a loadable `.ko` module. |
| unset | Not built. |

Kbuild often connects this directly:

```make
obj-$(CONFIG_EXAMPLE_DRIVER) += example_driver.o
```

If `CONFIG_EXAMPLE_DRIVER=y`, the object is linked into the kernel image. If it is `m`, Kbuild builds `example_driver.ko`. If unset, the file is ignored.

### Strings, Integers, And Hex Values

Some symbols carry values:

```text
CONFIG_LOCALVERSION="-product"
CONFIG_LOG_BUF_SHIFT=18
CONFIG_PHYSICAL_START=0x1000000
```

These can alter release strings, buffer sizes, address layout, timeout values, and platform behavior. Do not treat all configuration as simple on/off selection.

## Built-In Versus Module

For driver developers, `y` versus `m` is one of the most important decisions.

| Choice | Use When | Tradeoffs |
| --- | --- | --- |
| Built-in (`y`) | Needed for early boot, rootfs access, core platform initialization, or no module loader path is available | Harder to reload during development; always present; failures can affect boot. |
| Module (`m`) | Driver can load after rootfs is available; useful for iterative development and optional hardware | Easier reload cycle; must match kernel release; needs module install and dependency metadata. |
| Disabled | Feature not needed | Smaller kernel/runtime surface; source still exists but is inactive. |

Boot-critical features are usually built in:

```text
CONFIG_MMC=y
CONFIG_EXT4_FS=y
CONFIG_BLK_DEV_INITRD=y
```

An optional sensor driver might be a module:

```text
CONFIG_IIO=y
CONFIG_I2C=y
CONFIG_EXAMPLE_TEMP_SENSOR=m
```

A module is not available until the system can access the rootfs or initramfs containing it. If the driver is required before that point, building it as `m` is usually wrong.

## Finding The Symbol For A Driver

Start from the source file:

```text
drivers/iio/temperature/example_temp.c
```

Search for the object rule:

```sh
rg "example_temp" drivers/iio
```

You might find:

```make
obj-$(CONFIG_EXAMPLE_TEMP) += example_temp.o
```

Then search for the Kconfig symbol:

```sh
rg "config EXAMPLE_TEMP|EXAMPLE_TEMP" drivers/iio
```

Example Kconfig:

```kconfig
config EXAMPLE_TEMP
    tristate "Example temperature sensor"
    depends on I2C
    select REGMAP_I2C
    help
      Enable support for the Example temperature sensor.
```

This tells you:

- the symbol is `CONFIG_EXAMPLE_TEMP`
- it can be built as a module because it is `tristate`
- it requires `I2C`
- it selects `REGMAP_I2C`

Now inspect the final config:

```sh
grep '^CONFIG_EXAMPLE_TEMP' build/.config
grep '^CONFIG_I2C' build/.config
grep '^CONFIG_REGMAP_I2C' build/.config
```

If the symbol is absent, the driver will not be built.

## Dependencies

Dependencies control whether an option can be selected:

```kconfig
config EXAMPLE_TEMP
    tristate "Example temperature sensor"
    depends on I2C && OF
```

If `I2C` or `OF` is disabled, the prompt may be hidden and a fragment requesting `CONFIG_EXAMPLE_TEMP=m` may not survive into the final `.config`.

Debug dependency problems by reading both Kconfig and final `.config`:

```sh
rg "config EXAMPLE_TEMP" -n drivers
grep '^CONFIG_I2C' build/.config
grep '^CONFIG_OF' build/.config
grep '^CONFIG_EXAMPLE_TEMP' build/.config
```

Common dependency categories for drivers:

- bus support: I2C, SPI, PCI, USB, platform bus
- Device Tree support: OF
- ACPI support
- regmap support
- GPIO, pinctrl, clock, reset, regulator frameworks
- DMA APIs or DMA engine support
- IRQ domains
- firmware loading
- networking, PHY, MDIO, MII
- IIO, hwmon, input, DRM, V4L2, ALSA, tty, GPIO, LED, RTC, watchdog subsystems

## `select` And `imply`

Kconfig has mechanisms that can enable other symbols.

Example:

```kconfig
config EXAMPLE_TEMP
    tristate "Example temperature sensor"
    depends on I2C
    select REGMAP_I2C
```

`select` forces another symbol on. It is powerful and can be surprising because it bypasses the selected symbol's normal prompt path. Kernel Kconfig style generally uses `select` for simple helper symbols and avoids using it to force complex subsystems without care.

`imply` is weaker. It suggests a default but still allows dependency resolution and user choices to matter.

When a symbol appears unexpectedly:

```sh
rg "select REGMAP_I2C|imply REGMAP_I2C" .
```

When a symbol refuses to appear:

```sh
rg "depends on .*I2C|config EXAMPLE_TEMP" drivers
```

## Defconfig, `.config`, And Fragments

### Defconfig

A defconfig is a compact baseline configuration.

Examples:

```text
arch/arm64/configs/defconfig
arch/arm64/configs/vendor_board_defconfig
```

Start a build from a defconfig:

```sh
make O=build-arm64 ARCH=arm64 vendor_board_defconfig
```

This generates:

```text
build-arm64/.config
```

The `.config` is generated build state. It is important, but it is usually not the best long-term source of truth.

### `.config`

The final `.config` records the effective result after dependencies, defaults, and user selections:

```sh
grep '^CONFIG_EXAMPLE_TEMP' build-arm64/.config
```

Use it for auditing and debugging.

Avoid manually editing it as your only record. If you do edit it during exploration, convert the result into a defconfig change or fragment that can be reproduced.

### Config Fragments

A fragment contains only the options you want to add or override:

```text
CONFIG_I2C=y
CONFIG_REGMAP_I2C=y
CONFIG_EXAMPLE_TEMP=m
CONFIG_DYNAMIC_DEBUG=y
```

Fragments are common in Yocto, vendor SDKs, and product layers because they allow product policy to sit on top of a vendor defconfig.

The risk: fragments express intent, not guaranteed result. Always inspect the final `.config`.

## Configuration Commands

### Start From A Board Defconfig

```sh
make O=build-arm64 ARCH=arm64 vendor_board_defconfig
```

### Refresh Existing Config For A New Kernel

```sh
make O=build-arm64 ARCH=arm64 olddefconfig
```

`olddefconfig` accepts defaults for new options. It is useful in noninteractive builds.

Use `oldconfig` when you want to answer new prompts interactively:

```sh
make O=build-arm64 ARCH=arm64 oldconfig
```

### Use An Interactive Editor

```sh
make O=build-arm64 ARCH=arm64 menuconfig
```

Other frontends may be available depending on host packages:

```sh
make O=build-arm64 ARCH=arm64 nconfig
make O=build-arm64 ARCH=arm64 xconfig
make O=build-arm64 ARCH=arm64 gconfig
```

### Save A Minimal Defconfig

```sh
make O=build-arm64 ARCH=arm64 savedefconfig
cp build-arm64/defconfig arch/arm64/configs/my_board_defconfig
```

Use this carefully. For a vendor BSP or product layer, the correct location may be a Yocto fragment, Buildroot board file, or SDK-specific config file rather than `arch/.../configs/`.

### Script Config Changes

The kernel tree provides `scripts/config` in many versions:

```sh
scripts/config --file build-arm64/.config --enable I2C
scripts/config --file build-arm64/.config --module EXAMPLE_TEMP
scripts/config --file build-arm64/.config --disable UNUSED_DRIVER
make O=build-arm64 ARCH=arm64 olddefconfig
```

Always run a config resolution step after scripted edits.

### Merge Fragments

Many trees include:

```text
scripts/kconfig/merge_config.sh
```

Typical pattern:

```sh
scripts/kconfig/merge_config.sh \
  -O build-arm64 \
  arch/arm64/configs/vendor_board_defconfig \
  fragments/driver-debug.config
make O=build-arm64 ARCH=arm64 olddefconfig
```

Build frameworks may have their own fragment mechanism. Use the framework's normal path when working inside Yocto, Buildroot, or a vendor SDK.

## Tailoring Strategy For Driver Development

Start small and explicit:

1. Begin from the board or product defconfig.
2. Enable only the subsystem and driver options needed for the experiment.
3. Decide built-in versus module based on boot timing.
4. Add debug options temporarily in a separate fragment.
5. Build and inspect final `.config`.
6. Test on target.
7. Remove or isolate debug options before production.

Example driver fragment:

```text
CONFIG_I2C=y
CONFIG_REGMAP=y
CONFIG_REGMAP_I2C=y
CONFIG_EXAMPLE_TEMP=m
```

Example debug fragment:

```text
CONFIG_DYNAMIC_DEBUG=y
CONFIG_DEBUG_FS=y
CONFIG_FUNCTION_TRACER=y
CONFIG_KPROBES=y
```

Keep these separate because debug features can affect performance, timing, memory footprint, attack surface, and production policy.

## Driver-Focused Configuration Workflow

Suppose you want to enable a new SPI ADC driver.

### 1. Find The Driver

```sh
rg "Example ADC|example_adc|config .*ADC" drivers/iio
```

Suppose you find:

```text
drivers/iio/adc/example-adc.c
drivers/iio/adc/Kconfig
drivers/iio/adc/Makefile
```

### 2. Read Kconfig

```kconfig
config EXAMPLE_ADC
    tristate "Example SPI ADC"
    depends on SPI
    depends on IIO
    select REGMAP_SPI
```

### 3. Create A Fragment

```text
CONFIG_SPI=y
CONFIG_IIO=y
CONFIG_REGMAP_SPI=y
CONFIG_EXAMPLE_ADC=m
```

### 4. Merge And Resolve

```sh
make O=build-arm64 ARCH=arm64 vendor_board_defconfig
scripts/kconfig/merge_config.sh -O build-arm64 build-arm64/.config fragments/example-adc.config
make O=build-arm64 ARCH=arm64 olddefconfig
```

Your actual command may differ if a build framework owns configuration.

### 5. Audit Final `.config`

```sh
grep -E 'CONFIG_(SPI|IIO|REGMAP_SPI|EXAMPLE_ADC)=' build-arm64/.config
```

Expected:

```text
CONFIG_SPI=y
CONFIG_IIO=y
CONFIG_REGMAP_SPI=y
CONFIG_EXAMPLE_ADC=m
```

### 6. Build And Find Output

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) modules
find build-arm64 -name '*example*adc*.ko'
```

If no `.ko` appears, check:

```sh
rg "EXAMPLE_ADC" drivers/iio/adc
grep '^CONFIG_EXAMPLE_ADC' build-arm64/.config
```

## Built-In Driver Example

Suppose the root filesystem is on eMMC. The MMC host controller, block layer, and filesystem support must be available before rootfs mount unless an initramfs handles them.

Good production shape:

```text
CONFIG_MMC=y
CONFIG_MMC_SDHCI=y
CONFIG_MMC_SDHCI_PLTFM=y
CONFIG_EXT4_FS=y
```

Risky shape without initramfs support:

```text
CONFIG_MMC=m
CONFIG_MMC_SDHCI=m
CONFIG_EXT4_FS=m
```

Symptom of the risky shape:

```text
VFS: Cannot open root device
Kernel panic - not syncing: VFS: Unable to mount root fs
```

This is not a driver probe bug. It is a configuration and boot sequencing bug.

## Module Driver Example

Suppose an optional USB sensor can appear after boot. Building it as a module can make development faster:

```text
CONFIG_USB=y
CONFIG_USB_SERIAL=m
CONFIG_USB_SERIAL_EXAMPLE=m
```

Development loop:

```sh
make O=build-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) modules
scp build-arm64/drivers/usb/serial/example.ko target:/tmp/
ssh target 'insmod /tmp/example.ko; dmesg | tail -n 50'
ssh target 'rmmod example'
```

For product packaging, install modules under `/lib/modules/<kernelrelease>/` and run dependency handling rather than copying `.ko` files manually.

## Auditing Requested Versus Final Config

Always distinguish:

```text
requested:
  fragment says CONFIG_EXAMPLE_ADC=m

resolved:
  final .config contains CONFIG_EXAMPLE_ADC=m

built:
  build output contains example_adc.ko

runtime:
  target can load or contains the driver
```

Audit commands:

```sh
grep '^CONFIG_EXAMPLE_ADC' fragments/example-adc.config
grep '^CONFIG_EXAMPLE_ADC' build-arm64/.config
find build-arm64 -name '*example*adc*.ko'
modinfo build-arm64/drivers/iio/adc/example_adc.ko
```

For built-in drivers, there may be no `.ko`. Look for object files during build or runtime evidence:

```sh
grep '^CONFIG_EXAMPLE_ADC=y' build-arm64/.config
grep example_adc build-arm64/System.map
dmesg | grep -i example
```

Not every built-in driver's function names will appear in a simple `System.map` search after compiler optimizations, but it is still a useful clue.

## Debug Configuration

Debug options are useful, but they are not free.

Common development options:

```text
CONFIG_DEBUG_FS=y
CONFIG_DYNAMIC_DEBUG=y
CONFIG_FUNCTION_TRACER=y
CONFIG_FTRACE=y
CONFIG_KPROBES=y
CONFIG_LOCKDEP=y
CONFIG_PROVE_LOCKING=y
CONFIG_KASAN=y
CONFIG_KCSAN=y
CONFIG_KMEMLEAK=y
CONFIG_DEBUG_INFO=y
```

Tradeoffs:

| Option Type | Benefit | Cost |
| --- | --- | --- |
| dynamic debug | enables targeted `pr_debug()` output | needs config support and runtime control |
| debugfs | exposes useful debug interfaces | not always acceptable in production |
| ftrace | powerful tracing | memory and overhead depending on setup |
| lockdep | catches locking bugs | significant overhead |
| sanitizers | catch memory/concurrency bugs | high overhead and bigger builds |
| debug info | better symbols | larger artifacts |

Keep debug options in a named fragment:

```text
fragments/kernel-debug.config
```

Then product builds can exclude it cleanly.

## Production Tailoring

Production configuration is not simply "debug config minus logs". It should be a deliberate policy.

Review:

- boot-critical built-ins
- module loading policy
- module signing
- debugfs exposure
- tracing availability
- kernel command line policy
- security modules
- namespace and cgroup features
- watchdog support
- panic and reboot behavior
- firmware loading path
- filesystems
- network protocol surface
- unused bus and driver families

Examples:

```text
CONFIG_MODULE_SIG=y
CONFIG_MODULE_SIG_FORCE=y
CONFIG_SECURITY=y
CONFIG_SECURITY_APPARMOR=y
CONFIG_WATCHDOG=y
CONFIG_MAGIC_SYSRQ=n
```

The right choices depend on product requirements. The important practice is to separate production policy from temporary bring-up convenience.

## Generated Configuration Headers

The final `.config` feeds generated headers such as:

```text
include/generated/autoconf.h
include/config/auto.conf
include/generated/utsrelease.h
```

Kernel code usually sees configuration through generated macros:

```c
#include <linux/kconfig.h>

if (IS_ENABLED(CONFIG_EXAMPLE_TEMP))
    pr_info("example temperature support is enabled\n");
```

Useful macros:

| Macro | Use |
| --- | --- |
| `IS_ENABLED(CONFIG_FOO)` | True for built-in or module when compiling code that can see the symbol. |
| `IS_BUILTIN(CONFIG_FOO)` | True only when `CONFIG_FOO=y`. |
| `IS_MODULE(CONFIG_FOO)` | True only when `CONFIG_FOO=m`. |

Example:

```c
if (IS_ENABLED(CONFIG_REGMAP_I2C))
    dev_dbg(dev, "regmap over i2c support is available\n");
```

These macros are better than open-coded preprocessor checks in many normal C paths because they keep code visible to the compiler.

## Configuration And Device Tree Must Agree

Device Tree can describe hardware, but it cannot make a disabled driver exist.

For a driver to probe from Device Tree, you generally need:

```text
DTS node with compatible string
node status = "okay"
driver with matching of_device_id table
driver Kconfig symbol enabled
bus/controller enabled
driver built and deployed
```

Example checks:

```sh
grep '^CONFIG_EXAMPLE_TEMP' build-arm64/.config
rg "example,temp-sensor" drivers Documentation/devicetree
find build-arm64 -name '*.dtb'
```

On target:

```sh
find /proc/device-tree -name compatible -print
dmesg | grep -i example
```

If the DTS is correct but the driver config is unset, there is no probe. If the driver is built but the runtime DTB lacks the node, there is also no probe.

## Common Failure Modes

| Symptom | Likely Configuration Cause | First Checks |
| --- | --- | --- |
| Source file exists but no object is built | Kconfig symbol unset or Kbuild rule not active | `grep CONFIG_ build/.config`, inspect Makefile |
| Fragment requests option but final config lacks it | unmet dependency or symbol rename | `rg "config SYMBOL"`, inspect final `.config` |
| Driver builds as `.ko` but rootfs cannot mount | boot-critical driver selected as module | storage, filesystem, initramfs config |
| Driver probes on dev build but not production | debug/dev fragment contained required dependency | compare final `.config` files |
| `menuconfig` cannot find option | dependency hidden or wrong architecture | search Kconfig, check `ARCH` |
| Built-in driver cannot be reloaded | built-in by design | use module for iterative testing if safe |
| Module build output missing | symbol is `y` or unset, or modules target not built | inspect `.config`, run `make modules` |
| New kernel version changes prompts | old full `.config` copied blindly | use `olddefconfig`, audit diffs |

## Common Mistakes

- Checking a fragment but not the final `.config`.
- Treating `y` and `m` as interchangeable.
- Making rootfs storage support modular without an initramfs.
- Enabling a leaf driver without enabling its bus or subsystem dependencies.
- Keeping debug options mixed into product policy.
- Copying a full old `.config` across kernel upgrades without review.
- Editing generated headers instead of Kconfig inputs.
- Assuming a Device Tree node is enough to create a driver.
- Looking for a `.ko` when the driver was built in.
- Forgetting that Kconfig menus and defaults can be architecture-specific.

## Practical Checklist

For each driver you enable:

- Find the driver source file.
- Find the Kbuild object rule.
- Find the Kconfig symbol.
- Read its dependencies.
- Decide `y` versus `m`.
- Add the option in the correct defconfig or fragment source.
- Run config resolution.
- Check final `.config`.
- Build the relevant target.
- Verify output or built-in evidence.
- Deploy matching artifacts.
- Verify runtime behavior on target.

## Practice Exercises

### Exercise 1: Trace A Driver From Source To Config

Pick a driver file:

```sh
rg "obj-.*example" drivers
rg "config EXAMPLE" drivers
```

Answer:

- What is the `CONFIG_*` symbol?
- Is it boolean or tristate?
- What dependencies does it have?
- Which Makefile builds it?

### Exercise 2: Force A Missing Dependency

Create a fragment that requests a driver but omits a dependency. Resolve config and inspect the result:

```sh
grep '^CONFIG_EXAMPLE_DRIVER' fragment.config
grep '^CONFIG_EXAMPLE_DRIVER' build/.config
```

Answer:

- Did Kconfig keep or drop the request?
- Which dependency explains the result?

### Exercise 3: Compare Debug And Production Configs

Generate two configs:

```text
vendor_board_defconfig + product.config
vendor_board_defconfig + product.config + debug.config
```

Compare:

```sh
scripts/diffconfig build-prod/.config build-debug/.config
```

Answer:

- Which options are debug-only?
- Did any required driver accidentally live only in the debug config?

## Related Topics

- [Kernel Source, Build, And Tailoring](index.md)
- [Kconfig and Defconfig](../../build-systems/advanced/linux-kernel/kconfig-and-defconfig.md)
- [Configuration Fragments and Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [Kbuild Objects and Directories](../../build-systems/advanced/linux-kernel/kbuild-objects-and-directories.md)
- [Built-In Vs Module Policy](../configuration-and-platform-policy/built-in-vs-module-policy.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Config Review Workflow](../configuration-and-platform-policy/config-review-workflow.md)

## Official References

- [Kconfig Language](https://docs.kernel.org/kbuild/kconfig-language.html)
- [Configuration targets and editors](https://docs.kernel.org/kbuild/kconfig.html)
- [Linux Kernel Makefiles](https://docs.kernel.org/kbuild/makefiles.html)
