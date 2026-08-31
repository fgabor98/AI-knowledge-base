---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Linux Userspace And System Programming

Linux userspace topics from a programmer, embedded engineer, and system integrator perspective.

This is the practical boundary between C programs, the Linux kernel, hardware-facing interfaces, and a deployable embedded product. It explains what a userspace program can observe and request, how processes and resources behave, how drivers expose interfaces, and how services are built, tested, secured, and operated on a target.

The track assumes basic C, shell usage, and general Linux command-line experience. It does not replace the C, kernel, Device Tree, build-system, networking, debugging, or productization tracks; it connects them.

## What This Track Should Teach

By the end, a learner should be able to:

- explain the boundary between an application, libc, a system call, the kernel, and hardware;
- read the relevant Linux, POSIX, libc, and subsystem documentation for an interface;
- write reliable C programs that use files, processes, threads, signals, IPC, sockets, and timers;
- understand file-descriptor ownership, blocking behavior, partial I/O, cancellation, and failure;
- choose between a kernel driver, an existing kernel subsystem ABI, a userspace helper, and firmware;
- consume a driver through `/dev`, sysfs, uevents, `ioctl`, `poll`, `mmap`, or a standard subsystem API;
- build an event-driven hardware-facing daemon with bounded resources and explicit recovery behavior;
- package and start that daemon on an embedded target;
- diagnose startup, permission, ABI, dynamic-linking, device, performance, and runtime failures;
- test the program on a host, in an emulated target where useful, and on real hardware;
- design a userspace component that remains understandable when the device is offline, degraded, upgraded, or rebooted.

## How To Read Linux Interfaces

Before learning a facility, identify the contract layer:

| Layer | Typical examples | Primary questions |
| --- | --- | --- |
| ISO C | `malloc`, `memcpy`, `fopen`, `printf` | What does the C standard guarantee? |
| libc | glibc, musl, uClibc-ng | Which implementation and feature macros are present? |
| POSIX | `open`, `read`, `pthread_create`, `clock_gettime` | What portability and error rules apply? |
| Linux system call | `epoll`, `eventfd`, `signalfd`, `io_uring` | What Linux-specific behavior is required? |
| Kernel UAPI | `/dev`, `ioctl`, sysfs, netlink, standard subsystems | What userspace ABI is intentionally exposed? |
| Distribution or init system | systemd, BusyBox, package manager | How is the program deployed and supervised? |
| Board or product | GPIO wiring, calibration, update policy | What target-specific assumptions are allowed? |

Learn to use `man 2` for system calls, `man 3` for libc/POSIX functions, `man 5` for file formats and configuration, and `man 7` for overviews. Always check the target's libc, kernel version, enabled features, and init system instead of assuming that a desktop Linux environment is representative.

## Learning Path

The stages are ordered so that each one produces useful programs. The later stages are more valuable when implemented as extensions of one running embedded service rather than as isolated exercises.

### Stage 0: Environment And Mental Model

- hosted C process versus freestanding code and kernel code;
- application, libc, dynamic linker, system-call entry, kernel subsystem, driver, and hardware;
- user mode versus kernel mode and the purpose of privilege separation;
- virtual addresses, process address spaces, file descriptors, credentials, and namespaces as process resources;
- POSIX interfaces versus Linux extensions versus vendor or product APIs;
- reading manual-page synopsis, feature-test macros, return values, `errno`, cancellation, and thread-safety notes;
- host development machine versus target root filesystem, kernel, architecture, and ABI;
- setting up a repeatable host/target lab with a serial console, SSH or another transport, symbols, and captured logs;
- compiling with warnings and debug information, running a small program, observing it with `strace`, and inspecting it with `/proc`;
- learning which operations can block, allocate, fail transiently, be interrupted, or depend on global process state.

### Stage 1: Linux Runtime, Filesystem, And Rootfs

#### Filesystem and path model

- the purpose of `/`, `/bin`, `/sbin`, `/usr`, `/lib`, `/lib64`, `/etc`, `/dev`, `/proc`, `/sys`, `/run`, `/var`, `/tmp`, and `/home`;
- pathnames, current working directory, absolute versus relative paths, symlinks, hard links, mount points, and bind mounts;
- files, directories, inodes, file metadata, permissions, ownership, timestamps, and file types;
- `stat`, `lstat`, `fstat`, `readlink`, `realpath`, directory iteration, and directory file descriptors;
- safe path handling with `openat`, `fstatat`, `renameat`, `unlinkat`, and `O_CLOEXEC`;
- symbolic-link attacks, time-of-check/time-of-use races, untrusted paths, and safe temporary files;
- configuration, runtime state, caches, logs, sockets, locks, and temporary data as different classes of data.

#### Mounts and embedded root filesystems

