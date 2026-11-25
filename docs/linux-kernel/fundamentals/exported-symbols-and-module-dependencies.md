---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Exported Symbols And Module Dependencies

## What Problem Does This Solve?

A module is an object file whose undefined symbols are resolved when it is loaded. It can use only what the kernel or another loaded module explicitly exports. This page explains that contract: which functions a module may call, how modules depend on each other, and why a build or load fails with `Unknown symbol`.

## Core Concepts

- `EXPORT_SYMBOL()`, `EXPORT_SYMBOL_GPL()`
- symbol namespaces, `MODULE_IMPORT_NS()`
- `static` and name prefixes
- `Module.symvers`, modpost
- `depmod`, `modules.dep`, `modprobe`, `insmod`
- module reference counting
- `/proc/kallsyms`
- symbol CRCs (`CONFIG_MODVERSIONS`)

## Mental Model

```text
visible in C        non-static, declared in a header
linkable built-in   any non-static symbol in vmlinux
usable by a module  only symbols with EXPORT_SYMBOL*
```

Being able to see a function in a header, or its name in `/proc/kallsyms`, does not mean a module can call it. `kallsyms` lists every symbol, including `static` ones, for stack traces; the export table is much smaller.

## Exporting

```c
/* provider.h, included by both modules */
int demo_core_register(struct demo_client *client);

/* provider.c */
int demo_core_register(struct demo_client *client)
{
    return demo_do_register(client);
}
EXPORT_SYMBOL_GPL(demo_core_register);
```

`EXPORT_SYMBOL_GPL()` makes the symbol available only to modules with a GPL-compatible `MODULE_LICENSE()`. Use it for new kernel-internal interfaces. An export is a commitment to other code in your tree, not a stable ABI: upstream may change or remove it in any release.

Keep everything else `static`. A non-static helper pollutes the global namespace, and a built-in driver with a generic name such as `init_device` can collide with another one at link time. Prefix what must be global.

## Namespaces

A namespace groups exports that only certain modules should use. A module must opt in:

```c
/* provider */
EXPORT_SYMBOL_NS_GPL(demo_core_register, "DEMO_CORE");

/* consumer */
MODULE_IMPORT_NS("DEMO_CORE");
```

The namespace is a string literal since 6.13; earlier kernels take a bare token (`DEMO_CORE`). Without the import, modpost reports an error at build time and the loader refuses the module. Seeing `module uses symbol X from namespace Y, but does not import it` means the symbol is reserved for a subsystem. Decide whether your driver belongs there before adding the import.

A whole directory can use one namespace by setting `DEFAULT_SYMBOL_NAMESPACE` in its Makefile.

## How Dependencies Are Resolved

```text
build:  modpost reads Module.symvers of vmlinux and of other external modules
        -> records "depends" and symbol CRCs in the .ko
install: depmod scans /lib/modules/<release>
        -> writes modules.dep and modules.alias
load:   modprobe reads modules.dep, loads dependencies first
        insmod loads exactly one file and resolves nothing
kernel: resolves each undefined symbol against loaded exports
        -> takes a reference on the providing module
```

Inspect each layer:

```sh
modinfo -F depends demo_client.ko
modinfo -F import_ns demo_client.ko
grep -w demo_core_register Module.symvers
modprobe --show-depends demo_client
lsmod | grep demo_core
```

`lsmod` shows `demo_core ... 1 demo_client`. The symbol use itself holds the reference, so `rmmod demo_core` fails until the client is gone. No `try_module_get()` is needed for that.

### External Modules That Depend On Each Other

Two external modules built in one directory resolve each other automatically. When they live in separate directories, pass the provider's symbol file:

```sh
make -C $KDIR M=$PWD KBUILD_EXTRA_SYMBOLS=/path/to/provider/Module.symvers modules
```

## Symbol Versions

With `CONFIG_MODVERSIONS` every export carries a CRC of its prototype and the types it uses. If a structure changes between the build tree and the running kernel, loading fails with:

