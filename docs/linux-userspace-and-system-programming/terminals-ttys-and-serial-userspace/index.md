---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 8: Terminals, TTYs, And Serial Userspace

Terminals and serial ports are byte-oriented interfaces with line discipline,
foreground-process, flow-control, and timing behavior layered on top. A reliable
serial tool configures the port deliberately, treats bytes as an incomplete stream,
and tests framing and timeout behavior independently from the physical UART.

## The layered model

```text
application protocol
        |
read/write bytes
        |
TTY line discipline and termios settings
        |
serial driver / USB-serial adapter
        |
UART, modem, or external device
```

A terminal connected to a shell is not equivalent to a raw serial port. The default
line discipline may echo, buffer until newline, translate characters, generate
signals, and interpret control characters. A serial protocol normally wants raw mode
and explicit framing.

## Learning materials

1. [TTY Processes And Pseudo-terminals](tty-processes-and-pseudo-terminals.md)
2. [termios And Serial Configuration](termios-and-serial-configuration.md)
3. [Serial Protocols, Timeouts, And Testing](serial-protocols-timeouts-and-testing.md)

## Stage lab

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-termios-pty.c -o /tmp/termios-pty
/tmp/termios-pty
```

The probe creates a pseudo-terminal, applies raw-ish settings to its slave, and
exchanges bytes through the master. It demonstrates terminal APIs without requiring
a board or USB adapter.

## Serial contract checklist

| Area | Decision |
| --- | --- |
| Device identity | Stable discovery rule, permissions, and reconnect behavior |
| Electrical mode | Baud, data bits, parity, stop bits, flow control, inversion if applicable |
| Line discipline | Raw/canonical, echo, signal generation, translations |
| Framing | Header, length, checksum, delimiter, maximum frame |
| Timing | Inter-byte, response, transaction, and startup deadlines |
| Recovery | Flush, reset, reopen, resynchronize, or fail degraded |
| Ownership | One reader/writer or serialized access; FD and termios owner |
| Testing | PTY, fake device, fault injection, and real hardware matrix |

## Completion criteria

You can complete this stage when you can:

- distinguish controlling terminals, TTYs, PTYs, and serial devices;
- configure `termios` without accidentally retaining echo, canonical mode, or flow
  control from a previous owner;
- design a bounded serial frame parser with inter-byte and transaction deadlines;
- recover from framing errors, disconnects, partial responses, and device reset;
- test protocol logic through a PTY/fake and validate electrical behavior on hardware.

## Related topics

- [Stage 7: IPC And Event-Driven Design](../ipc-and-event-driven-design/index.md)
- [Stage 9: Userspace Networking](../userspace-networking/index.md)
- [Serial Input, Sensors, And hwmon](../hardware-facing-userspace-and-kernel-uapi/serial-input-sensors-and-hwmon.md)
- [Signal Model And sigaction](../time-clocks-and-signals/signal-model-and-sigaction.md)

## References

- [`termios(3)`](https://man7.org/linux/man-pages/man3/termios.3.html)
- [`pty(7)`](https://man7.org/linux/man-pages/man7/pty.7.html)
- [`tty(4)`](https://man7.org/linux/man-pages/man4/tty.4.html)
