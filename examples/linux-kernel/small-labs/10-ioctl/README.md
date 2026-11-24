# Lab 10: ioctl ABI

This checkpoint exposes three ioctl commands on a misc device and checks the
ABI rules from userspace: fixed-width structures, validated flags and reserved
fields, `-EFAULT` for a bad pointer, and `-ENOTTY` for an unknown command.

A misc device replaces the device-number, cdev, class, and device steps of
Lab 3 with one registration. Use it for a driver that needs one node.

```sh
make
sudo insmod ./lab_ioctl.ko
ls -l /dev/lab_ioctl
sudo ./test_ioctl
sudo rmmod lab_ioctl
```

Every line must start with `ok`. `lab_ioctl.h` is included by both sides, as a
real UAPI header would be.

Then break the ABI on purpose: change `rate_hz` to `unsigned long`, rebuild
only the test program, and explain the errno you get. The size is encoded in
the command number, so the kernel sees an unknown command.
