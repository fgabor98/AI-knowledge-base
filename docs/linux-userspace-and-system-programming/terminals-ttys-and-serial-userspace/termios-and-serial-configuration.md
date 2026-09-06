---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# termios And Serial Configuration

## What problem does this solve?

Serial settings persist in the device/driver state for the open session and can be
inherited from a previous application. If canonical mode, echo, parity, or flow
control is wrong, the protocol can appear corrupt even when the wiring is correct.

## Configuration sequence

```text
open device with O_NOCTTY | O_CLOEXEC
tcgetattr -> save original settings
modify a copy deliberately
cfmakeraw or explicit raw settings where appropriate
set baud/data/parity/stop/flow control
tcsetattr with chosen timing
flush only according to protocol policy
restore settings/close on exit if the device is a terminal
```

Use `O_NOCTTY` when a serial device must not become the process’s controlling
terminal. Preserve the original settings for interactive devices and restore them on
all normal cleanup paths. A service may instead own a dedicated port and configure it
from a known baseline on every open.

## Important settings

| Setting | Examples | Failure if wrong |
| --- | --- | --- |
| Input mode | `IGNBRK`, parity checks, `ISTRIP`, `IXON` | Bytes changed, dropped, or flow-controlled |
| Output mode | `OPOST`, newline translation | Binary payload altered |
| Control mode | `CS8`, `CLOCAL`, `CREAD`, parity, `CSTOPB` | Framing mismatch or no receiver |
| Local mode | `ICANON`, `ECHO`, `ISIG`, `IEXTEN` | Lines/signals/echo instead of raw bytes |
| Flow control | `CRTSCTS`, `IXON`/`IXOFF` | Deadlock or dropped data |
| Baud | `cfsetispeed`, `cfsetospeed` | Corruption or no communication |
| Read timing | `VMIN`, `VTIME` | Unexpected short reads or waits |

`cfmakeraw` is a useful starting point on systems that provide it, but inspect and
adjust the result. It does not select the device’s baud or solve protocol timeouts.
Check every `tcgetattr`, `tcsetattr`, `tcflush`, and modem-control call.

## Read timing is not framing

`VMIN`/`VTIME` control when a read returns in noncanonical mode. They do not identify
the end of a response and may interact with partial bytes and inter-byte gaps. Use a
parser with an absolute transaction deadline and an inter-byte policy for protocols
that need one.

## Modem control and hangup

`CLOCAL`, `HUPCL`, DTR/RTS, carrier detect, and USB-serial behavior vary by device and
driver. A close may drop modem control lines and reset the attached equipment. Treat
open/close as a hardware state transition and document it.

## The four noncanonical read modes

For a blocking descriptor, `VTIME` is measured in tenths of a second:

| VMIN | VTIME | Read behavior |
| --- | --- | --- |
| 0 | 0 | Return available bytes immediately; zero is possible without EOF |
| > 0 | 0 | Wait for the minimum byte count |
| 0 | > 0 | Timer starts at read; return on first byte or expiry |
| > 0 | > 0 | Wait indefinitely for the first byte, then use an inter-byte timer |

The last mode is a common trap: the timer does not bound silence before the
first byte. Count limits and implementation details also affect when a read
returns. With `O_NONBLOCK`, do not rely on VMIN/VTIME to enforce waits;
use the platform's documented behavior and readiness with an explicit deadline.
See [termios(3)](https://man7.org/linux/man-pages/man3/termios.3.html).

For a binary protocol, read existing settings, apply raw mode, explicitly set
baud, parity, stop bits, `CREAD`, `CLOCAL`, and hardware/software flow
control, then read the effective settings back. `TCSANOW` changes settings
immediately; `TCSADRAIN` waits for queued output; `TCSAFLUSH` also discards
unread input. A flush loses real bytes and belongs at a defined protocol boundary.

## Common mistakes

- Configuring only baud and ignoring line discipline/flow control.
- Using `cfmakeraw` without checking required parity or modem settings.
- Treating `VMIN`/`VTIME` as a complete response timeout.
- Forgetting `O_NOCTTY` in a noninteractive service.
- Not restoring terminal settings on an interactive tool.
- Flushing input/output at the wrong protocol boundary.
- Ignoring configuration return values and continuing with old settings.

## Debugging checklist

- Capture before/after `termios` settings and device identity.
- Check baud, data bits, parity, stop bits, flow control, carrier, and modem lines.
- Use a logic analyzer or loopback to separate electrical from parser faults.
- Test short reads, inter-byte gaps, disconnect, hangup, and reopen.
- Verify the service user’s device permissions and exclusive-access policy.
- Restore or reinitialize settings after every failed setup path.

## Related topics

- [Stage 8: Terminals, TTYs, And Serial Userspace](index.md)
- [TTY Processes And Pseudo-terminals](tty-processes-and-pseudo-terminals.md)
- [Serial Protocols, Timeouts, And Testing](serial-protocols-timeouts-and-testing.md)
- [Serial Input, Sensors, And hwmon](../hardware-facing-userspace-and-kernel-uapi/serial-input-sensors-and-hwmon.md)

## References

- [`termios(3)`](https://man7.org/linux/man-pages/man3/termios.3.html)
- [`tcsetattr(3)`](https://man7.org/linux/man-pages/man3/tcsetattr.3.html)
- [`cfmakeraw(3)`](https://man7.org/linux/man-pages/man3/cfmakeraw.3.html)
- [`serial(4)`](https://man7.org/linux/man-pages/man4/ttyS.4.html)
