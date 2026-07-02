---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Virtio And RPMsg

## What Problem Does This Solve?

Virtio and RPMsg provide a common message transport between Linux and remote
cores. They let Linux drivers or userspace services exchange packets with
firmware without each platform inventing a completely different communication
bus.

The important boundary:

```text
RPMsg provides message delivery.
Your product still needs a protocol.
```

RPMsg does not define:

- command IDs
- payload schema
- ABI versioning
- retries
- flow control policy
- security policy
- firmware update compatibility
- what a timeout means

Those belong to the driver, userspace service, firmware, and product design.

## Stack Overview

Typical remoteproc + RPMsg stack:

```text
remote firmware resource table
  -> declares virtio device and vrings
     -> remoteproc creates Linux virtio device
        -> virtio_rpmsg_bus binds
           -> RPMsg channels/endpoints appear
              -> kernel RPMsg client driver or rpmsg-char userspace interface
```

Notification path:

```text
Linux sends message
  -> writes payload into shared buffer
  -> updates virtqueue
  -> kicks remote core through mailbox/doorbell

remote sends response
  -> writes payload into shared buffer
  -> updates virtqueue
  -> interrupts/kicks Linux
```

## Core Concepts

### Virtio Device

Virtio is the generic virtual-device framework. In this context, remoteproc
uses firmware resource table entries to instantiate a virtio device backed by
shared memory and mailbox notifications.

Common remoteproc virtio resource:

```text
RSC_VDEV
  -> virtio device id
  -> vring descriptors
  -> feature bits
```

RPMsg is commonly exposed as a virtio device.

### Vring

A vring is a shared-memory ring used by virtio queues.

For RPMsg, there are usually two rings:

```text
vring0: messages from Linux to remote
vring1: messages from remote to Linux
```

Each side places buffers in a ring and notifies the other side.

Vring failures usually look like:

- no messages delivered
- one direction works but not the other
- timeouts under load
- corrupted messages
- remoteproc logs about vring address or size

Check memory size, alignment, cacheability, and notify IDs.

### RPMsg Bus

RPMsg is a Linux bus for message channels to remote processors.

Kernel RPMsg client drivers match channels by name:

```c
static const struct rpmsg_device_id demo_rpmsg_id_table[] = {
    { .name = "demo-control" },
    { },
};
MODULE_DEVICE_TABLE(rpmsg, demo_rpmsg_id_table);
```

Driver:

```c
static struct rpmsg_driver demo_rpmsg_driver = {
    .drv.name = "demo-control",
    .id_table = demo_rpmsg_id_table,
    .probe = demo_rpmsg_probe,
    .callback = demo_rpmsg_cb,
    .remove = demo_rpmsg_remove,
};
module_rpmsg_driver(demo_rpmsg_driver);
```

If firmware announces `demo_ctrl` but the driver matches `demo-control`, the
driver will not bind.

### Endpoint

An RPMsg endpoint is an addressable communication endpoint.

In a simple client driver, the default endpoint is provided by the RPMsg device:

```c
static int demo_rpmsg_probe(struct rpmsg_device *rpdev)
{
    dev_set_drvdata(&rpdev->dev, rpdev);
    return rpmsg_send(rpdev->ept, "hello", 5);
}
```

Callback:

```c
static int demo_rpmsg_cb(struct rpmsg_device *rpdev, void *data, int len,
                         void *priv, u32 src)
{
    dev_info(&rpdev->dev, "rx %d bytes from 0x%x\n", len, src);
    return 0;
}
```

Drivers can also create additional endpoints when they need multiple logical
channels. Keep endpoint lifetime clear: remote shutdown or crash can remove the
device underneath you.

### Name Service

Many RPMsg systems use a name service announcement from firmware. The remote
side announces a channel name; Linux creates an `rpmsg_device`; a matching Linux
driver binds.

Flow:

```text
remote firmware starts
  -> initializes RPMsg
  -> announces "demo-control"
  -> Linux creates rpmsg device
  -> demo-control rpmsg driver probes
```

If no RPMsg device appears:

- firmware may not have created/announced the endpoint
- resource table may lack RPMsg virtio device
- virtio/rpmsg driver may be disabled
- mailbox interrupts may not work
- endpoint name may differ

## Kernel RPMsg Client Driver Example

Minimal request/response shape:

```c
struct demo_msg {
    __le16 abi_major;
    __le16 abi_minor;
    __le16 cmd;
    __le16 len;
    __le32 seq;
    u8 payload[];
};
```

Private state:

```c
struct demo_rpmsg {
    struct rpmsg_device *rpdev;
    struct completion reply_done;
    struct mutex lock;
    u32 next_seq;
    int status;
};
```

Probe:

```c
static int demo_rpmsg_probe(struct rpmsg_device *rpdev)
{
    struct demo_rpmsg *demo;

    demo = devm_kzalloc(&rpdev->dev, sizeof(*demo), GFP_KERNEL);
    if (!demo)
        return -ENOMEM;

    demo->rpdev = rpdev;
    init_completion(&demo->reply_done);
    mutex_init(&demo->lock);
    dev_set_drvdata(&rpdev->dev, demo);

    dev_info(&rpdev->dev, "bound to remote endpoint\n");
    return 0;
}
```

Send request:

```c
static int demo_send_ping(struct demo_rpmsg *demo)
{
    struct demo_msg msg = {
        .abi_major = cpu_to_le16(1),
        .abi_minor = cpu_to_le16(0),
        .cmd = cpu_to_le16(DEMO_CMD_PING),
        .len = cpu_to_le16(0),
        .seq = cpu_to_le32(demo->next_seq++),
    };
    int ret;

    mutex_lock(&demo->lock);
    reinit_completion(&demo->reply_done);

    ret = rpmsg_send(demo->rpdev->ept, &msg, sizeof(msg));
    if (ret)
        goto out_unlock;

    if (!wait_for_completion_timeout(&demo->reply_done,
                                     msecs_to_jiffies(1000)))
        ret = -ETIMEDOUT;

out_unlock:
    mutex_unlock(&demo->lock);
    return ret;
}
```

Receive callback:

```c
static int demo_rpmsg_cb(struct rpmsg_device *rpdev, void *data, int len,
                         void *priv, u32 src)
{
    struct demo_rpmsg *demo = dev_get_drvdata(&rpdev->dev);
    const struct demo_msg *msg = data;

    if (len < sizeof(*msg))
        return -EINVAL;

    if (le16_to_cpu(msg->abi_major) != 1)
        return -EPROTO;

    demo->status = le16_to_cpu(msg->cmd);
    complete(&demo->reply_done);

    return 0;
}
```

This is intentionally small. Real drivers need stronger locking, sequence
matching, shutdown handling, and message validation.

## Userspace RPMsg Interfaces

Some systems expose RPMsg channels to userspace through rpmsg character-device
interfaces. Availability depends on kernel configuration, drivers, and platform
policy.

Useful inspection:

```sh
find /sys/bus/rpmsg -maxdepth 3 -type f -print
ls /dev | grep rpmsg
dmesg | grep -i rpmsg
```

Userspace RPMsg is useful for:

- development tools
- diagnostics
- simple product services
- protocol testing

Kernel RPMsg client drivers are more appropriate when:

- another kernel subsystem consumes the data
- strict lifecycle coordination is needed
- security policy should not expose raw channels to userspace
- the channel controls hardware resources used by kernel drivers

## Protocol Design

At minimum, define:

| Field | Purpose |
| --- | --- |
| magic | reject wrong stream or stale memory |
| ABI major/minor | compatibility |
| command/event ID | operation type |
| length | bounds checking |
| sequence number | match request/response |
| status | report remote-side errors |
| flags | optional behavior |

Example header:

```c
#define DEMO_MAGIC 0x44454d4f /* "DEMO" */

struct demo_hdr {
    __le32 magic;
    __le16 abi_major;
    __le16 abi_minor;
    __le16 type;
    __le16 flags;
    __le32 seq;
    __le32 payload_len;
};
```

Rules:

- always validate length before reading payload
- reject unsupported major versions
- ignore or negotiate unknown feature bits
- include a timeout for request/response operations
- define what happens if firmware restarts
- define byte order explicitly
- avoid embedding raw kernel pointers or physical addresses

## Flow Control And Timeouts

RPMsg send can fail or block depending on API and buffer availability. A robust
protocol handles backpressure.

Questions:

- What is the maximum message size?
- How many outstanding requests are allowed?
- What happens when vring buffers are exhausted?
- Can the remote side send unsolicited events?
- Are events dropped, coalesced, or queued?
- What timeout is appropriate?
- Can a timed-out request receive a late response?

