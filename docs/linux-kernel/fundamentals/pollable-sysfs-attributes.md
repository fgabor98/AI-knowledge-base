---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Pollable Sysfs Attributes

## What Problem Does This Solve?

Pollable sysfs attributes let userspace wait for state changes without repeatedly reading a file in a loop.

They are useful for low-rate state transitions:

- link state changed
- threshold crossed
- configuration applied
- fault state changed
- data-ready flag changed

They are not a transport for high-rate data. The notification says "state changed"; userspace still reads the attribute to observe the current state.

## Core Concepts

- `poll()`
- `select()`
- `epoll()`
- `POLLPRI`
- `POLLERR`
- `sysfs_notify()`
- state-change notification
- sysfs callbacks
- userspace event loops
- missed event handling
- reread-after-wakeup rule

## Mental Model

The kernel maintains real state. The sysfs attribute exposes that state. `sysfs_notify()` wakes waiters when the state changes.

```text
driver updates state
-> driver calls sysfs_notify()
-> userspace poll wakes
-> userspace seeks/reads attribute
-> userspace compares state
```

The notification is not the data. The file content is the data.

## Kernel-Side Attribute

Private state:

```c
struct demo_priv {
    struct device *dev;
    struct mutex lock;
    bool fault;
};
```

Show callback:

```c
static ssize_t fault_show(struct device *dev,
                          struct device_attribute *attr,
                          char *buf)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    bool fault;

    mutex_lock(&priv->lock);
    fault = priv->fault;
    mutex_unlock(&priv->lock);

    return sysfs_emit(buf, "%u\n", fault);
}
static DEVICE_ATTR_RO(fault);
```

When state changes:

```c
static void demo_set_fault(struct demo_priv *priv, bool fault)
{
    bool changed = false;

    mutex_lock(&priv->lock);
    if (priv->fault != fault) {
        priv->fault = fault;
        changed = true;
    }
    mutex_unlock(&priv->lock);

    if (changed)
        sysfs_notify(&priv->dev->kobj, NULL, "fault");
}
```

The attribute name passed to `sysfs_notify()` must match the sysfs file.

## Userspace Poll Example

Minimal C shape:

```c
int fd = open("/sys/bus/platform/devices/10000000.demo/fault", O_RDONLY);
struct pollfd pfd = {
    .fd = fd,
    .events = POLLPRI | POLLERR,
};

for (;;) {
    char buf[32];
    int ret;

    ret = poll(&pfd, 1, -1);
    if (ret < 0)
        err(1, "poll");

    lseek(fd, 0, SEEK_SET);
    ret = read(fd, buf, sizeof(buf) - 1);
    if (ret < 0)
        err(1, "read");

    buf[ret] = '\0';
    printf("fault=%s", buf);
}
```

Important userspace rule:

```text
after poll wakes, seek to offset 0 and read the current value
```

Do not assume one wakeup equals one event payload.

## Shell Testing

Shell tools are clumsy for `poll()`, but you can still observe state:

```sh
cat /sys/bus/platform/devices/<device>/fault
```

Use a small C program or Python with `select.poll()` for real testing.

Python sketch:

```python
import os
import select

path = "/sys/bus/platform/devices/10000000.demo/fault"
fd = os.open(path, os.O_RDONLY)
poller = select.poll()
poller.register(fd, select.POLLPRI | select.POLLERR)

while True:
    poller.poll()
    os.lseek(fd, 0, os.SEEK_SET)
    print(os.read(fd, 32).decode().strip())
```

## Avoiding Missed State

Poll notifications can coalesce. Userspace should always read current state and compare it with previous state.

Bad userspace assumption:

```text
one poll wakeup == exactly one hardware event
```

Better:

```text
poll wakeup == something may have changed
read state
compare with previous state
act on current state
```

If every event must be counted, use a counter attribute or a different ABI.

Example counter:

```c
return sysfs_emit(buf, "%llu\n", priv->fault_change_count);
```

## State Versus Events

Sysfs is state-oriented.

Good:

```text
fault = 0/1
link_state = up/down
ready = 0/1
change_count = monotonic counter
```

Poor:

```text
read consumes one event
poll delivers a binary packet stream
attribute returns variable binary records
```

For event streams, consider:

- character device with `poll()`
- input subsystem
- IIO buffers/events
- netlink
- tracepoints
- subsystem-specific event mechanisms

## Locking

Protect shared state in both update path and show callback.

Example:

```c
mutex_lock(&priv->lock);
priv->fault = fault;
mutex_unlock(&priv->lock);
```

If state changes in IRQ context, do not take a mutex there. Use:

- atomic variables for simple state
- spinlocks for IRQ-safe shared state
- threaded IRQs or workqueues to move sleepable work out of hard IRQ context

Example with atomic:

```c
atomic_set(&priv->fault, fault);
sysfs_notify(&priv->dev->kobj, NULL, "fault");
```

Then show:

```c
return sysfs_emit(buf, "%d\n", atomic_read(&priv->fault));
```

## Notification Timing

Set state before notifying:

```c
priv->fault = true;
sysfs_notify(&priv->dev->kobj, NULL, "fault");
```

Do not notify first and update later, or userspace may wake and read the old value.

If multiple related attributes change, either:

- notify the primary state attribute
- notify each relevant file
- expose a generation counter

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| poll never wakes | missing `sysfs_notify()` or wrong name | attribute name, code path |
| poll wakes but value unchanged | notification before state update or coalesced event | ordering, userspace compare |
| userspace reads empty data | forgot `lseek(fd, 0, SEEK_SET)` | test program |
| too many wakeups | notifying without state change | change detection |
| missed events | treating sysfs as event queue | counter or better ABI |
| crash on notify/remove | device/kobject lifetime issue | remove ordering |

## Common Mistakes

- Using pollable sysfs for high-rate data.
- Calling `sysfs_notify()` with the wrong attribute name.
- Not maintaining a real backing state variable.
- Not rereading the attribute after wakeup.
- Assuming notifications are counted.
- Updating state after notifying.
- Not handling removal while userspace is polling.

## Practice Exercises

### Exercise 1: Add A Fault Attribute

Expose:

```text
fault
```

and call `sysfs_notify()` only when it changes.

### Exercise 2: Write A Userspace Poll Test

Use C or Python to poll the sysfs file, seek to zero, read, and print the new state.

### Exercise 3: Add A Change Counter

Expose:

```text
fault_change_count
```

so userspace can detect coalesced changes.

## Debugging Checklist

- Is the attribute readable without polling?
- Does the driver update state before notifying?
- Does `sysfs_notify()` use the correct kobject, subdirectory, and filename?
- Does userspace request `POLLPRI`/`POLLERR`?
- Does userspace `lseek()` before rereading?
- Can repeated changes coalesce safely?
- Is a counter needed?
- Is sysfs appropriate for this event rate?
- Is lifetime safe during remove?

## Related Topics

- [Sysfs Attributes](sysfs-attributes.md)
- [Kobjects And Sysfs Groups](kobjects-and-sysfs-groups.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)

## Official References

- [Kobjects](https://docs.kernel.org/core-api/kobject.html)
- [ABI documentation](https://docs.kernel.org/admin-guide/abi.html)
