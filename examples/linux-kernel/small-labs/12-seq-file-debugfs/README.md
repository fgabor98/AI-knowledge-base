# Lab 12: seq_file And debugfs

This checkpoint prints a kernel list through `/proc/lab_seq` with a `seq_file`
iterator, and provides diagnostic controls in debugfs.

```sh
make
sudo insmod ./lab_seq.ko
cat /proc/lab_seq
sudo sh -c 'cd /sys/kernel/debug/lab_seq && ls -l && cat summary tunable'
echo 5000 | sudo tee /sys/kernel/debug/lab_seq/add
wc -l /proc/lab_seq
dd if=/proc/lab_seq bs=7 count=3 2>/dev/null
echo 1 | sudo tee /sys/kernel/debug/lab_seq/clear
cat /proc/lab_seq
sudo rmmod lab_seq
```

`wc -l` must report 5003 lines. That is far more than one page, so `seq_file`
called `start()` and `stop()` many times; the iterator never dealt with buffer
sizes or partial reads. The `dd` command shows the same for tiny reads.

`/proc` is used here to learn the API. A new driver should not add `/proc`
files: use sysfs for device state and debugfs for diagnostics. `seq_file` works
the same in debugfs.

Experiment: add 100000 items, then `strace -e read cat /proc/lab_seq` and note
how many bytes each `read()` returns.
