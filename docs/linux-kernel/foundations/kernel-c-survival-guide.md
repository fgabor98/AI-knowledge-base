---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel C Survival Guide

## What Problem Does This Solve?

Kernel code uses C patterns that are uncommon in small userspace programs. A beginner who knows ordinary C may still struggle with kernel code because kernel C is built around embedded objects, callback tables, intrusive data structures, error-encoded pointers, explicit ownership, and cleanup paths.

This page teaches the patterns you need to recognize before reading or writing drivers.

## Core Concepts

- embedded structs
- `container_of`
- intrusive lists
- function pointers
- callback tables
- macros
- `ERR_PTR`
- `IS_ERR`
- `PTR_ERR`
- `goto` cleanup
- bit flags
- fixed-width integer types
- `__iomem` annotations
- `const` data tables
- compile-time helpers

## Embedded Structs

Kernel code often models "is-a" or "has-a" relationships by embedding one struct inside another.

Example:

```c
struct demo_dev {
        struct device *dev;
        struct cdev cdev;
        struct mutex lock;
        char buffer[128];
};
```

Here, `struct demo_dev` owns a `struct cdev`. The character-device core may later give the driver a pointer to the embedded `cdev`, and the driver must recover the containing `struct demo_dev`.

This is different from allocating every object separately and storing only pointers.

Why the kernel uses this pattern:

- fewer allocations
- clear object lifetime
- cache locality
- generic subsystems can operate on embedded common objects

## `container_of`

`container_of(ptr, type, member)` returns a pointer to the outer struct that contains a known member.

Example:

```c
struct demo_dev {
        struct cdev cdev;
        int value;
};

static struct demo_dev *to_demo_dev(struct cdev *cdev)
{
        return container_of(cdev, struct demo_dev, cdev);
}
```

Conceptually:

```text
pointer to member
-> subtract offset of member within outer struct
-> pointer to outer struct
```

Common driver example:

```c
static int demo_open(struct inode *inode, struct file *file)
{
        struct demo_dev *demo;

        demo = container_of(inode->i_cdev, struct demo_dev, cdev);
        file->private_data = demo;
        return 0;
}
```

What to watch:

- The member pointer must really point into the expected containing type.
- The containing object must still be alive.
- `container_of` is powerful, but it bypasses normal type ownership checks.

## Intrusive Lists

Kernel linked lists are intrusive: the list node lives inside the object.

Example object:

```c
struct demo_request {
        struct list_head node;
        u32 id;
        size_t len;
};
```

List head:

```c
static LIST_HEAD(pending_requests);
```

Add an item:

```c
struct demo_request *req;

req = kzalloc(sizeof(*req), GFP_KERNEL);
if (!req)
        return -ENOMEM;

req->id = 10;
list_add_tail(&req->node, &pending_requests);
```

Iterate:

```c
struct demo_request *req;

list_for_each_entry(req, &pending_requests, node) {
        pr_info("request %u len=%zu\n", req->id, req->len);
}
```

Remove safely:

```c
struct demo_request *req, *tmp;

list_for_each_entry_safe(req, tmp, &pending_requests, node) {
        list_del(&req->node);
        kfree(req);
}
```

Common mistakes:

- forgetting `INIT_LIST_HEAD` for dynamically initialized list heads
- deleting while iterating without the `_safe` variant
- freeing an object still linked into a list
- accessing a list without the required lock

## Function Pointers And Callback Tables

Drivers register behavior by filling callback tables.

Character device example:

```c
static const struct file_operations demo_fops = {
        .owner = THIS_MODULE,
        .open = demo_open,
        .read = demo_read,
        .write = demo_write,
        .release = demo_release,
};
```

Platform driver example:

```c
static struct platform_driver demo_driver = {
        .probe = demo_probe,
        .remove = demo_remove,
        .driver = {
                .name = "demo",
                .of_match_table = demo_of_match,
        },
};
```

The key reading technique:

```text
find registration call
-> inspect callback table
-> read callbacks in lifecycle order
```

For the platform example:

```text
module_platform_driver(demo_driver)
-> platform_driver_register()
-> bus matching
-> demo_probe()
-> runtime callbacks
-> demo_remove()
```