Simple policy:

```text
one outstanding request at a time
sequence number increments per request
timeout after 1 second
late responses are logged and ignored
remote restart clears pending request
```

More complex protocols may need queues, credits, or feature negotiation.

## Remote Restart And Endpoint Disappearance

Remote firmware can crash or be stopped. RPMsg devices may disappear and later
reappear.

Driver implications:

- `.remove` must stop new requests
- callbacks must tolerate shutdown state
- pending completions must be completed with an error
- userspace file descriptors may need hangup/error behavior
- sequence state may reset after firmware reboot
- firmware version should be renegotiated after every bind

Example remove:

```c
static void demo_rpmsg_remove(struct rpmsg_device *rpdev)
{
    struct demo_rpmsg *demo = dev_get_drvdata(&rpdev->dev);

    mutex_lock(&demo->lock);
    demo->status = -ENODEV;
    complete_all(&demo->reply_done);
    mutex_unlock(&demo->lock);
}
```

Do not assume the remote endpoint is permanent just because it appeared once.

## Security Considerations

Remote firmware may have access to memory, hardware, or privileged actions.
Treat messages as untrusted unless the platform security model proves otherwise.

Validation rules:

- check all lengths
- check enum ranges
- reject unknown critical flags
- avoid integer overflow in size calculations
- do not pass raw message payloads directly to kernel subsystems
- restrict userspace access to raw RPMsg devices
- log protocol version and peer identity where possible

If remote firmware can be updated independently, protocol validation becomes
more important, not less.

## Debugging RPMsg

Start with remoteproc:

```sh
cat /sys/class/remoteproc/remoteproc0/state
dmesg | grep -Ei 'remoteproc|rproc|virtio|rpmsg'
```

Inspect RPMsg bus:

```sh
find /sys/bus/rpmsg -maxdepth 4 -print
```

Check character devices:

```sh
ls -l /dev/rpmsg*
```

Check interrupts/mailboxes:

```sh
cat /proc/interrupts
dmesg | grep -Ei 'mailbox|mbox|vring|virtqueue'
```

Trace relevant events if available:

```sh
sudo trace-cmd record -e irq -e workqueue -e sched sleep 5
sudo trace-cmd report
```

Firmware-side logs are often essential. If Linux sees the channel but gets no
response, determine whether the remote firmware received the message.

## Common Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| resource table lacks RPMsg vdev | no RPMsg bus device | rebuild firmware resource table |
| endpoint name mismatch | Linux client driver never probes | align firmware announcement and id table |
| mailbox channel wrong | send blocks or remote never wakes | fix Device Tree mailbox references |
| vring memory wrong | corrupted messages or virtio failure | fix reserved memory/resource table |
| no ABI version | silent protocol mismatch | add handshake and reject incompatible versions |
| no timeout | caller hangs forever | use bounded waits |
| endpoint removed during request | use-after-free or hang | handle `.remove` and complete pending work |
| userspace raw access too broad | security/product risk | restrict device nodes or use kernel driver |
| cache maintenance wrong | stale payloads | use transport APIs and correct memory attributes |

## Practice Exercises

1. Start a remote core and inspect whether any RPMsg devices appear under
   `/sys/bus/rpmsg`.
2. Find the endpoint name announced by firmware and match it to a Linux RPMsg
   driver id table.
3. Write a minimal message header with magic, ABI version, command, length, and
   sequence number.
4. Simulate remote firmware restart and check whether your client handles
   endpoint removal.
5. Compare RPMsg behavior with a broken mailbox phandle in a lab Device Tree.

## Review Checklist

- Does the firmware resource table declare the expected virtio/RPMsg device?
- Are vring and buffer memory correctly reserved, aligned, and coherent?
- Do endpoint names match Linux drivers or userspace expectations?
- Does the protocol include versioning and length validation?
- Are timeouts and backpressure handled?
- Does the client handle remote shutdown, crash, and restart?
- Is raw userspace access appropriate for the product security model?
- Are mailbox/interrupt paths verified in both directions?

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Reserved Memory](reserved-memory.md)
- [Remote Core Logs And Crashes](remote-core-logs-and-crashes.md)

## Official References

- [Remote Processor Messaging](https://docs.kernel.org/staging/rpmsg.html)
- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Virtio Specification](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html)
