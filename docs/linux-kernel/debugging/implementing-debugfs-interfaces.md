---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Implementing Debugfs Interfaces

## What Problem Does This Solve?

[Debugfs And Sysfs Inspection](debugfs-and-sysfs-inspection.md) explains how to read what other drivers expose. This page explains how to add debugfs files to your own driver: register dumps, state machines, counters, and fault-injection switches that help during bring-up and support without creating a userspace ABI.

## Core Concepts

- `debugfs_create_dir()`, `debugfs_create_file()`, `debugfs_remove()`
- typed helpers: `debugfs_create_u32()`, `debugfs_create_bool()`, `debugfs_create_x32()`
- `DEFINE_SHOW_ATTRIBUTE()`, `DEFINE_DEBUGFS_ATTRIBUTE()`
- `debugfs_create_regset32()`, `debugfs_create_blob()`, `debugfs_create_devm_seqfile()`
- no error checking
- removal safety

## Mental Model

```text
driver state -> debugfs file -> root user during development or support
```

Debugfs is optional and unstable. The driver must behave identically when it is absent, and no product software may depend on it. If a script, service, or test fixture needs the value permanently, it belongs in sysfs, a subsystem ABI, or a tracepoint.

## Directory Per Device

```c
struct demo_priv {
    struct device *dev;
    struct dentry *debugfs;
    struct mutex lock;
    u32 irq_count;
    bool inject_error;
};

static void demo_debugfs_init(struct demo_priv *priv)
{
    priv->debugfs = debugfs_create_dir(dev_name(priv->dev), demo_debugfs_root);

    debugfs_create_u32("irq_count", 0444, priv->debugfs, &priv->irq_count);
    debugfs_create_bool("inject_error", 0600, priv->debugfs,
                        &priv->inject_error);
    debugfs_create_file("state", 0444, priv->debugfs, priv, &demo_state_fops);
}
```

The function returns nothing on purpose. **Do not check debugfs results.** On failure, or with `CONFIG_DEBUG_FS=n`, the calls return an error pointer that every other debugfs function accepts silently. Driver code that fails probe because debugfs failed is wrong.

Use the device name for the directory so that two instances do not collide. Create the shared parent directory in module init and remove it in module exit. Many subsystems already provide a per-device directory, for example `client->debugfs` for I2C clients and `iio_get_debugfs_dentry()`; use it when available.

## Typed Helpers

| Helper | File content |
| --- | --- |
| `debugfs_create_u8()` ... `debugfs_create_u64()` | decimal integer |
| `debugfs_create_x8()` ... `debugfs_create_x64()` | hexadecimal integer |
| `debugfs_create_bool()` | `Y` or `N` |
| `debugfs_create_atomic_t()` | atomic counter |
| `debugfs_create_size_t()`, `debugfs_create_ulong()` | native sizes |
| `debugfs_create_str()` | string |
| `debugfs_create_blob()` | read-only memory block |
| `debugfs_create_regset32()` | named MMIO registers |

These access the variable directly without your lock. That is acceptable for an independent counter or flag read with a single load. It is not acceptable when the value must be consistent with other state, or when writing must be validated or must have an effect.

## Formatted Output

```c
static int demo_state_show(struct seq_file *m, void *unused)
{
    struct demo_priv *priv = m->private;

    mutex_lock(&priv->lock);
    seq_printf(m, "mode:     %s\n", demo_mode_name(priv->mode));
    seq_printf(m, "retries:  %u\n", priv->retries);
    mutex_unlock(&priv->lock);
    return 0;
}
DEFINE_SHOW_ATTRIBUTE(demo_state);
```

`DEFINE_SHOW_ATTRIBUTE(demo_state)` generates `demo_state_fops`. For listings with many records use a full [seq_file iterator](../fundamentals/procfs-and-seq-file.md).

Inside a probe function, `debugfs_create_devm_seqfile(dev, "state", parent, show)` is a shorter form; there the show function finds the device through `m->private`, which is the `struct device`.

## Values With Side Effects

