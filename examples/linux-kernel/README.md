# Linux Kernel Examples

These examples are small, external kernel modules intended for a disposable
VM, QEMU guest, or recoverable development board. They are teaching artifacts,
not production drivers.

The [small driver lab progression](small-labs/README.md) keeps one checkpoint
per lab. Build a checkpoint against the exact kernel that will load it:

```sh
make -C examples/linux-kernel/small-labs/01-hello
```

Kernel modules execute with kernel privileges. Read the checkpoint README
before loading a module, unload modules in the documented order, and keep a
known-good boot path available.

