# Lab 3: Dummy Character Device

This checkpoint registers one character device, creates `/dev/demo0`, and
implements a small in-kernel text buffer. It demonstrates the path from a
device number and `struct cdev` to file-operation callbacks.

```sh
make
sudo insmod ./lab_char.ko
ls -l /dev/demo0
cat /proc/devices | grep lab_char
printf 'hello from userspace\n' | sudo tee /dev/demo0 >/dev/null
cat /dev/demo0
dmesg | tail -n 30
sudo rmmod lab_char
```

`read()` and `write()` use `copy_to_user()`/`copy_from_user()` and protect the
buffer with a mutex. The interface is intentionally a teaching ABI; a real
device should normally use an established subsystem instead of inventing a
private character device.