- mount and unmount concepts, mount flags, `fstab`, and boot-time filesystem availability;
- `procfs`, `sysfs`, `devtmpfs`, `tmpfs`, `debugfs`, `configfs`, and their intended uses;
- initramfs, the transition to the real root filesystem, and early-userspace responsibilities;
- read-only root filesystems, overlayfs, bind mounts, persistent-data partitions, and volatile state;
- filesystem image formats and deployment assumptions without duplicating the storage deep dive in Embedded Linux;
- why a path may exist on the host but not on the target;
- mount namespaces and per-service filesystem views at an introductory level;
- detecting missing mounts, stale runtime directories, full filesystems, and read-only remounts.

#### Deployment and dynamic linking basics

- executable permissions, interpreter selection, shebangs, and `execve`;
- ELF program headers, the dynamic loader, shared-library lookup, and ABI compatibility;
- `readelf`, `ldd` or safer equivalent inspection, `nm`, `objdump`, and `file`;
- static versus dynamic linking and the footprint, update, and diagnostic tradeoffs;
- package file ownership, configuration-file policy, service users, and runtime directories;
- environment variables, `PATH`, `LD_LIBRARY_PATH`, `LD_PRELOAD`, and why environment-based behavior must be controlled in services.

### Stage 2: Processes And Program Lifetime

#### Process creation and replacement

- the difference between a program, process, thread, task, and executable image;
- process IDs, parent process IDs, thread IDs, process groups, sessions, and the process tree;
- `fork`, `vfork`, `clone` as concepts, and when ordinary applications should avoid low-level process creation;
- copy-on-write after `fork` and what is inherited by the child;
- `execve`, `execveat`, argument vectors, environment vectors, interpreter scripts, and failed `exec` behavior;
- `posix_spawn` and when it is preferable to a manual `fork`/`exec` sequence;
- close-on-exec discipline and preventing descriptor leaks into child programs.

#### Exit, waiting, and supervision

- `exit`, `_exit`, return status, `atexit`, and buffered I/O during termination;
- `wait`, `waitpid`, `waitid`, exit-status decoding, and `SIGCHLD`;
- zombies, orphans, reparenting, subreapers, and PID 1 responsibilities;
- preventing double-fork folklore from replacing an explicit supervisor;
- process groups, sessions, controlling terminals, and signal delivery to groups;
- graceful shutdown, termination deadlines, escalation to `SIGKILL`, and child cleanup;
- restart loops, crash storms, backoff, and preserving the first failure evidence.

#### Process observation and control

- `/proc/PID/status`, `cmdline`, `environ`, `cwd`, `exe`, `fd`, `fdinfo`, `maps`, `smaps`, `limits`, `stat`, and `task`;
- process states, runnable versus sleeping versus blocked processes, and basic scheduler observations;
- `ps`, `top`, `pgrep`, `pkill`, `pidof`, `kill`, `nice`, `renice`, and `chrt`;
- process groups and job control in an interactive shell;
- environment, current directory, umask, resource limits, credentials, and inherited descriptors as part of process identity;
- distinguishing a process that crashed, exited, hung, blocked on I/O, was killed, or was restarted by a supervisor.

### Stage 3: System Calls, Files, And File Descriptors

#### System-call discipline

- libc wrappers versus direct system calls;
- return values, `errno`, `EINTR`, `EAGAIN`/`EWOULDBLOCK`, `ECONNRESET`, `ENODEV`, `ENXIO`, `ETIMEDOUT`, and other common failure classes;
- feature-test macros, Linux headers, ABI types, 32-bit versus 64-bit concerns, and time-related interfaces;
- partial success, short reads and writes, interrupted operations, and retry policies;
- cancellation and timeout behavior for every blocking operation;
- checking errors from `close`, `fsync`, `munmap`, `pthread_*`, and cleanup operations rather than only the main call;
- writing small wrappers that preserve the original error and make ownership explicit.

#### File descriptors and open-file state

- descriptors as per-process references to open-file descriptions;
- descriptor numbers, open flags, file status flags, descriptor flags, offsets, and sharing after `fork`;
- `open`, `openat`, `creat`, `close`, `read`, `write`, `pread`, `pwrite`, `lseek`, `truncate`, and `ftruncate`;
- `dup`, `dup2`, `dup3`, descriptor redirection, standard input/output/error, and descriptor inheritance;
- `fcntl`, `FD_CLOEXEC`, `O_CLOEXEC`, `O_NONBLOCK`, append semantics, and atomicity boundaries;
- blocking and nonblocking descriptors, readiness versus completion, and why readiness does not remove the need to handle errors;
- descriptor ownership conventions, cleanup order, and avoiding use-after-close or double-close bugs;
- `close_range`, `pidfd`, and other newer Linux facilities as optional advanced material.