```c
static int demo_threshold_get(void *data, u64 *val)
{
    struct demo_priv *priv = data;

    *val = READ_ONCE(priv->threshold);
    return 0;
}

static int demo_threshold_set(void *data, u64 val)
{
    struct demo_priv *priv = data;
    int ret;

    if (val > DEMO_THRESHOLD_MAX)
        return -ERANGE;

    mutex_lock(&priv->lock);
    ret = demo_write_threshold(priv, val);
    mutex_unlock(&priv->lock);
    return ret;
}
DEFINE_DEBUGFS_ATTRIBUTE(demo_threshold_fops, demo_threshold_get,
                         demo_threshold_set, "%llu\n");

debugfs_create_file_unsafe("threshold", 0600, priv->debugfs, priv,
                           &demo_threshold_fops);
```

`DEFINE_DEBUGFS_ATTRIBUTE()` already protects its callbacks against concurrent removal, so it is paired with `debugfs_create_file_unsafe()`. With your own `file_operations` use the ordinary `debugfs_create_file()`, which adds that protection.

## Register Dumps

```c
static const struct debugfs_reg32 demo_regs[] = {
    { .name = "CTRL",   .offset = 0x00 },
    { .name = "STATUS", .offset = 0x04 },
};

priv->regset.regs = demo_regs;
priv->regset.nregs = ARRAY_SIZE(demo_regs);
priv->regset.base = priv->base;
priv->regset.dev = priv->dev;   /* runtime-resumes the device for the read */

debugfs_create_regset32("regs", 0400, priv->debugfs, &priv->regset);
```

Set `.dev` when the device uses runtime PM. Reading registers of a powered-down block hangs or aborts on many SoCs. Leave out registers whose read has side effects, such as clear-on-read status.

If the driver uses regmap, a register dump already exists under `/sys/kernel/debug/regmap/<device>/registers`. Do not write another.

## Removal

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    debugfs_remove(priv->debugfs);
    /* only now tear down what the files use */
}
```

`debugfs_remove()` removes the whole subtree, blocks new operations, and waits for running ones. `debugfs_remove_recursive()` is the same function under its old name. Call it **before** freeing the data. With `devm_kzalloc()`ed data, removing in `remove()` is early enough, because managed memory is released after `remove()` returns.

Files created by `probe()` must also be removed when a later probe step fails. Creating debugfs as the last step avoids that.

## Security And Policy

- Debugfs is root-only by default, but a file that writes registers can still damage hardware or bypass lockdown policy. In lockdown mode writable files and unusual modes are refused.
- Use `0400` or `0600` for anything that reveals addresses, keys, or user data.
- Print pointers with `%p`, never `%px`.
- Production kernels may set `CONFIG_DEBUG_FS=n` or `debugfs=off`. Test the driver that way.

## Debugfs Compared With Alternatives

| Need | Prefer |
| --- | --- |
| one-off inspection during bring-up | debugfs |
| event order and latency | [tracepoints](ftrace-and-tracepoints.md) |
| conditional messages | [dynamic debug](dynamic-debug.md) |
| a value userspace relies on | [sysfs](../fundamentals/sysfs-attributes.md) |
| inspecting code that has no hook at all | [kprobes](kprobes-and-dynamic-probes.md) |

## Runnable Example

`examples/linux-kernel/small-labs/12-seq-file-debugfs` has a typed variable, a formatted file, and two command files.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| driver fails probe without debugfs | return value checked | probe code |
| directory missing for the second device | name collision | use `dev_name()` |
| crash when a file stays open during unbind | `_unsafe` variant with plain fops | creation call |
| use-after-free on read | data freed before `debugfs_remove()` | remove order |
| bus error or hang when dumping registers | device runtime-suspended | `.dev` in the regset, `pm_runtime_resume_and_get()` |
| customer script breaks after update | debugfs used as ABI | move the value to sysfs |

## Practice Exercises

1. Add a `regs` or `state` file to one of your drivers and prove that it still probes with `debugfs=off`.
2. Open a debugfs file, unbind the device through sysfs, then read the open file. Explain the result.
3. For three debugfs files on your board, decide whether each could become a tracepoint.

## Debugging Checklist

- Are debugfs results ignored everywhere?
- Is the subtree removed before its data?
- Does each multi-field file take the lock?
- Are register reads safe in every power state?
- Are modes minimal?

## Related Topics

- [Debugfs And Sysfs Inspection](debugfs-and-sysfs-inspection.md)
- [Procfs And seq_file](../fundamentals/procfs-and-seq-file.md)
- [Regmap](../driver-interfaces/regmap.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)

## Official References

- [DebugFS](https://docs.kernel.org/filesystems/debugfs.html)
- [The seq_file interface](https://docs.kernel.org/filesystems/seq_file.html)
