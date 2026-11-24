---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Procfs And seq_file

## What Problem Does This Solve?

Kernel code often has to print a list or a table whose size is not known in advance. A hand-written `read()` callback must handle partial reads, file positions, and buffer limits, and it is easy to get wrong. `seq_file` does this once for all users: you print records, it handles the rest.

`procfs` is where many of these files live historically. A driver developer must be able to read and maintain procfs code. New driver interfaces belong elsewhere.

## Core Concepts

- `proc_create()`, `proc_mkdir()`, `remove_proc_entry()`, `proc_remove()`
- `struct proc_ops`
- `single_open()`, `proc_create_single()`
- `struct seq_operations`: `start`, `next`, `stop`, `show`
- `seq_printf()`, `seq_puts()`, `seq_list_start()`
- `DEFINE_SHOW_ATTRIBUTE()`, `DEFINE_PROC_SHOW_ATTRIBUTE()`

## Where Should The File Live?

| Content | Place |
| --- | --- |
| one value of a device | sysfs attribute |
| diagnostics, counters, dumps | debugfs |
| events and timing | tracepoints |
| stream or commands | character device |
| existing process or system-wide text interface | procfs |

Do not add a new `/proc` file for a driver. Everything below except the `proc_create*()` calls works identically in debugfs.

## Mental Model

A read of a seq file is a loop over records into an internal buffer:

```text
read():
  start(pos)                first record at or after pos
  while the buffer has room:
      show(record)          append text
      next(record)          advance pos, return next record or NULL
  stop(record)              always paired with start()
  copy the buffer to userspace
```

Two consequences matter in practice:

- A long listing is produced in several passes. Between passes your lock is released, and the list may change.
- If one record does not fit, the buffer is enlarged and the record is shown again. `show()` must therefore have no side effects.

## One Short Text: `single_open()`

When everything fits comfortably in memory, only `show()` is needed:

```c
static int demo_summary_show(struct seq_file *m, void *unused)
{
    struct demo_priv *priv = m->private;

    mutex_lock(&priv->lock);
    seq_printf(m, "packets: %llu\n", priv->packets);
    seq_printf(m, "errors:  %llu\n", priv->errors);
    mutex_unlock(&priv->lock);
    return 0;
}
DEFINE_PROC_SHOW_ATTRIBUTE(demo_summary);

proc_create_data("demo_summary", 0444, NULL, &demo_summary_proc_ops, priv);
```

`DEFINE_PROC_SHOW_ATTRIBUTE(name)` generates `name_open()` and `name_proc_ops`. `DEFINE_SHOW_ATTRIBUTE(name)` does the same with a `file_operations` called `name_fops`, for debugfs and character devices.

With `single_open()` the whole output is generated in one `show()` call, so it is a consistent snapshot, but a very large output needs a very large kernel buffer.

## Many Records: A Full Iterator

```c
static void *demo_seq_start(struct seq_file *m, loff_t *pos)
{
    mutex_lock(&demo_lock);
    return seq_list_start(&demo_items, *pos);
}

static void *demo_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
    return seq_list_next(v, &demo_items, pos);
}

static void demo_seq_stop(struct seq_file *m, void *v)
{
    mutex_unlock(&demo_lock);
}

static int demo_seq_show(struct seq_file *m, void *v)
{
    const struct demo_item *item = list_entry(v, struct demo_item, node);

    seq_printf(m, "%u %s\n", item->id, item->name);
    return 0;
}

static const struct seq_operations demo_seq_ops = {
    .start = demo_seq_start,
    .next = demo_seq_next,
    .stop = demo_seq_stop,
    .show = demo_seq_show,
};

proc_create_seq("demo_items", 0444, NULL, &demo_seq_ops);
```

Rules:

- `stop()` is called after every `start()`, including when `start()` returned `NULL`. Take the lock in `start()` and release it in `stop()` without conditions.
- `start()` with `*pos` beyond the end returns `NULL`, which ends the file.
- `next()` must always advance `*pos`, also when it returns `NULL`. Otherwise the last record can be repeated.
- To print a header, return `SEQ_START_TOKEN` from `start()` for `*pos == 0` and test for it in `show()`; `seq_list_start_head()` helps for lists.
- For RCU lists use `rcu_read_lock()`/`rcu_read_unlock()` with the `seq_list_*_rcu()` helpers. Then `show()` must not sleep.
- Return `SEQ_SKIP` from `show()` to omit a record.

`seq_list_start()` walks to `*pos` from the beginning each time, so a huge list costs quadratic time. Use an index, an `xarray`, or per-open state (`__seq_open_private()`) if that matters.

## Output Functions

| Function | Use |
| --- | --- |
| `seq_printf()` | formatted text |
| `seq_puts()`, `seq_putc()` | constant text |
| `seq_hex_dump()` | binary data |
| `seq_escape()` | strings that may contain spaces or newlines |
| `seq_put_decimal_ull()` | fast path for large numeric tables |

Do not check their results for overflow; the core detects it and retries. Do not print raw kernel pointers.

## Writable Files And `proc_ops`

Since 5.6, procfs uses `struct proc_ops` instead of `file_operations`:

```c
static const struct proc_ops demo_proc_ops = {
    .proc_open = demo_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
    .proc_write = demo_write,
};
```

`seq_file` handles only reading. A `proc_write` callback copies from userspace like any other write:

```c
static ssize_t demo_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    unsigned int val;
    int ret;

    ret = kstrtouint_from_user(ubuf, count, 0, &val);
    if (ret)
        return ret;
    if (val > DEMO_MAX)
        return -ERANGE;

    WRITE_ONCE(demo_limit, val);
    return count;
}
```

Match open and release: `single_open()` with `single_release()`, `seq_open()` with `seq_release()`, `seq_open_private()` with `seq_release_private()`. A mismatch leaks memory on every open.

## Lifetime

`proc_create*()` makes the file visible immediately. Create it after everything it uses is ready.

```c
remove_proc_entry("demo_items", NULL);
demo_free_items();
```

`remove_proc_entry()` prevents new operations and waits for running ones, so the data can be freed afterwards. It does not need `try_module_get()`.

## Runnable Example

`examples/linux-kernel/small-labs/12-seq-file-debugfs` prints a list through a `seq_file` iterator and has debugfs controls to grow it past the buffer size.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `cat` never ends | `read()` never returns 0, or `start()` ignores `*pos` | iterator code |
| last line repeated with small reads | `next()` does not advance `*pos` at the end | `dd bs=1` test |
| lock held forever or unlock imbalance | conditional lock in `start()` or `stop()` | lockdep |
| memory grows with each open | mismatched release function | kmemleak |
| counters jump between reads | `show()` has side effects and ran twice | `show()` body |
| records missing or duplicated | list changed between passes | document, or print a snapshot |
| crash after unload | data freed before the entry was removed | exit order |

## Practice Exercises

1. Run Lab 12, add 100000 items, and use `strace -e read cat /proc/lab_seq` to see the passes.
2. Delete the `*pos` handling by writing your own `next()` that returns `NULL` without advancing. Find the bug with `dd bs=1`.
3. Convert the lab's list to RCU and use `seq_list_start_rcu()`.

## Debugging Checklist

- Is this really a procfs interface, or should it be sysfs or debugfs?
- Are `start()` and `stop()` balanced on every path?
- Does `dd if=file bs=1` give the same bytes as `cat`?
- Is the file created last and removed first?
- Are permissions as narrow as the content requires?

## Related Topics

- [Implementing Debugfs Interfaces](../debugging/implementing-debugfs-interfaces.md)
- [Sysfs Attributes](sysfs-attributes.md)
- [Character Device Basics](character-device-basics.md)
- [RCU](../execution-and-concurrency/rcu.md)

## Official References

- [The seq_file interface](https://docs.kernel.org/filesystems/seq_file.html)
- [The /proc filesystem](https://docs.kernel.org/filesystems/proc.html)