## Macros

The kernel uses macros for:

- registration boilerplate
- compile-time checks
- type-safe wrappers
- table definitions
- bit masks
- section annotations

Example:

```c
module_platform_driver(demo_driver);
```

This expands into module init and exit functions that register and unregister the platform driver. Beginners should not treat this as magic. When stuck, search the macro definition.

Search examples:

```bash
rg "define module_platform_driver" include drivers
rg "module_platform_driver" drivers/gpio drivers/iio
```

Macro reading rule:

- First understand the lifecycle the macro represents.
- Then inspect the macro only when debugging registration, ordering, or generated symbols.

## Error Pointers

Many kernel APIs return either a valid pointer or an encoded error pointer.

Example:

```c
demo->base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(demo->base))
        return PTR_ERR(demo->base);
```

The return value is not `NULL` on failure. It is a pointer-shaped error code.

Common helpers:

| Helper | Meaning |
|---|---|
| `ERR_PTR(err)` | encode a negative error code as a pointer |
| `IS_ERR(ptr)` | true if pointer encodes an error |
| `PTR_ERR(ptr)` | recover the negative error code |
| `IS_ERR_OR_NULL(ptr)` | true for error pointer or NULL |
| `PTR_ERR_OR_ZERO(ptr)` | convert pointer-or-error to `0` or negative error |

Example wrapper:

```c
static int demo_get_resources(struct platform_device *pdev,
                              struct demo_dev *demo)
{
        demo->reset = devm_reset_control_get_optional_exclusive(&pdev->dev, NULL);
        if (IS_ERR(demo->reset))
                return dev_err_probe(&pdev->dev, PTR_ERR(demo->reset),
                                     "failed to get reset\n");

        return 0;
}
```

Common mistake:

```c
if (!demo->base)
        return -ENOMEM;
```

This is wrong for APIs that return error pointers.

## Negative Error Codes

Kernel functions usually return `0` for success and a negative `-errno` value for failure.

Examples:

| Code | Meaning | Typical driver use |
|---|---|---|
| `-ENOMEM` | out of memory | allocation failed |
| `-EINVAL` | invalid argument | invalid property, mode, or parameter |
| `-ENODEV` | no such device | hardware absent or unsupported |
| `-ENOENT` | no entry | missing optional file/property/name |
| `-EPROBE_DEFER` | try probe again later | provider not ready |
| `-EIO` | I/O error | hardware transaction failed |
| `-ETIMEDOUT` | timeout | hardware did not become ready |
| `-EBUSY` | resource busy | IRQ, region, or device already owned |

Example:

```c
ret = regulator_enable(demo->vdd);
if (ret)
        return dev_err_probe(dev, ret, "failed to enable vdd\n");
```

Use the original error code when possible. Do not collapse all failures into `-EINVAL` or `-EIO`; that destroys debugging evidence.

## `goto` Cleanup Style

Kernel C often uses `goto` for cleanup because it keeps error paths explicit and avoids deeply nested code.

Manual cleanup example:

```c
static int demo_init_manual(struct demo_dev *demo)
{
        int ret;

        demo->buf = kzalloc(4096, GFP_KERNEL);
        if (!demo->buf)
                return -ENOMEM;

        ret = request_irq(demo->irq, demo_irq, 0, "demo", demo);
        if (ret)
                goto err_free_buf;

        ret = demo_hw_start(demo);
        if (ret)
                goto err_free_irq;

        return 0;

err_free_irq:
        free_irq(demo->irq, demo);
err_free_buf:
        kfree(demo->buf);
        return ret;
}
```

The labels unwind in reverse acquisition order:

```text
allocate buffer
-> request IRQ
-> start hardware
failure:
-> stop/free latest successful resource first
```

Managed-resource version:

```c
static int demo_probe(struct platform_device *pdev)
{
        struct demo_dev *demo;
        int ret;

        demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
        if (!demo)
                return -ENOMEM;

        ret = devm_request_irq(&pdev->dev, demo->irq, demo_irq, 0,
                               dev_name(&pdev->dev), demo);
        if (ret)
                return ret;

        return demo_hw_start(demo);
}
```

