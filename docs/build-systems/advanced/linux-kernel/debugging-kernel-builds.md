---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Debugging Kernel Builds

## What Problem Does This Solve?

Kernel build failures can come from configuration, Kbuild object selection, host tools, cross-compilers, generated headers, stale output trees, modules, device trees, or deployment mismatches.

This page provides a systematic debugging workflow.

## Core Concepts

- verbose build
- output directory
- final `.config`
- generated headers
- Kbuild object selection
- host tools
- cross compiler
- module versioning
- DTB deployment
- stale artifacts

## Mental Model

Classify the failure first:

```text
configuration failure
compile failure
link failure
module build failure
device tree build failure
install/deploy failure
runtime mismatch
```

Each class has different evidence.

## Syntax / API / Mechanism

Verbose build:

```sh
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1 Image
```

Explain more:

```sh
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1 W=1
```

Clean selected outputs:

```sh
make O=build clean
```

Deep clean output tree:

```sh
rm -rf build
```

Inspect config:

```sh
grep CONFIG_FOO build/.config
```

Inspect generated command:

```sh
make O=build V=1 drivers/path/file.o
```

## Debugging By Symptom

### Driver Source Does Not Build

Check:

- final `.config`
- Kconfig dependencies
- Kbuild Makefile
- parent directory selection
- built-in vs module expectation

Commands:

```sh
grep CONFIG_DRIVER build/.config
grep -R "driver.o" drivers/
make O=build V=1 drivers/subsystem/
```

### Compile Error In Driver

Check:

- exact compiler command
- include paths
- config-dependent APIs
- kernel version compatibility
- missing generated headers
- architecture-specific definitions

Use:

```sh
make O=build V=1 path/to/file.o
```

### Link Error In `vmlinux`

Likely causes:

- object selected without dependency
- missing symbol
- duplicate symbol
- config mismatch
- wrong built-in vs module expectation

Check:

```sh
nm object.o
grep symbol Module.symvers
```

### External Module Fails

Check:

- kernel build directory
- `Module.symvers`
- `ARCH`
- `CROSS_COMPILE`
- kernel release
- generated headers

Use:

```sh
make -C /path/to/kernel/build M=$PWD V=1 modules
```

### DTB Build Fails

Check:

- DTS syntax
- include paths
- missing labels
- binding expectations
- overlay syntax

Use:

```sh
make O=build ARCH=arm64 V=1 dtbs
dtc -I dts -O dtb file.dts -o /tmp/test.dtb
```

### Build Succeeds But Runtime Is Wrong

This is often not a kernel build failure. It may be deployment.

Check:

```sh
uname -a
cat /proc/cmdline
tr -d '\0' < /proc/device-tree/model
find /lib/modules -maxdepth 1 -type d
```

Compare with build outputs and deployed artifacts.

## Stale Output Trees

When changing these, consider a clean output tree:

- `ARCH`
- compiler/toolchain
- major config baseline
- source branch
- generated headers
- vendor BSP version

Separate output trees avoid many issues:

```text
build-am62x/
build-am64x/
build-debug/
```

## Host Tool Problems

Kernel builds compile host tools under `scripts/` and sometimes `tools/`. These run on the build machine, not the target.

Symptoms:

- host compiler missing
- `bison`/`flex` missing
- OpenSSL headers missing for certificate/signing tools
- `dtc` build failure

Fix host dependencies separately from target toolchain issues.

## Cross-Compiler Problems

Symptoms:

- wrong architecture output
- unsupported compiler option
- assembler errors
- linker cannot find target runtime pieces

Check:

```sh
which aarch64-linux-gnu-gcc
aarch64-linux-gnu-gcc --version
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- V=1
```

## Expansion: Failure-Mode Matrix

Use this matrix to avoid chasing the wrong layer.

| Symptom | Likely Layer | First Checks |
| --- | --- | --- |
| source file exists but object is missing | Kconfig/Kbuild | final `.config`, `obj-*` rule, `CONFIG_*` value |
| config fragment requests option but final `.config` lacks it | Kconfig dependencies | `scripts/kconfig/merge_config.sh` output, dependencies, symbol rename |
| module build succeeds but target rejects module | module compatibility | `uname -r`, `modinfo vermagic`, `Module.symvers`, architecture |
| board boots with old behavior after DTS change | deployment | generated DTB checksum, U-Boot load path, `/proc/device-tree` |
| DTB compiles but driver does not probe | binding/runtime | `dtbs_check`, compatible string, clocks, resets, pinctrl, regulators |
| rootfs cannot mount | configuration/initramfs | storage driver built-in vs module, initramfs contents, kernel command line |
| two builds from same source differ | reproducibility | timestamps, build user/host, compiler version, dirty source tree |
| crash log cannot be decoded | release artifact retention | matching `vmlinux`, `System.map`, `.config`, build ID |

For deeper treatments, see:

- [Configuration Fragments and Auditing](configuration-fragments-and-auditing.md)
- [Device Tree Binding Validation](device-tree-binding-validation.md)
- [Kernel Release Artifacts](kernel-release-artifacts.md)
- [Reproducible Kernel Builds](reproducible-kernel-builds.md)

## Common Mistakes

- Debugging deployment as if it were compilation.
- Ignoring final `.config`.
- Forgetting `V=1`.
- Building external modules against the wrong kernel.
- Reusing output directories across architectures.
- Deleting source changes while trying to clean generated output.
- Ignoring host tool dependencies.
- Assuming a module exists when the driver was built in.

## Debugging Checklist

- Classify the failure type.
- Confirm source tree and output tree.
- Confirm `ARCH` and `CROSS_COMPILE`.
- Inspect final `.config`.
- Use verbose build output.
- Trace Kconfig to Kbuild to object.
- Check generated headers.
- Check module/kernel release match.
- Check deployed artifacts on target.
- Preserve logs from the first failure, not only later failures.
- Classify whether the failure is build, configuration, deployment, runtime, or release provenance.

## Related Topics

- [Kconfig and Defconfig](kconfig-and-defconfig.md)
- [Kbuild Objects and Directories](kbuild-objects-and-directories.md)
- [Modules and External Modules](modules-and-external-modules.md)
- [Configuration Fragments and Auditing](configuration-fragments-and-auditing.md)
- [Kernel Release Artifacts](kernel-release-artifacts.md)
- [Boot Debugging and Runtime Validation](../bsp-integration/boot-debugging-and-runtime-validation.md)

## References

- Linux kernel Kbuild documentation
- Linux kernel external module documentation
- Linux kernel device tree documentation
- GNU Make manual
