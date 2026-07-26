# Lab 9: Basic Tracing

This checkpoint observes the Lab 8 module with trace events instead of adding
more `printk()` calls. Keep `lab_irq.ko` loaded in another terminal, then run:

```sh
make -C ../08-threaded-irq
sudo insmod ../08-threaded-irq/lab_irq.ko period_ms=1000
sudo ./trace-demo.sh 5
sudo rmmod lab_irq
```

The script records IRQ, workqueue, and timer events for the requested number of
seconds and prints the report. Use the report to compare the timer event, hard
handler, and threaded handler ordering.

If `trace-cmd` is unavailable, the equivalent manual tracefs exercise from the
lab page can be used. Tracing support and event names depend on the kernel
configuration.