#### Regular files and directories

- buffered stdio versus descriptor-based I/O and choosing deliberately between `FILE *` and raw descriptors;
- file creation modes, umask, permission checks, and ownership changes;
- `stat`, `chmod`, `chown`, `utimensat`, `link`, `unlink`, `rename`, and atomic replacement;
- `fsync`, `fdatasync`, directory syncing, durability, and the difference between returning from a write and surviving power loss;
- advisory locking with `flock` and `fcntl`, lock lifetime, stale locks, and why locks do not enforce cooperation by themselves;
- `sendfile`, `splice`, `copy_file_range`, direct I/O, and zero-copy techniques as optional performance topics;
- file-descriptor-relative APIs and race-resistant filesystem operations;
- quotas, read-only filesystems, disk-full behavior, and handling partial persistence.

#### Pipes and shell integration

- anonymous pipes, named FIFOs, `pipe2`, `dup2`, and pipeline construction;
- EOF, broken pipes, `SIGPIPE`, `EPIPE`, buffering, and backpressure;
- shell redirection versus programmatic descriptor management;
- producer/consumer shutdown and avoiding processes waiting forever for an unread or unclosed end;
- when a pipe is sufficient and when a framed protocol or Unix socket is more appropriate.

### Stage 4: Process Memory And Mapping

- virtual address spaces, pages, page tables, protection bits, and user/kernel address separation;
- text, read-only data, writable data, BSS, heap, stack, thread-local storage, shared libraries, and mapped files;
- anonymous `mmap`, file-backed mappings, `munmap`, `mprotect`, and alignment requirements;
- copy-on-write, page faults, lazy allocation, demand paging, and memory-mapped file behavior;
- shared memory using `mmap`, `shm_open`, `memfd_create`, and explicit synchronization;
- guard pages, stack limits, stack growth, alternate signal stacks, and diagnosing stack exhaustion;
- ASLR, PIE, NX, RELRO, and the relationship between build flags and process memory protection;
- RSS, virtual size, shared pages, dirty pages, memory pressure, overcommit, and the OOM killer;
- `mlock` and real-time memory considerations;
- mapping device memory only through a documented UAPI, never by treating a physical address as an ordinary pointer;
- cache coherency, DMA, and memory barriers as a boundary to kernel and hardware material rather than a userspace shortcut.

### Stage 5: Time, Clocks, And Signals

#### Time and timers

- wall-clock time versus monotonic elapsed time;
- `clock_gettime`, `clock_nanosleep`, `nanosleep`, `gettimeofday`, and why wall-clock time is unsuitable for most timeouts;
- `CLOCK_MONOTONIC`, `CLOCK_BOOTTIME`, `CLOCK_REALTIME`, raw clocks, suspend behavior, and clock adjustments;
- absolute versus relative deadlines and avoiding timeout extension caused by repeated `EINTR`;
- POSIX timers, timer signals, `timerfd`, alarms, and timer ownership;
- RTC, NTP, PTP, timestamp domains, and time synchronization at an embedded-system level;
- periodic work, drift, missed deadlines, watchdog intervals, and timer coalescing.

#### Signals

- signal disposition, default actions, ignored signals, blocked signals, pending signals, and per-thread masks;
- `sigaction`, reliable signal handling, `SA_RESTART`, `SIGCHLD`, `SIGTERM`, `SIGINT`, `SIGHUP`, `SIGPIPE`, and fatal signals;
- async-signal-safe functions and why ordinary locks, allocation, stdio, and most library calls are unsafe in a signal handler;
- self-pipe and `signalfd` designs for integrating signals into an event loop;
- signal delivery to processes, threads, process groups, and supervisors;
- graceful shutdown and reload protocols;
- why signals are a poor general-purpose message bus.

### Stage 6: Threads And Userspace Concurrency

- POSIX threads, thread creation, joining, detaching, naming, and lifetime ownership;
- thread stacks, attributes, guard sizes, thread-local storage, and process-wide versus per-thread state;
- mutexes, recursive mutexes, robust mutexes, condition variables, read/write locks, barriers, and semaphores;
- condition-variable predicates, spurious wakeups, lost wakeups, and correct wait loops;
- C atomics and POSIX synchronization: when each is appropriate and how they interact;
- data races, deadlocks, lock ordering, lock scope, priority inversion, and priority-inheritance options;
- cancellation points, deferred versus asynchronous cancellation, cleanup handlers, and safer cooperative shutdown;
- reentrancy, thread-safe library use, `errno` and thread-local state;
- communicating ownership and lifetime across worker threads;
- producer/consumer queues, bounded buffers, backpressure, and shutdown markers;
- CPU affinity, scheduler policy, real-time priorities, `SCHED_FIFO`/`SCHED_RR`, and why priority changes require system-level reasoning;
- diagnosing a hung thread, a deadlock, a starvation problem, or a data race;
- futexes as the kernel primitive behind many userspace locks, for conceptual understanding rather than routine direct use.