```text
disagrees about version of symbol module_layout
```

Rebuild against the exact tree. Do not use `--force-vermagic` or `--force-modversion`; they hide an ABI mismatch that then corrupts memory. See [Module Compatibility Checks](../source-build-and-tailoring/kernel-build-and-install-overview.md#module-compatibility-checks).

## Optional Dependencies

A hard symbol dependency forces the other module to be loaded. Alternatives when the feature is optional:

| Need | Mechanism |
| --- | --- |
| compile-time option | Kconfig dependency plus stub inline functions in the header |
| load another module first, without calling it | `MODULE_SOFTDEP("pre: other")` |
| load on demand | `request_module("other-%s", name)` |
| use it only if already present | `symbol_get()` and `symbol_put()`, GPL exports only |
| providers found at runtime | a registration API or the device model |

When the dependency is a hardware resource such as a clock, GPIO, or regulator, do not model it as a module dependency at all. Request the resource and let `-EPROBE_DEFER` handle ordering.

## Taking Module References

`try_module_get(owner)` is for code that calls into *another* module through a pointer, such as a subsystem core calling a driver's operations. That is what `.owner = THIS_MODULE` in `file_operations` is for: the VFS takes the reference before calling `open()`.

`try_module_get(THIS_MODULE)` from inside the same module is racy. If the module can already be unloading, the instruction that takes the reference may itself be gone.

## Why Some Symbols Are Not Exported

`kallsyms_lookup_name()` has not been exported since 5.7, and the system call table is neither exported nor, on current x86 kernels, used as an indirect call table. Older material, including parts of the LKMPG, patches such internals. It does not work on current kernels and is not a technique for a driver. To observe internal functions use [kprobes or tracepoints](../debugging/kprobes-and-dynamic-probes.md).

## Runnable Example

`examples/linux-kernel/small-labs/16-symbol-exports` builds a provider and a consumer with a symbol namespace.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `Unknown symbol X (err -2)` at load | provider not loaded, or not exported | `lsmod`, `Module.symvers`, use `modprobe` |
| `modpost: "X" undefined!` | not exported by the target tree, or missing `KBUILD_EXTRA_SYMBOLS` | `grep -w X Module.symvers` |
| `GPL-incompatible module ... uses GPL-only symbol` | license | `MODULE_LICENSE()` |
| `module uses symbol ... from namespace` | missing import | `MODULE_IMPORT_NS()` |
| `disagrees about version of symbol` | built against another tree or config | vermagic, build directory |
| `Module X is in use` | client module or open file | `lsmod`, `fuser` |
| works with `=y`, fails with `=m` | function not exported | `EXPORT_SYMBOL_GPL()`, Kconfig tristate logic |

## Practice Exercises

1. Run Lab 16 and provoke each of its four failures. Match every message to the layer that produced it.
2. Pick a subsystem function you use and find its export line, namespace, and header with `git grep`.
3. Explain why a driver that compiles as built-in can fail as a module.

## Debugging Checklist

- Is the symbol exported in *this* kernel version and configuration?
- Is the module license compatible?
- Is a namespace import required?
- Does `modinfo -F depends` match your expectation?
- Was `depmod` run after installation?
- Is everything that need not be global `static`?

## Related Topics

- [Kernel Module Lifecycle](kernel-module-lifecycle.md)
- [Modules And External Modules](../../build-systems/advanced/linux-kernel/modules-and-external-modules.md)
- [Kprobes And Dynamic Probes](../debugging/kprobes-and-dynamic-probes.md)

## Official References

- [Symbol namespaces](https://docs.kernel.org/core-api/symbol-namespaces.html)
- [Building external modules](https://docs.kernel.org/kbuild/modules.html)
- [Unreliable guide to hacking the Linux kernel: symbols](https://docs.kernel.org/kernel-hacking/hacking.html)
