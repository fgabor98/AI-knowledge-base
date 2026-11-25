# Lab 8: Hard IRQ and Threaded IRQ

This checkpoint raises events on a dynamically allocated virtual IRQ, so it
runs without claiming a real hardware interrupt: the hard handler does minimal
work and returns `IRQ_WAKE_THREAD`, while the threaded handler performs the
observable work.

A timer provides the pace, but its callback runs in softirq context with
interrupts enabled. Calling `generic_handle_irq()` from there would run the
"hard" handler in the wrong context and make the kernel warn that the handler
enabled interrupts. The timer therefore queues an `irq_work`, whose callback
runs in hard interrupt context and injects the event.

Build and load it:

```sh
make
sudo insmod ./lab_irq.ko period_ms=1000
sleep 5
dmesg | tail -n 30
cat /proc/interrupts | grep lab_virtual_irq
sudo rmmod lab_irq
```

The interrupt count and `lab8_irq` messages are the success evidence. The hard
handler must report `hardirq=1 irqs_off=1`; the threaded handler must report
`hardirq=0 irqs_off=0` and a `comm` such as `irq/57-lab_virt`. Keep the
period at or above 10 ms while experimenting; the log is rate-limited in the
hard handler but intentionally verbose in the threaded handler.

This simulation teaches handler structure, not electrical trigger polarity or
hardware acknowledgement. A real platform driver should obtain its IRQ with
`platform_get_irq()` or a managed firmware-aware helper and acknowledge the
device in the appropriate handler context.

