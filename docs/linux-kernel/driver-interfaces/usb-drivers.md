---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# USB Device Drivers

## What Problem Does This Solve?

USB devices are discovered at runtime, can disappear at any moment, and never expose registers to the CPU. A USB driver submits asynchronous requests to a host controller and must survive an unplug in the middle of every operation.

This page covers host-side drivers: the kernel is the host and the driver handles an attached device. The opposite direction, where the board acts as a USB device, uses the gadget framework.

## Do You Need A Kernel Driver?

Check these first:

| Device | Usual solution |
| --- | --- |
| keyboard, mouse, HID control | HID core, `hid-generic`, or a small HID driver |
| serial adapter | `cdc-acm`, `ftdi_sio`, `cp210x` |
| storage | `usb-storage`, `uas` |
| network adapter | `cdc_ether`, `cdc_ncm`, `r8152` |
| camera, audio | `uvcvideo`, `snd-usb-audio` |
| vendor protocol, one application | libusb in userspace |
| vendor protocol, several applications or a standard subsystem | kernel driver |

Often only a device ID must be added to an existing driver.

## Core Concepts

- device, configuration, interface, altsetting, endpoint
- `struct usb_driver`, `struct usb_interface`, `struct usb_device_id`
- control, bulk, interrupt, and isochronous transfers
- URB
- anchors
- `disconnect()` and lifetime
- autosuspend

## Mental Model

```text
host controller driver
-> USB core enumerates the device, reads descriptors
-> struct usb_device with one or more struct usb_interface
-> drivers match per interface
-> probe(interface)
-> URBs to endpoints, completions in atomic context
-> disconnect(interface), at any time
```

A driver binds to an **interface**, not to a device. A composite device, such as a modem with several serial ports and a network interface, uses several drivers at once.

## Skeleton

```c
static const struct usb_device_id demo_ids[] = {
    { USB_DEVICE(0x1234, 0x5678) },
    { }
};
MODULE_DEVICE_TABLE(usb, demo_ids);

struct demo_usb {
    struct usb_device *udev;
    struct usb_interface *intf;
    struct usb_anchor anchor;
    struct kref ref;
    struct mutex io_lock;       /* I/O against disconnect */
    bool gone;                  /* protected by io_lock */
    u8 bulk_in;
    u8 bulk_out;
    size_t in_size;
};

static int demo_probe(struct usb_interface *intf,
                      const struct usb_device_id *id)
{
    struct usb_endpoint_descriptor *in, *out;
    struct demo_usb *demo;
    int ret;

    ret = usb_find_common_endpoints(intf->cur_altsetting,
                                    &in, &out, NULL, NULL);
    if (ret)
        return dev_err_probe(&intf->dev, ret, "missing bulk endpoints\n");

    demo = kzalloc(sizeof(*demo), GFP_KERNEL);
    if (!demo)
        return -ENOMEM;

    kref_init(&demo->ref);
    mutex_init(&demo->io_lock);
    init_usb_anchor(&demo->anchor);
    demo->udev = usb_get_dev(interface_to_usbdev(intf));
    demo->intf = intf;
    demo->bulk_in = in->bEndpointAddress;
    demo->bulk_out = out->bEndpointAddress;
    demo->in_size = usb_endpoint_maxp(in);

    usb_set_intfdata(intf, demo);
    return demo_register_interface(demo);   /* last */
}

static struct usb_driver demo_driver = {
    .name = "demo_usb",
    .id_table = demo_ids,
    .probe = demo_probe,
    .disconnect = demo_disconnect,
};
module_usb_driver(demo_driver);
```

Always validate the endpoints. Descriptors come from the device, and a malicious or broken device can claim anything. `usb_find_common_endpoints()` checks direction and type.

Match as narrowly as needed: `USB_DEVICE()`, `USB_DEVICE_INTERFACE_NUMBER()`, or a class match with `USB_INTERFACE_INFO()`.

## Synchronous Transfers

For occasional control or bulk transfers in process context:

```c
ret = usb_control_msg_recv(demo->udev, 0, DEMO_REQ_STATUS,
                           USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
                           0, 0, &status, sizeof(status),
                           USB_CTRL_GET_TIMEOUT, GFP_KERNEL);
if (ret)
    return ret;

ret = usb_bulk_msg(demo->udev,
                   usb_sndbulkpipe(demo->udev, demo->bulk_out),
                   buf, len, &actual, 1000);
```

Buffers must be DMA-capable: allocate them with `kmalloc()`, never on the stack and never inside a larger structure next to data the CPU uses. `usb_control_msg_send()` and `usb_control_msg_recv()` make that copy for you, which is why `&status` is allowed above.

Always pass a finite timeout, and handle a short transfer through `actual`.

## URBs

A USB request block describes one asynchronous transfer:

```c
static void demo_read_complete(struct urb *urb)
{
    struct demo_usb *demo = urb->context;

    switch (urb->status) {
    case 0:
        demo_handle_data(demo, urb->transfer_buffer, urb->actual_length);
        break;
    case -ENOENT:
    case -ECONNRESET:
    case -ESHUTDOWN:
        return;                 /* killed or unplugged: do not resubmit */
    default:
        dev_dbg(&demo->intf->dev, "read failed: %d\n", urb->status);
    }

    usb_anchor_urb(urb, &demo->anchor);
    if (usb_submit_urb(urb, GFP_ATOMIC))
        usb_unanchor_urb(urb);
}
```

