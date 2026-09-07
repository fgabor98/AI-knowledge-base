---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# TTY Processes And Pseudo-terminals

## What problem does this solve?

Terminal behavior depends on sessions, process groups, a controlling terminal, and a
line discipline. Pseudo-terminals (PTYs) reproduce this behavior for SSH, terminal
emulators, tests, and expect-like tools without requiring physical hardware.

## TTY roles

```text
terminal emulator / SSH / PTY master
              |
              v
        PTY slave (TTY device)
              |
              v
       session leader / shell / foreground process group
```

The PTY master is controlled by the emulator or test harness. The slave looks like a
terminal to the shell or program. The line discipline sits between them and can
perform canonical input buffering, echo, special-character handling, and signal
generation.

## Controlling terminal

A session can have one controlling terminal. The terminal tracks a foreground process
group. Input and terminal-generated signals go to that group; background access may
produce `SIGTTIN`/`SIGTTOU`. A service should not depend on a controlling terminal;
interactive tools may deliberately require one.

Inspect:

```sh
ps -o pid,ppid,pgid,sid,tpgid,stat,tty,cmd
tty
stty -a
ls -l /dev/pts
mountpoint /dev/pts
```

`/dev/pts` is a devpts filesystem. Its availability and mount options are part of a
container or target rootfs contract.

## Canonical and raw input

Canonical mode buffers input until a line delimiter and interprets special characters.
Noncanonical mode returns based on `VMIN` and `VTIME`, but these settings are not a
complete transaction timeout. A raw serial protocol normally disables canonical
processing, echo, signal characters, and unwanted input/output translations, then
adds its own framing and deadline logic.

## PTY lifecycle

Create a PTY master, obtain/unlock the slave, and give the slave to the child or test
subject. The master observes the subject’s output and supplies input. Closing one side
produces hangup/EOF behavior that differs from a clean protocol close; test both.

PTYs are not electrically faithful serial ports. They do not model baud timing,
parity errors, UART FIFO behavior, USB latency, power loss, or modem control lines.
They are ideal for line discipline, framing, command/response, and process-lifecycle
tests.

## Common mistakes

- Assuming a TTY is a raw byte stream.
- Leaving echo or canonical mode enabled for a binary protocol.
- Ignoring controlling-terminal and foreground-process-group rules.
- Treating PTY tests as proof of UART electrical behavior.
- Forgetting `/dev/pts` mount and namespace requirements.
- Closing the master/slave side without testing resulting HUP/EOF behavior.

## Debugging checklist

- Record device path, session, PGID, foreground TPGID, and line discipline.
- Capture `stty -a`/`termios` settings before and after configuration.
- Inspect `/dev/pts`, mount namespace, permissions, and process groups.
- Test canonical/raw, echo, signal characters, HUP, EOF, and background access.
- Separate PTY protocol evidence from physical UART/USB evidence.

## Related topics

- [Stage 8: Terminals, TTYs, And Serial Userspace](index.md)
- [termios And Serial Configuration](termios-and-serial-configuration.md)
- [Sessions, Process Groups, And Job Control](../processes-and-program-lifetime/sessions-process-groups-and-job-control.md)
- [Serial Protocols, Timeouts, And Testing](serial-protocols-timeouts-and-testing.md)

## References

- [`pty(7)`](https://man7.org/linux/man-pages/man7/pty.7.html)
- [`termios(3)`](https://man7.org/linux/man-pages/man3/termios.3.html)
- [`credentials(7)`](https://man7.org/linux/man-pages/man7/credentials.7.html)
- [`devpts(5)`](https://man7.org/linux/man-pages/man5/devpts.5.html)