### Stage 7: IPC And Event-Driven Design

#### IPC choices

- anonymous pipes and FIFOs for simple streams;
- Unix-domain stream and datagram sockets for local request/response and notification protocols;
- `socketpair` for related processes and testable bidirectional channels;
- shared memory and `memfd_create` for large or high-rate data, with explicit ownership and synchronization;
- POSIX message queues and System V IPC as compatibility knowledge and maintenance concerns;
- `eventfd` for counter or wakeup notifications;
- `timerfd` for timer events and `signalfd` for signal events;
- netlink for selected Linux kernel configuration and event interfaces;
- D-Bus as an optional service-bus topic, including when its dependencies and runtime cost fit an embedded product;
- choosing IPC by message size, rate, ownership, failure behavior, security, observability, and versioning.

#### Protocol and lifecycle design

- message framing, lengths, maximum sizes, byte order, alignment, and serialization;
- request IDs, replies, asynchronous events, cancellation, and duplicate requests;
- version negotiation, forward and backward compatibility, reserved fields, and feature discovery;
- queue bounds, backpressure, overload behavior, and priority inversion between producers and consumers;
- peer identity using credentials such as `SO_PEERCRED` where appropriate;
- handling peer death, half-open connections, stale sockets, restart, and reconnection;
- idempotent commands and safe retry behavior;
- failure containment between a hardware-facing service and its clients;
- documenting an IPC interface as a product contract rather than as an internal implementation detail.

#### Readiness and event loops

- `select`, `poll`, `ppoll`, `epoll`, and the reason for using `pselect`/`ppoll` with signal masks;
- level-triggered versus edge-triggered readiness;
- nonblocking accept, connect, read, write, and drain loops;
- `EPOLLIN`, `EPOLLOUT`, `EPOLLERR`, `EPOLLHUP`, `EPOLLRDHUP`, one-shot notifications, and rearming;
- integrating sockets, device descriptors, pipes, `eventfd`, `timerfd`, and `signalfd` into one loop;
- filesystem notifications with `inotify` and the limitations of treating file changes as complete state updates;
- fairness, starvation, wakeup storms, and bounded work per event;
- descriptor close/reuse races and event-loop ownership rules;
- timeout, cancellation, shutdown, and error paths in event-driven code;
- `io_uring` as optional advanced Linux-specific material, with attention to embedded kernel support and operational complexity.

### Stage 8: Terminals, TTYs, And Serial Userspace

- terminal devices, TTYs, line disciplines, controlling terminals, and pseudo-terminals;
- `termios`, canonical versus raw mode, echo, signal generation, special characters, and input buffering;
- baud rate, character size, parity, stop bits, hardware/software flow control, and modem-control lines;
- blocking reads, `VMIN`/`VTIME`, `poll`, timeout design, and serial disconnect behavior;
- serial framing, partial messages, checksums, resynchronization, and protocol timeouts;
- UART console versus application serial port and avoiding interference with boot logs;
- PTYs for terminal emulation, SSH-like testing, and deterministic host-side tests;
- tools such as `stty`, `setserial` where available, `screen`, and `minicom`, while keeping the protocol logic in testable code.

### Stage 9: Userspace Networking

This stage covers socket programming and network behavior. Ethernet MAC/PHY bring-up and board wiring remain in the [Networking](../networking/index.md) track.