```c
usb_fill_bulk_urb(urb, demo->udev,
                  usb_rcvbulkpipe(demo->udev, demo->bulk_in),
                  buf, demo->in_size, demo_read_complete, demo);
usb_anchor_urb(urb, &demo->anchor);
ret = usb_submit_urb(urb, GFP_KERNEL);
if (ret)
    usb_unanchor_urb(urb);
usb_free_urb(urb);              /* the anchor or the core holds a reference */
```

- The completion handler runs in atomic context, usually softirq. Use `GFP_ATOMIC` and do not sleep.
- A successful submission guarantees exactly one completion call. A failed one guarantees none.
- Limit resubmission on repeated errors such as `-EPROTO`; a tight loop can flood the log and the controller. `-EPIPE` means a stalled endpoint: clear it with `usb_clear_halt()` from process context.
- `usb_kill_urb()` and `usb_kill_anchored_urbs()` cancel and wait. They sleep. `usb_poison_anchored_urbs()` also rejects later submissions, which closes the resubmission race during teardown.

## Disconnect

Unplugging calls `disconnect()` while system calls may be running and file descriptors are open:

```c
static void demo_disconnect(struct usb_interface *intf)
{
    struct demo_usb *demo = usb_get_intfdata(intf);

    demo_unregister_interface(demo);        /* no new opens */

    mutex_lock(&demo->io_lock);
    demo->gone = true;                      /* existing users fail from now */
    mutex_unlock(&demo->io_lock);

    usb_poison_anchored_urbs(&demo->anchor);
    demo_wake_all_waiters(demo);

    kref_put(&demo->ref, demo_release);     /* freed on the last close */
}
```

- `open()` takes a `kref`; `release()` drops it. The structure outlives the device.
- Every operation takes `io_lock` and returns `-ENODEV` when `gone` is set.
- Wait conditions include `gone`; otherwise a blocked reader sleeps forever.
- After `disconnect()` returns, the driver must not use the interface or submit transfers.

Do not use `devm_kzalloc()` for a structure that an open file can still reference: it is freed right after `disconnect()`.

## Registering With Userspace

Prefer an existing subsystem: input, TTY, netdev, IIO, hwmon, LED. Only for a vendor-specific protocol use `usb_register_dev()` with a `struct usb_class_driver`, or a misc device. The same [ABI rules](../memory-and-io/userspace-copy-and-ioctl-abi.md) apply.

## Power Management

Runtime suspend of USB devices is called autosuspend. Around each I/O burst:

```c
ret = usb_autopm_get_interface(intf);
if (ret)
    return ret;
/* transfers */
usb_autopm_put_interface(intf);
```

Set `.supports_autosuspend = 1` only with working `suspend` and `resume` callbacks: kill URBs in suspend, resubmit in resume. `reset_resume` is called when the device lost power, so its state must be restored. `pre_reset` and `post_reset` bracket a port reset requested by another interface.

## Inspecting

```sh
lsusb -t
lsusb -v -d 1234:5678
udevadm monitor --kernel --property --subsystem-match=usb
grep -H . /sys/bus/usb/devices/*/{idVendor,idProduct,product} 2>/dev/null
cat /sys/bus/usb/devices/1-1:1.0/modalias
```

Capture traffic with usbmon, readable in Wireshark:

```sh
sudo modprobe usbmon
sudo cat /sys/kernel/debug/usb/usbmon/1u
```

Simulate an unplug from software:

```sh
echo 0 | sudo tee /sys/bus/usb/devices/1-1/authorized
echo 1 | sudo tee /sys/bus/usb/devices/1-1/authorized
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| probe not called | another driver bound the interface, or ID table | sysfs driver link, `modalias` |
| `-EPIPE` | stalled endpoint or unsupported control request | usbmon |
| `-EOVERFLOW` | IN buffer smaller than the packet | use a multiple of wMaxPacketSize |
| `-EPROTO`, `-EILSEQ` | cable, signal integrity, or unplug in progress | hardware, power |
| DMA warning about a stack address | buffer on the stack | `kmalloc()` the buffer |
| hang in `disconnect()` | URB resubmitted while being killed, or a lock cycle | poison anchors, lock order |
| use-after-free after unplug | data freed while a file is open | `kref` |
| device stops after idle time | autosuspend without proper resume | `power/control`, resume path |

## Practice Exercises

1. Read `drivers/usb/usb-skeleton.c` and mark where it handles each disconnect rule above.
2. Attach any USB device and map its descriptors to `sysfs` entries and bound drivers.
3. Use `authorized` to disconnect a device while a program reads from it, and record which error the program sees.

## Debugging Checklist

- Are all endpoints validated before use?
- Are all transfer buffers separately allocated?
- Does every completion handler stop resubmitting on shutdown statuses?
- Does every sleeping user wake and return `-ENODEV` on disconnect?
- Was unplug tested during open, read, write, and close?
- Was it tested with autosuspend enabled?

## Related Topics

- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)
- [Runtime PM](../power-management/runtime-pm.md)
- [User-Space Hardware Access Vs Kernel Drivers](../fundamentals/userspace-hardware-access-vs-kernel-drivers.md)

## Official References

- [Linux USB API](https://docs.kernel.org/driver-api/usb/index.html)
- [Writing USB device drivers](https://docs.kernel.org/driver-api/usb/writing_usb_driver.html)
- [USB request block](https://docs.kernel.org/driver-api/usb/URB.html)
- [USB anchors](https://docs.kernel.org/driver-api/usb/anchors.html)
- [Power management for USB](https://docs.kernel.org/driver-api/usb/power-management.html)
