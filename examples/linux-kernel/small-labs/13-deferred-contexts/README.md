# Lab 13: Deferred Execution Contexts

One timer triggers three deferral mechanisms, and each reports where it runs:
a normal work item, a BH work item (the tasklet replacement, kernel 6.9 and
later), and a dedicated kernel thread.

```sh
make
sudo insmod ./lab_defer.ko
sleep 5
dmesg | grep lab13_defer | tail -n 8
sudo rmmod lab_defer
```

Expected evidence:

| Callback | `softirq` | `task` | May sleep |
| --- | --- | --- | --- |
| `timer` | 1 | 0 | no |
| `bh-work` | 1 | 0 | no |
| `work` | 0 | 1 | yes |
| `kthread` | 0 | 1 | yes |

Experiments:

- In a VM, add `msleep(1)` to `lab_bh_work_fn()`. The kernel reports
  `BUG: scheduling while atomic` with the offending stack, and the guest is not
  usable afterwards.
- Remove `kthread_should_stop()` from the wait condition, then try to unload.
  `rmmod` hangs in `kthread_stop()`. Do this only in a VM.