- socket domains, types, protocols, addresses, and byte order;
- `getaddrinfo`, `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `sendmsg`, `recvmsg`, and shutdown;
- TCP stream semantics, partial sends and receives, connection states, half-close, reset, keepalive, and reconnect;
- UDP datagrams, message boundaries, loss, duplication, ordering, broadcast, and multicast;
- Unix-domain sockets versus TCP loopback sockets;
- IPv4 and IPv6 dual-stack behavior, address selection, and interface binding;
- blocking and nonblocking sockets, connect timeouts, read/write deadlines, and event-loop integration;
- socket options, buffer sizes, `SO_REUSEADDR`, `SO_REUSEPORT`, `TCP_NODELAY`, credentials, and ancillary data;
- `sendmsg`/`recvmsg`, file-descriptor passing, and Unix-socket control messages;
- DNS and resolver configuration as a runtime dependency;
- TLS library integration, certificate storage, verification, timeouts, and failure policy;
- network service startup ordering, link loss, DHCP changes, time synchronization, and offline operation;
- diagnosing sockets with `ss`, `ip`, `ethtool`, `tcpdump`, and `/proc/net`.

### Stage 10: Hardware-Facing Userspace And Kernel UAPI

#### Choosing the boundary

- deciding whether functionality belongs in hardware, firmware, a kernel driver, an existing kernel subsystem, or userspace;
- latency, interrupt handling, DMA, power management, security, sharing, and recovery as boundary criteria;
- why a userspace prototype may use a raw bus interface while a product should use a proper kernel subsystem;
- stable userspace ABI versus private implementation details;
- separating board wiring in Device Tree, hardware policy in the driver, and product policy in userspace;
- keeping the userspace application independent from unstable register layouts and vendor-specific details.

#### Device discovery and permissions

- `/dev` device nodes, major/minor numbers, character versus block devices, and open-time failures;
- `devtmpfs`, uevents, udev, device rules, symlinks, tags, and persistent naming;
- sysfs device topology, attributes, symlinks, modaliases, and runtime inspection;
- hotplug races, device appearance/disappearance, deferred availability, and retry policy;
- ownership, groups, udev permissions, Linux capabilities, and service sandboxing;
- never assuming that a device node implies a bound or healthy driver;
- discovering the relationship between a device node, sysfs object, driver, Device Tree node, and physical hardware.

#### Userspace ABI patterns

- `read`/`write` interfaces for streams and commands;
- sysfs attributes for small, human-readable, kernel-owned state;
- `ioctl` for structured control operations and its type, number, direction, size, alignment, and error conventions;
- `poll`/`select`/`epoll` for readiness and asynchronous device events;
- `mmap` for documented shared buffers or device memory;
- `readv`/`writev`, `preadv`/`pwritev`, and `splice` where a subsystem uses them;
- blocking, nonblocking, timeout, cancellation, and close semantics for every operation;
- 32-bit/64-bit compatibility, pointer-free UAPI structures, fixed-width types, padding, reserved fields, and endianness;
- ABI versioning, feature discovery, extensibility, and never changing the meaning of an existing field;
- event records, sequence numbers, timestamps, overflow reporting, and lost-event recovery;
- security checks and authorization at the kernel boundary rather than trusting the caller;
- documenting ownership, lifetime, concurrency, and error behavior for the interface.

#### Common embedded subsystem APIs

- GPIO character-device v2 and line-request ownership;
- I2C userspace access through `i2c-dev`, including address ownership and product limitations;
- SPI userspace access through `spidev`, transfer boundaries, chip select, and when a kernel client driver is required;
- serial and TTY devices;
- input event devices and evdev;
- IIO channels, raw versus processed values, triggers, buffers, timestamps, and calibration;
- hwmon sensors and alarms;
- RTC devices and time-setting permissions;
- CAN sockets and SocketCAN concepts;
- watchdog devices and keepalive ownership;
- LEDs, PWM, regulators, and power controls when exposed through standard interfaces;
- V4L2/media, ALSA, DRM, USB, and PCIe as optional subsystem-specific branches;
- UIO and VFIO as specialized mechanisms with explicit security and ownership constraints;
- firmware loading, remoteproc/RPMsg, and heterogeneous-SoC interfaces as an advanced branch.

### Stage 11: Services, Init, And systemd

#### Service model

- PID 1, early userspace, init systems, and the transition to application services;
- BusyBox init and simple init scripts as useful embedded knowledge;
- systemd units, unit types, targets, dependencies, ordering, conditions, and conflicts;
- `ExecStart`, `ExecStartPre`, `ExecStartPost`, `ExecStop`, `Environment`, `EnvironmentFile`, `WorkingDirectory`, `User`, and `Group`;
- `Type=simple`, `exec`, `forking`, `notify`, and `oneshot`, including readiness semantics;
- restart policy, start limits, failure status, backoff, and avoiding restart loops;
- `KillSignal`, `TimeoutStartSec`, `TimeoutStopSec`, `KillMode`, and graceful shutdown;
- service dependencies versus ordering dependencies and avoiding unnecessary coupling;
- socket activation, timer activation, path activation, mount units, device units, and udev-triggered work;
- service users, runtime directories, state directories, cache directories, and temporary files.

#### Operability

- journald, structured fields, log levels, rate limits, persistent versus volatile logs, and log privacy;
- `journalctl`, `systemctl status`, `systemctl show`, dependency inspection, and boot analysis;
- `sd_notify`, readiness, status messages, watchdog notifications, and watchdog failure behavior;
- reload protocols, configuration validation before reload, and atomic configuration replacement;
- signal-safe shutdown, draining work, closing devices, flushing state, and returning meaningful exit status;
- resource limits, cgroups, sandboxing, private temporary directories, filesystem protection, and capability bounding;
- service ordering around network, storage, time synchronization, firmware, and device availability;
- service behavior during boot, brownout recovery, update, rollback, and repeated restart;
- making a service safe to start more than once and safe to stop at any point.

### Stage 12: Identity, Privilege, And Userspace Security

- real, effective, saved, and filesystem user/group IDs;
- supplementary groups, ownership, permissions, umask, ACLs, and access checks;
- root as an authority, not as a convenience for normal application code;
- Linux capabilities, inheritable/permitted/effective/ambient sets, bounding sets, and capability-aware design;
- dropping privileges after privileged setup and avoiding confused-deputy behavior;
- `setuid`, `setgid`, `no_new_privs`, securebits, and set-user-ID risks;
- Unix-socket peer credentials and authorization of local clients;
- namespaces for mount, PID, network, user, IPC, and UTS isolation;
- cgroups for resource control and containment;
- seccomp filtering and the difference between reducing attack surface and providing authorization;
- SELinux, AppArmor, and other LSM policy concepts;
- read-only rootfs, immutable system files, writable-data boundaries, and service sandboxing;
- safe handling of device nodes, sysfs writes, firmware files, keys, certificates, calibration data, and update artifacts;
- path traversal, symlink and TOCTOU attacks, unsafe temporary files, command injection, environment attacks, and parser bugs;
- compiler and linker hardening as an application-build concern: PIE, RELRO, stack protection, fortify, and symbols;
- least privilege, fail-closed behavior, auditability, and security evidence for production images.

### Stage 13: Persistent State, Storage, And Power-Loss Behavior

- distinguishing immutable application files, configuration, runtime state, user data, caches, logs, and crash artifacts;
- choosing volatile versus persistent storage for every piece of state;
- atomic write pattern: temporary file, `fsync`, rename, directory sync, and recovery from an interrupted update;
- configuration schema, defaults, migration, rollback, validation, and corruption handling;
- lock files, PID files, Unix socket paths, stale state, and single-instance ownership;
- `tmpfs`, overlayfs, bind mounts, persistent partitions, and read-only-rootfs design;
- ext4, squashfs, UBI/UBIFS, and eMMC/NAND behavior at the level needed by an application owner;
- flash wear, write amplification, log rotation, bounded logging, and avoiding high-rate writes to persistent media;
- `fsync` limitations, storage caches, barriers, sudden power loss, and brownout behavior;
- detecting and recovering from full, missing, corrupted, or unexpectedly read-only storage;
- coordinating state with updates, A/B slots, rollback, factory reset, and version changes;
- preserving diagnostic evidence without allowing logs to consume the recovery path.

### Stage 14: Diagnostics, Debugging, And Performance

This stage applies the general [Debugging And Diagnostics](../debugging/index.md) methods to userspace programs. It should teach evidence collection and reasoning, not a memorized tool list.

#### Failure investigation

- classify startup failure, permission failure, missing dependency, device failure, protocol failure, hang, crash, data corruption, and resource exhaustion;
- preserve the first failure: command line, environment, kernel log, service status, timestamps, versions, and target identity;
- reduce a failure to the smallest reproducer and separate host, target, kernel, hardware, and deployment variables;
- distinguish application bugs from kernel UAPI misuse, driver probe failures, Device Tree errors, packaging mistakes, and service-ordering errors;
- build a hypothesis, collect discriminating evidence, change one variable, and record the result;
- test cold boot, warm reboot, service restart, device removal, network loss, storage full, power interruption, and update interruption.

#### Tools and evidence

- `strace` for system-call sequence, arguments, blocking, signals, descriptors, and error values;
- `ltrace` where library-call behavior is relevant;
- GDB, `gdbserver`, core dumps, `coredumpctl`, `addr2line`, and debug-symbol deployment;
- `/proc/PID`, `/sys`, `dmesg`, journald, service status, and kernel tracepoints;
- `readelf`, `objdump`, `nm`, `file`, and dynamic-loader diagnostics for binary and ABI failures;
- `lsof`, `fuser`, `ss`, `ip`, `ethtool`, `mount`, `findmnt`, `df`, `du`, and `dmesg` for resource and integration evidence;
- `ps`, `top`, `vmstat`, `iostat`, `pidstat`, `time`, and `/proc/pressure` for runtime behavior;
- `perf`, eBPF-based tools, and flame graphs as advanced performance topics when target support permits;
- AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer, Valgrind, static analysis, and fuzzing on the host;
- logic analyzers, oscilloscopes, serial captures, and hardware traces when userspace timing depends on physical behavior.

#### Performance and resource reasoning

- CPU time, wall time, latency, throughput, jitter, queue depth, and tail latency;
- memory footprint, stack size, allocations, fragmentation, file-descriptor count, and thread count;
- blocking I/O, context switches, copies, cache behavior, syscalls, and event-loop wakeups;
- buffer sizing, batching, zero-copy tradeoffs, and backpressure;
- `RLIMIT_*`, cgroup v2 CPU/memory/I/O/PIDs controls, OOM behavior, and service-level limits;
- CPU affinity, scheduling policy, priority inversion, and real-time constraints;
- measure on the target with a representative workload before optimizing;
- preserve observability while reducing production overhead.

### Stage 15: Cross-Compilation, Packaging, And Target Integration

- target triples, architecture, ABI, endianness, floating-point ABI, and CPU feature selection;
- sysroots, headers, libraries, pkg-config files, and the dynamic loader path;
- native tools versus target tools and avoiding accidental host-library linkage;
- compiler, linker, loader, and libc compatibility;
- debug versus production binaries, separate debug symbols, build IDs, and crash-symbol lookup;
- installation prefixes, library paths, configuration paths, service files, udev rules, tmpfiles rules, and permissions;
- package metadata, dependencies, alternatives, triggers, post-install behavior, and offline installation;
- Yocto recipe integration for applications, libraries, services, configuration, users/groups, and package splitting;
- image composition, package selection, read-only-rootfs constraints, and SDK workflow;
- deploying the exact kernel, DTB, rootfs, firmware, application, and symbols that belong together;
- version reporting, release manifests, ABI compatibility, and rollback-aware application behavior;
- reproducible builds, source and binary provenance, licenses, SBOMs, and CVE evidence as a boundary with Build Systems and Productization.

### Stage 16: Testing And Verification

#### Host-side testing

- unit tests for parsing, state machines, protocol framing, timeout logic, and error handling;
- dependency injection for files, clocks, sockets, device access, and command execution;
- `pipe`, `socketpair`, Unix sockets, PTYs, `memfd`, temporary filesystems, and loopback interfaces as test fixtures;
- fake devices and simulators that reproduce partial I/O, delays, malformed data, disconnects, and missing files;
- deterministic clocks, bounded random data, reproducible seeds, and controlled scheduler stress;
- sanitizers, static analysis, coverage, fuzzing, and reference-model testing;
- testing the binary with the same compiler warnings, hardening, and linker settings used for release.

#### Target and integration testing

- test the real rootfs, libc, kernel, Device Tree, permissions, init system, and hardware combination;
- boot smoke tests, service-start tests, device-discovery tests, and userspace-to-driver ABI tests;
- peripheral tests for normal values, boundary values, unplug/replug, reset, timeout, and bus errors;
- process crash, service restart, watchdog, network loss, storage full, clock change, and power-cycle tests;
- update, rollback, factory reset, configuration migration, and interrupted-update tests;
- hardware-in-the-loop fixtures, relay-controlled power cycling, serial capture, and artifact linking;
- record target identity, software versions, kernel logs, service logs, test results, and failure artifacts;
- distinguish a test that proves code behavior from a test that proves board integration.

### Stage 17: Design And Architecture Patterns

- single-purpose command-line tool versus long-running daemon;
- synchronous request/response versus event-driven service;
- one process with threads versus multiple processes with failure containment;
- polling versus interrupt-driven kernel ABI versus notifications;
- state machine design for hardware initialization, operation, recovery, and shutdown;
- explicit ownership of devices, descriptors, threads, timers, queues, and persistent state;
- bounded queues, backpressure, retry budgets, circuit breakers, and degraded operation;
- startup readiness versus mere process existence;
- restart-safe initialization and idempotent cleanup;
- stable internal and external APIs, compatibility policy, and migration strategy;
- observability requirements as part of the design rather than as a post-failure addition;
- documenting assumptions about timing, memory, privileges, device availability, and power state.

## Practical Capstones

Complete these in order, reusing code and documentation between them.

### Capstone 1: Reliable Linux Utility

Write a C command-line tool that reads configuration, validates arguments, opens files safely, handles partial I/O and signals, emits useful exit statuses, and works on both a host and a cross-compiled target.

Include:

- `openat`-style safe file handling;
- bounded input and explicit encoding rules;
- atomic configuration or output replacement;
- unit tests and sanitizer runs;
- `strace` evidence for normal and failure paths;
- a target package and version-reporting command.

### Capstone 2: Hardware-Facing Device Client

Write a userspace C client for a real or simulated device exposed through a standard Linux interface.

Possible interfaces include GPIO character-device v2, I2C, SPI, serial, input, IIO, hwmon, watchdog, or SocketCAN.

Include:

- discovery and permission diagnostics;
- timeouts, nonblocking operation, and clean shutdown;
- malformed data and disconnect handling;
- a documented boundary between product policy and kernel UAPI;
- host-side fake-device tests and target integration tests.

### Capstone 3: Event-Driven Hardware Service

Build a daemon that combines a device descriptor, a Unix-domain control socket, a timer, a signal path, and a structured log stream in one bounded event loop.

Include:

- a versioned request/response protocol;
- peer authorization;
- backpressure and maximum message sizes;
- graceful restart and device recovery;
- systemd service integration, watchdog behavior, and resource limits;
- tests for timeout, peer death, service restart, and device removal.

### Capstone 4: Embedded Product Integration

Integrate the service into a Yocto-built image with its application package, service unit, service user, configuration, udev rule if needed, tmpfiles rule, logs, diagnostics, and release metadata.

Validate:

- exact kernel/DTB/rootfs/application identity;
- boot ordering and readiness;
- read-only-rootfs behavior;
- permissions and least privilege;
- power loss, reboot, update, rollback, and storage-full behavior;
- automated boot and hardware smoke tests.

## Core Decision Questions

Use these questions during design and review:

- Is this behavior hardware-specific, board-specific, kernel-specific, or product-specific?
- Does it need interrupt, DMA, precise timing, privileged access, power management, or shared ownership?
- Is there already a stable kernel subsystem ABI for the device?
- Should the interface be a stream, a structured command API, an event source, a memory mapping, or a standard network protocol?
- What happens on a short read, short write, `EINTR`, timeout, disconnect, reset, or process restart?
- Who owns every descriptor, thread, timer, buffer, device, lock, and persistent record?
- What is the maximum queue, message, file, memory, and retry size?
- What happens when the service starts before hardware, loses hardware, or starts twice?
- Which failures are recoverable locally, which require a service restart, and which require a system reboot or safe hardware state?
- What ABI and versioning rules let an old client communicate with a new service or kernel?
- Which privileges are truly required, and how are they removed after initialization?
- What evidence will remain after a crash, watchdog reset, power loss, or failed update?
- How can the behavior be tested without the target, and what must still be tested on real hardware?

## Recommended Study Order For This Role

For embedded Linux, kernel-driver, and low-level hardware work, prioritize the material in this order:

1. Stages 0–3: Linux runtime, processes, system calls, descriptors, and files.
2. Stages 4–7: process memory, time/signals, threads, IPC, and event loops.
3. Stage 10: hardware-facing userspace and kernel UAPI.
4. Stage 11: services, init, and systemd.
5. Stages 14–16: diagnostics, cross-compilation, packaging, and testing.
6. Stages 12–13: security and persistent-state reliability.
7. Stages 8–9: serial and networking branches as required by current hardware.
8. Stage 17: architecture patterns and integrated product decisions.

Do not wait until the end to write programs. Each stage should produce a small C utility, test fixture, diagnostic command, or service extension that can run on the target.

## Boundaries With Other Topics

- Use [C Programming](../c/index.md) for language semantics, libc foundations, memory safety, compilation, and C-level testing.
- Use [Linux Kernel Programming](../linux-kernel/index.md) for kernel execution contexts, driver internals, synchronization, memory, DMA, and subsystem implementation.
- Use [Device Tree](../device-tree/index.md) for hardware description, bindings, board wiring, overlays, and validation.
- Use [Embedded Linux](../embedded-linux/index.md) for boot flow, board bring-up, storage media, rootfs composition, and recovery architecture.
- Use [Build Systems](../build-systems/index.md) for compiler/linker mechanics, cross-toolchains, Yocto, Buildroot, images, and reproducible builds.
- Use [Bash Programming](../bash/index.md) for shell language semantics and robust scripting; use this track for the Linux process and descriptor behavior those scripts rely on.
- Use [Networking](../networking/index.md) for Ethernet, MAC/PHY, board networking, and protocol-specific bring-up; use this track for socket-based application behavior.
- Use [Debugging And Diagnostics](../debugging/index.md) for general investigation methods and evidence workflows; use this track for applying them to userspace programs and services.
- Use [Systems And Embedded Architecture](../systems-and-embedded-architecture/index.md) for cross-component partitioning, tradeoffs, and product architecture.
- Use [Embedded Productization](../embedded-productization/index.md) for release engineering, factory provisioning, OTA systems, secure boot, and field operations.

## Related Topics

- [C Programming](../c/index.md)
- [Bash Programming](../bash/index.md)
- [Python Programming](../python/index.md)
- [C++ For Systems And Embedded Linux](../cpp/index.md)
- [Build Systems](../build-systems/index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [Device Tree](../device-tree/index.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Networking](../networking/index.md)
- [Debugging And Diagnostics](../debugging/index.md)
- [Systems And Embedded Architecture](../systems-and-embedded-architecture/index.md)
- [Embedded Productization](../embedded-productization/index.md)
