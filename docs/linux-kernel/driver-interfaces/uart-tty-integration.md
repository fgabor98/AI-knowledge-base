---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# UART And TTY Integration Overview

## What Problem Does This Solve?

UART and TTY integration connects serial hardware drivers to Linux terminal, console, and line discipline behavior.

Most product work uses existing UART controller drivers. Driver development requires understanding where controller code ends and TTY behavior begins.

## Core Concepts

- UART controller
- serial core
- TTY layer
- console
- line discipline
- termios
- baud rate
- parity
- stop bits
- hardware flow control
- RS-485
- `/dev/ttyS*`
- `/dev/ttyAMA*`
- `/dev/ttyUSB*`
- `/dev/console`

## Mental Model

UART hardware moves serial bytes. The serial core and TTY layer provide the Linux userspace interface.

```text
UART controller driver
-> serial core
-> TTY device
-> line discipline
-> userspace /dev/tty*
```

If your task is board bring-up, you usually debug Device Tree, clocks, pinctrl, console bootargs, and getty configuration before writing a UART driver.

## Device Tree UART Node

Example:

```dts
&uart3 {
    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&uart3_default_pins>;
    pinctrl-1 = <&uart3_sleep_pins>;
    clocks = <&clkctrl 42>;
    status = "okay";
};
```

Optional RS-485-style properties are controller/binding-specific.

## Runtime Mapping

Find serial devices:

```sh
dmesg | grep -i tty
ls -l /dev/ttyS* /dev/ttyAMA* /dev/ttyUSB* 2>/dev/null
cat /proc/tty/driver/serial 2>/dev/null
```

Map Device Tree node to driver:

```sh
ls /sys/bus/platform/devices
readlink /sys/class/tty/ttyS0/device
```

The exact tty name depends on the driver.

## Console Configuration

Kernel command line:

```sh
cat /proc/cmdline
```

Example:

```text
console=ttyS0,115200n8
```

Console parameters include:

- device name
- baud rate
- parity
- data bits
- flow control where supported

If boot logs disappear, check:

- bootloader console
- kernel `console=`
- UART pinctrl
- UART clock
- driver enabled/built-in
- loglevel

## Getty And Login

A serial console device is not the same as a login prompt. Userspace often starts a getty:

```sh
systemctl status serial-getty@ttyS0.service
```

Enable:

```sh
systemctl enable --now serial-getty@ttyS0.service
```

On embedded systems, init system configuration determines whether a login prompt appears.

## Termios

Userspace configures serial settings through termios:

- baud rate
- parity
- stop bits
- character size
- flow control
- canonical/raw mode

Use:

```sh
stty -F /dev/ttyS0 -a
stty -F /dev/ttyS0 115200 raw -echo
```

Drivers implement hooks so serial core can apply these settings to hardware.

## Flow Control

Hardware flow control uses RTS/CTS lines.

Debug:

- pinmux for RTS/CTS
- cable wiring
- `crtscts` termios flag
- board-level inversion/buffer chips

Software flow control uses XON/XOFF and is handled higher in the TTY stack.

## RS-485

RS-485 often requires driver support for transmit-enable direction control.

Check:

- controller RS-485 support
- Device Tree properties
- GPIO direction control if used
- delay before/after send
- termination and biasing

Userspace may configure RS-485 through serial ioctls, depending on driver support.

## Writing UART Controller Drivers

A real UART controller driver usually integrates with serial core, not a custom character device.

Responsibilities include:

- register UART port
- implement transmit/receive operations
- handle interrupts
- handle FIFO and status bits
- implement termios configuration
- support console if needed
- handle suspend/resume
- support DMA if hardware/subsystem requires it

This is advanced compared with using an existing UART controller. Start by reading an existing driver for similar hardware.

## Debugging UART Bring-Up

Checklist:

```sh
cat /proc/cmdline
dmesg | grep -i -E 'tty|serial|uart'
ls /sys/class/tty
stty -F /dev/ttyS0 -a
```

Hardware:

- TX/RX crossed correctly
- voltage level correct, not RS-232 where TTL expected
- ground connected
- pinmux selected
- clock enabled
- baud rate accurate
- flow control disabled or wired correctly

Use a scope or logic analyzer for TX waveform and baud timing.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no tty device | driver/config/DT disabled | dmesg, `.config`, DT |
| tty exists, no output | pinmux, clock, console config | scope, cmdline |
| garbled text | baud or clock wrong | stty, clock rate |
| login absent | no getty | systemd/init config |
| TX works, RX not | wiring, pinmux, level, flow control | scope, stty |
| boot console stops | console handoff or driver init issue | dmesg, console args |
| RS-485 fails | direction control/termination/bias | driver support, hardware |

## Common Mistakes

- Confusing kernel console with userspace login.
- Debugging TTY settings before checking pinmux and electrical levels.
- Writing a custom char driver for serial hardware instead of serial core.
- Forgetting clock rate affects baud accuracy.
- Leaving hardware flow control enabled without CTS wiring.
- Assuming `/dev/ttyS0` naming is universal.

## Practice Exercises

### Exercise 1: Map A UART

Pick one `/dev/tty*` and trace it back to sysfs, driver, and Device Tree node.

### Exercise 2: Console Versus Getty

Remove or change getty configuration in a lab image and observe that kernel logs can still appear while login does not.

### Exercise 3: Baud Debug

Set mismatched baud rates between host and target and observe garbled output. Then fix with `stty`.

## Debugging Checklist

- Is the UART node enabled?
- Is pinctrl correct?
- Is the clock correct and enabled?
- Does dmesg register a tty?
- Is the kernel command line console correct?
- Is userspace getty configured if login is needed?
- Are termios settings correct?
- Is flow control intended and wired?
- Are voltage levels and wiring correct?

## Related Topics

- [Pinctrl](pinctrl.md)
- [Clocks](clocks.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)
- [Device Tree Hardware Description](../fundamentals/device-tree-hardware-description.md)
- [Embedded Linux](../../embedded-linux/index.md)

## Official References

- [TTY driver API](https://docs.kernel.org/driver-api/tty/index.html)
- [Serial driver documentation](https://docs.kernel.org/driver-api/serial/driver.html)
- [The kernel command line](https://docs.kernel.org/admin-guide/kernel-parameters.html)
