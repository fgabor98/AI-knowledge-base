# Lab 11: Blocking I/O And poll

This checkpoint adds a wait queue to a character device. A delayed work item
produces one sample per period into a kfifo; readers block, get `-EAGAIN` with
`O_NONBLOCK`, or wait in `poll()`.

```sh
make
sudo insmod ./lab_poll.ko period_ms=500
sudo ./test_poll
sudo rmmod lab_poll
```

Every line must start with `ok`.

Things to observe:

- Start `sudo cat /dev/lab_poll | od -An -tu4` and press Ctrl-C. The read ends
  immediately because the wait is interruptible.
- While it runs, `sudo rmmod lab_poll` fails with "Module is in use". The open
  file holds a module reference through `.owner`.
- Start two readers. Each sample reaches only one of them. Replace the `while`
  in `lab_read()` with `if` and explain how the loser can return zero bytes,
  which userspace interprets as end of file.