Managed cleanup simplifies resource release, but you still must stop hardware, timers, workqueues, and callbacks in the right order.

## Bit Flags And Bit Masks

Kernel code often represents hardware or state flags as bits.

Example register bits:

```c
#define DEMO_CTRL_ENABLE        BIT(0)
#define DEMO_CTRL_IRQ_ENABLE    BIT(1)
#define DEMO_CTRL_MODE_MASK     GENMASK(5, 4)
```

Set and clear:

```c
u32 val;

val = readl(demo->base + DEMO_CTRL);
val |= DEMO_CTRL_ENABLE;
val &= ~DEMO_CTRL_IRQ_ENABLE;
writel(val, demo->base + DEMO_CTRL);
```

Field preparation:

```c
val &= ~DEMO_CTRL_MODE_MASK;
val |= FIELD_PREP(DEMO_CTRL_MODE_MASK, mode);
```

Common mistakes:

- using raw constants like `0x10` without names
- forgetting to clear old field bits before setting new value
- confusing bit index with bit mask
- ignoring endianness or register access width

## Fixed-Width Integer Types

Kernel code uses fixed-width types when hardware, ABI, or wire format requires exact sizes.

Examples:

```c
u8 command;
u16 length;
u32 status;
u64 timestamp_ns;
```

Use fixed-width types for:

- hardware registers
- binary ABI structs
- DMA descriptors
- protocol formats
- Device Tree property values after parsing

Use plain `int` for normal return codes and small internal counters unless exact width matters.

## `__iomem` Annotations

MMIO pointers are not normal memory pointers. They are usually annotated:

```c
void __iomem *base;
```

Access them with I/O helpers:

```c
u32 status = readl(base + DEMO_STATUS);
writel(DEMO_CTRL_ENABLE, base + DEMO_CTRL);
```

Do not do this:

```c
u32 status = *(u32 *)(base + DEMO_STATUS);  /* wrong */
```

The annotation helps static analysis tools such as sparse catch misuse.

## `const` Tables

Match tables and operation tables are commonly `static const`.

Example:

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "example,demo-v1" },
        { .compatible = "example,demo-v2" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Reasons:

- the table is not modified at runtime
- the compiler can place it in read-only data
- the intent is clear to readers

## A Minimal Pattern Recognition Example

Code:

```c
struct demo_dev {
        struct device *dev;
        struct cdev cdev;
        struct mutex lock;
};

static int demo_open(struct inode *inode, struct file *file)
{
        struct demo_dev *demo;

        demo = container_of(inode->i_cdev, struct demo_dev, cdev);
        file->private_data = demo;
        return 0;
}

static const struct file_operations demo_fops = {
        .owner = THIS_MODULE,
        .open = demo_open,
};
```

Reading it:

```text
struct demo_dev owns an embedded cdev
-> VFS gives open() an inode
-> inode points to the cdev
-> container_of recovers demo_dev
-> file->private_data carries device state to later callbacks
```

This is the kind of reasoning needed throughout driver code.

## Common Mistakes

- Checking error-pointer returns with `if (!ptr)`.
- Returning `0` after a failed helper call.
- Freeing an object while it is still on a list.
- Forgetting to initialize embedded locks or lists.
- Using `goto` labels that do not match reverse allocation order.
- Treating MMIO pointers like normal RAM.
- Storing a pointer in `file->private_data` without ensuring it remains alive.
- Copying patterns from old drivers without checking whether APIs changed.

## Debugging Checklist

- What object owns this embedded struct?
- Who calls this callback?
- What context does the callback run in?
- Is this pointer normal, NULL, or error-encoded?
- What releases each acquired resource?
- Is this list protected by a lock?
- Is this table constant data or mutable state?
- Does this helper return a pointer, integer status, or bytes processed?

## Related Topics

- [Reading Kernel Source](reading-kernel-source.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)
- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)
- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)

## References

- Linux Kernel API: <https://docs.kernel.org/core-api/kernel-api.html>
- Linked Lists in Linux: <https://docs.kernel.org/core-api/list.html>
- Linux kernel coding style: <https://docs.kernel.org/process/coding-style.html>
