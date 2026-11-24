# Lab 17: kprobe Events

This checkpoint instruments Lab 3 without editing or rebuilding it: one kprobe
event records the arguments of `demo_write()`, one kretprobe event its result.

```sh
make -C ../03-char-device
sudo insmod ../03-char-device/lab_char.ko
sudo ./kprobe-demo.sh
sudo rmmod lab_char
```

Requires `CONFIG_KPROBE_EVENTS` and, for `$argN`, x86-64 or arm64. Expected
evidence: an entry event with `count=13` followed by `ret=13`, and for the
200-byte write an entry event per call, showing the short write and the retry
of the rest.

`ustring` reads userspace memory. The buffer is not NUL-terminated, so the
recorded text can run past the data that was written.
