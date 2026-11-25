# Lab 14: RCU

Two reader threads check an invariant across two fields while a writer changes
both. With RCU the writer publishes a complete new object; with `broken=1` it
edits the live object in place.

```sh
make
sudo insmod ./lab_rcu.ko && sleep 3 && sudo rmmod lab_rcu
sudo insmod ./lab_rcu.ko broken=1 && sleep 3 && sudo rmmod lab_rcu
dmesg | grep lab14_rcu
```

Expected evidence: `torn=0` in `rcu` mode and a large `torn` count in
`in-place` mode. Needs at least two CPUs.

Look for these rules in the source:

- readers never use the pointer after `rcu_read_unlock()`
- `rcu_dereference()` and `rcu_assign_pointer()` style accessors only
- writers still take a mutex among themselves
- the old object is freed with `kfree_rcu()`, after a grace period
- `lab_reads` is a per-CPU counter, summed only for reporting

Experiment in a VM with KASAN: replace `kfree_rcu(old, rcu)` with `kfree(old)`
and read the use-after-free report.
