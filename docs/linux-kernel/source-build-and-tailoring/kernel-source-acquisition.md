---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Source Acquisition

## What Problem Does This Solve?

Driver work must start from a kernel source tree that matches the system you are targeting closely enough for APIs, configuration, generated headers, exported symbols, module versioning, and Device Tree sources to line up.

There are two different reasons to acquire kernel source:

| Goal | Source Requirement |
| --- | --- |
| Learn kernel structure or read subsystem code | Any reasonably recent upstream tree is useful. |
| Build, load, and debug a driver on a specific target | You need the exact product, vendor, distro, or BSP kernel source and build identity. |

Mixing those goals is a common beginner trap. Upstream source is excellent for learning and for understanding where Linux is going. It is not automatically the source for a vendor board image, a distro kernel, or an SDK-generated embedded image.

## Core Concepts

- upstream kernel source
- mainline release
- stable and longterm branches
- vendor BSP kernel
- distro kernel
- Yocto kernel provider
- Buildroot kernel tree
- SDK source package
- source provenance
- patch stack
- exact commit identity
- kernel release string
- local version
- kernel headers
- generated headers
- prepared build tree
- `Module.symvers`
- `/lib/modules/<kernelrelease>/build`
- `/lib/modules/<kernelrelease>/source`

## Mental Model

The useful source tree is not "a Linux kernel tree". It is the tree that answers your current question.

```text
reading a concept
  -> upstream source is often enough

debugging a product driver
  -> source must match product kernel, config, patches, compiler, and artifacts

building an external module
  -> prepared build tree must match running kernel release and exported symbols
```

A kernel API can differ across:

- upstream versions
- stable backports
- vendor BSP branches
- distro patch sets
- architecture-specific code
- enabled configuration options
- optional subsystem integrations

Before assuming an API behaves as described by upstream code or an online example, verify the actual source tree and `.config` used by the target.

## Step 1: Identify The Running Kernel

On a target board or Linux system, start with runtime evidence:

```sh
uname -a
uname -r
cat /proc/version
cat /proc/cmdline
```

Example output:

```text
6.6.32-product-g3f02c1d
```

This string is the kernel release. It usually comes from:

```text
VERSION.PATCHLEVEL.SUBLEVEL
+ EXTRAVERSION
+ CONFIG_LOCALVERSION
+ optional git-derived local version
```

It matters because modules are normally installed under:

```text
/lib/modules/<kernelrelease>/
```

Check the target:

```sh
ls -l /lib/modules
ls -l /lib/modules/$(uname -r)
```

If the directory for `uname -r` is missing, module loading and `modprobe` will already be suspicious before you inspect driver code.

## Step 2: Check Whether The Target Exposes Build Links

Many systems install links under `/lib/modules/<kernelrelease>/`:

```sh
ls -l /lib/modules/$(uname -r)/build
ls -l /lib/modules/$(uname -r)/source
```

Typical meanings:

| Path | Meaning |
| --- | --- |
| `/lib/modules/<release>/build` | Prepared build directory used for external module builds. |
| `/lib/modules/<release>/source` | Source tree, if packaged separately from generated build output. |

On a desktop distro, these links often point into installed kernel header packages. On an embedded target, they may be absent because the target rootfs does not include headers or build output.

Do not assume absence means the source does not exist. It may live in:

- vendor SDK source archives
- Yocto `tmp/work/.../linux-*/` work directories
- Buildroot `output/build/linux-*`
- a product git repository
- a CI artifact bundle
- a release engineering archive

## Step 3: Identify The Source Category

### Upstream Mainline

Use upstream mainline when you want to learn kernel structure, inspect canonical subsystem implementations, or prepare patches for eventual upstreaming.

Example:

```sh
git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linux-mainline
cd linux-mainline
git describe --tags --always
```

Strengths:

- best reference for current upstream behavior
- clean history
- official subsystem layout
- useful for learning and upstream patch preparation

Limitations:

- may not contain vendor board support
- may not match your product kernel
- may have APIs that differ from an older BSP
- may require forward-porting board DTS and drivers

### Stable And Longterm Trees

Use stable or longterm branches when your product tracks a supported kernel series.

Example:

```sh
git clone https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git linux-stable
cd linux-stable
git checkout linux-6.6.y
```

Strengths:

- receives fixes for a specific release series
- closer to production maintenance workflows than mainline
- good base for products that avoid vendor-heavy trees

Limitations:

- still may not match board vendor patches
- stable backports can differ from both original mainline and vendor code

### Vendor BSP Kernel

Use the vendor BSP kernel when the board image came from a silicon vendor, board vendor, or SDK.

Examples include SoC vendor SDKs, board support repositories, Android-derived vendor trees, and Yocto BSP layers.

Strengths:

- contains board-specific DTS files
- contains vendor drivers and backports
- usually matches the bootable reference image
- may contain product-critical patches not upstream

Limitations:

- may diverge significantly from upstream
- APIs may be older, backported, or vendor-modified
- source provenance can be unclear if releases are delivered as archives
- out-of-tree drivers may depend on vendor-only symbols

When using a vendor BSP, record:

```text
vendor name:
SDK/BSP release:
kernel repository:
branch:
commit:
patches:
defconfig:
machine or board name:
toolchain:
```

### Distro Kernel Source

Use distro kernel source when the target is running a distro-packaged kernel, such as Debian, Ubuntu, Fedora, Arch, or a vendor distribution.

Installed headers are often enough to build simple external modules against the running distro kernel:

```sh
ls /lib/modules/$(uname -r)/build
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

For deeper debugging, you may also need the distro source package and patch set, not only headers.

Strengths:

- convenient for host-side learning
- headers and module build directories are packaged
- kernel release usually matches `uname -r`

Limitations:

- distro patch sets can differ from upstream
- debug symbols may be separate packages
- source package layout is distro-specific
- not automatically useful for an embedded board running a different kernel

### Yocto Kernel Provider

In Yocto, the source is defined by metadata:

```text
MACHINE
PREFERRED_PROVIDER_virtual/kernel
kernel recipe
SRC_URI
branch
SRCREV
patches
configuration fragments
```

The source tree you inspect manually must match the provider that built the image.

Useful inspection commands from a Yocto build environment:

```sh
bitbake -e virtual/kernel | grep '^PREFERRED_PROVIDER_virtual/kernel='
bitbake -e virtual/kernel | grep '^SRCREV='
bitbake -e virtual/kernel | grep '^WORKDIR='
bitbake -e virtual/kernel | grep '^B='
```

The final source and build directories are build-specific. Do not assume that a kernel repository checked out elsewhere is the same as the one BitBake used.

### Buildroot Kernel Tree

In Buildroot, kernel source and output usually appear below:

```text
output/build/linux-*/
output/images/
```

Buildroot configuration determines:

- kernel version
- custom git repository
- patches
- defconfig or custom config
- DTBs copied to images

Check Buildroot's `.config` and board files before editing a random Linux tree.

## Step 4: Match Source To Runtime

The source is likely correct only when several pieces agree.

| Evidence | What To Check |
| --- | --- |
| `uname -r` | Matches `make kernelrelease` from the build tree. |
| `/proc/version` | Mentions expected compiler, build user, build date, or CI identity. |
| `/lib/modules/<release>/` | Exists and matches the running kernel release. |
| `modinfo driver.ko` | `vermagic` matches the target kernel. |
| `build/.config` | Contains expected driver and debug options. |
| `Module.symvers` | Exists for external module symbol versioning. |
| `git describe --dirty` | Matches release manifest or local version string. |
| boot artifacts | Image and DTB checksums match what was deployed. |

Run from the build tree:

```sh
make O=build-arm64 ARCH=arm64 kernelrelease
git rev-parse HEAD
git describe --tags --dirty --always
```

Run on target:

```sh
uname -r
cat /proc/version
ls /lib/modules/$(uname -r)
```

For a module:

```sh
modinfo ./my_driver.ko | grep -E 'filename|vermagic|depends|srcversion'
```

The key comparison:

```text
make kernelrelease == uname -r == module vermagic release component
```

Exact `vermagic` contains more than just the release string. It may include SMP, preempt, module unload, modversions, architecture, and compiler-related metadata depending on kernel configuration and version. Treat mismatch as a compatibility warning.

## Step 5: Understand Headers Versus Full Source

Kernel headers are not the same thing as a complete kernel source and build context.

| Need | Headers Enough? |
| --- | --- |
| Build a simple external module for a distro kernel | Often yes. |
| Inspect a driver implementation | No, you need source. |
| Modify an in-tree driver | No, you need source. |
| Rebuild the kernel image | No, you need source and config. |
| Rebuild DTBs | No, you need DTS sources and build rules. |
| Diagnose vendor-specific behavior | Usually no, you need vendor source and patches. |
| Build external modules with symbol versions | You need a prepared matching build tree and `Module.symvers`. |

Installed headers normally contain enough generated files for external module compilation, but they do not necessarily contain:

- full driver source
- full DTS/DTSI source
- vendor patch history
- build logs
- release manifest
- debug symbols
- the exact `.config` source inputs

For embedded work, prefer a full source plus build-output bundle.

## Step 6: Know The Generated Files

The kernel build produces generated files that source code depends on.

Common examples:

```text
include/generated/autoconf.h
include/generated/utsrelease.h
include/config/auto.conf
Module.symvers
scripts/mod/modpost
```

These are not normal source files. They come from configuration and build steps.

If you build in-tree, generated files appear inside the source tree. If you build with `O=build-arm64`, they appear in the output directory.

This distinction matters for external modules:

```sh
make -C /path/to/kernel/build M=$PWD modules
```

The `-C` path should normally be the prepared build directory, not an arbitrary clean source checkout.

For separated source and output trees:

```sh
make -C /path/to/linux-source O=/path/to/kernel-build M=$PWD modules
```

or, when the build directory has the right generated files and Makefile support:

```sh
make -C /path/to/kernel-build M=$PWD modules
```

Use the pattern expected by your kernel version and build framework, and verify it with the build logs.

## Step 7: Prepare A Source Provenance Record

For a product or lab, keep a short provenance file next to the build output:

```text
kernel_source_repo=https://example.com/vendor/linux.git
kernel_branch=vendor-6.6.y
kernel_commit=3f02c1d9c8a4d3b5e6f708192aabbccddeeff001
kernel_describe=v6.6.32-product-17-g3f02c1d
patch_stack=meta-product/recipes-kernel/linux/linux-product/*.patch
defconfig=vendor_board_defconfig
fragments=debug.config i2c-sensor.config
output_dir=/work/build/kernel/build-arm64
arch=arm64
cross_compile=aarch64-linux-gnu-
compiler=aarch64-linux-gnu-gcc 13.2.0
kernel_release=6.6.32-product-g3f02c1d
module_symvers_sha256=...
config_sha256=...
image_sha256=...
dtb_sha256=...
```

This file does not replace CI metadata, but it gives a developer enough evidence to reproduce and debug a driver experiment.

## Example: Matching A Target Board To A Vendor Kernel

On the board:

```sh
uname -r
cat /proc/version
cat /proc/cmdline
ls /lib/modules/$(uname -r)
```

Suppose `uname -r` reports:

```text
6.1.80-vendor-g9abc012
```

In the vendor source tree:

```sh
git describe --tags --dirty --always
make O=build ARCH=arm64 kernelrelease
grep '^CONFIG_LOCALVERSION' build/.config
test -f build/Module.symvers
```

If `make kernelrelease` reports:

```text
6.1.80-vendor-g9abc012
```

and the module directory on target is:

```text
/lib/modules/6.1.80-vendor-g9abc012/
```

then you likely have the correct build identity. You still need to verify DTB deployment and config, but module compatibility checks are now meaningful.

## Example: Wrong Source Tree Symptom

You build an external module:

```sh
make -C ~/linux-mainline M=$PWD modules
```

It compiles, but target loading fails:

```text
insmod: ERROR: could not insert module demo.ko: Invalid module format
```

Check:

```sh
modinfo demo.ko | grep vermagic
uname -r
dmesg | tail -n 30
```

If `vermagic` says:

```text
6.8.0 SMP preempt mod_unload aarch64
```

but `uname -r` says:

```text
6.1.80-vendor-g9abc012
```

the driver may not be the problem. The module was built against the wrong kernel.

## Example: Source Exists But The Driver Is Not Built

Finding this file:

```text
drivers/i2c/busses/i2c-example.c
```

does not mean it is part of the target kernel.

Trace it:

```sh
rg "i2c-example" drivers/i2c
rg "CONFIG_I2C_EXAMPLE|config I2C_EXAMPLE" drivers/i2c
grep '^CONFIG_I2C_EXAMPLE' build/.config
```

You need all of these to align:

```text
source file exists
Kconfig symbol exists
Kbuild rule references the object
final .config enables the symbol
build target includes built-in or module output
deployed target uses that artifact
```

## Source Acquisition By Environment

### Development Host Running Its Own Kernel

For a quick module experiment on your host:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

This is useful for learning module mechanics. It does not validate compatibility with an embedded target.

### Vendor SDK

Look for:

```text
sources/
kernel/
board-support/
linux-*/
Makefile
defconfig
patches/
toolchain/
```

Record the SDK version. Vendor SDKs often pair a specific kernel, compiler, bootloader, rootfs, and board configuration. Mixing SDK components across releases can create hard-to-read failures.

### CI Build Artifact

Ask for or archive:

```text
source commit
patch list
final .config
Image or other boot image
DTB/DTBO files
modules directory
Module.symvers
vmlinux
System.map
build log
compiler version
```

Without these, a field failure may be impossible to reproduce precisely.

## Common Failure Modes

| Symptom | Likely Source Problem | First Checks |
| --- | --- | --- |
| `Invalid module format` | Module built against different kernel release/config/arch | `modinfo`, `uname -r`, `dmesg` |
| `Unknown symbol` | Wrong or missing `Module.symvers`, missing dependency, vendor symbol mismatch | `dmesg`, `grep symbol Module.symvers`, `modprobe` dependencies |
| Driver source compiles upstream but not in BSP | Vendor kernel has older or modified API | inspect BSP headers and subsystem code |
| DTS edit has no effect | Editing a source tree not used by deployed DTB | compare DTB checksum and `/proc/device-tree` |
| Config option missing | Wrong tree, symbol renamed, dependency unmet, fragment not applied | `rg "config SYMBOL"`, final `.config` |
| Probe path differs from example | Vendor patches or different config alter subsystem behavior | compare actual source and config |
| External module build cannot find generated headers | Clean source tree used instead of prepared build tree | check `include/generated/`, `.config`, `Module.symvers` |

## Practical Checklist

Before building or debugging a driver:

- Capture `uname -r`, `/proc/version`, and `/proc/cmdline` from the target.
- Locate `/lib/modules/$(uname -r)/` on the target or rootfs.
- Find the matching source tree, not just a similar upstream version.
- Record repository, branch, commit, dirty state, and patch stack.
- Locate the final `.config`.
- Run `make kernelrelease` from the build tree.
- Verify `Module.symvers` exists if external modules matter.
- Check whether the target uses modules from the same build.
- Identify the DTS source and the deployed DTB path.
- Archive the evidence before changing anything.

## Practice Exercises

### Exercise 1: Identify Your Host Kernel Build Links

On a normal Linux host:

```sh
uname -r
ls -l /lib/modules/$(uname -r)/build
ls -l /lib/modules/$(uname -r)/source
```

Questions:

- Are headers installed?
- Does `build` point to a generated build directory or a source tree?
- Can you find `.config` or generated headers?

### Exercise 2: Compare Two Kernel Trees

Given an upstream tree and a vendor BSP tree:

```sh
git -C linux-mainline describe --tags --always
git -C linux-vendor describe --tags --always
rg "config YOUR_DRIVER" linux-mainline linux-vendor
```

Questions:

- Does the driver exist in both?
- Is the Kconfig symbol identical?
- Are there vendor-only patches?

### Exercise 3: Prove Module Build Identity

Build a trivial module and inspect it:

```sh
make -C /path/to/kernel/build M=$PWD modules
modinfo ./hello.ko | grep vermagic
make -C /path/to/kernel/build kernelrelease
```

Question:

- Does module `vermagic` match the intended target?

## Related Topics

- [Kernel Source, Build, And Tailoring](index.md)
- [Kernel Source Tree and Outputs](../../build-systems/advanced/linux-kernel/source-tree-and-outputs.md)
- [Vendor Kernel Patch Management](../../build-systems/advanced/linux-kernel/vendor-kernel-patch-management.md)
- [Kernel Release Artifacts](../../build-systems/advanced/linux-kernel/kernel-release-artifacts.md)
- [Reading Kernel Source](../foundations/reading-kernel-source.md)

## Official References

- [Kbuild](https://docs.kernel.org/kbuild/kbuild.html)
- [Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- [Linux Kernel Makefiles](https://docs.kernel.org/kbuild/makefiles.html)
