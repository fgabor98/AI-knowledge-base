---
status: draft
reviewed: false
domain: meta
difficulty: beginner
last_reviewed: null
---

# Topic Map

This file owns the taxonomy. Add and reorganize topics here before expanding them into full pages.

## C Programming

### Orientation

- origins and history
  - BCPL and B
  - Dennis Ritchie and early C
  - Unix and the PDP-11
  - K&R C
  - ANSI C and ISO C
  - C90, C99, C11, C17, and C23
- standards and conformance
  - ISO C standard structure
  - hosted and freestanding implementations
  - conforming programs and implementations
  - undefined, unspecified, and implementation-defined behavior
  - constraint violations and required diagnostics
  - defect reports and corrigenda
  - compiler language modes and extensions
- use cases and environments
  - bare-metal firmware
  - RTOS applications
  - bootloaders
  - device drivers
  - Linux kernel code
  - embedded Linux userspace
  - protocol stacks
  - safety-critical and security-critical systems
  - C interoperability
- hosted and freestanding C
  - runtime assumptions
  - standard-library availability
  - startup without an operating system
  - minimal runtime support
  - portability boundaries
- learning workflow and tooling
  - compiler and debugger setup
  - command-line workflow
  - build and test loop
  - reading diagnostics
  - host-based experimentation
  - target-based experimentation

### Language Fundamentals

- source code and syntax
  - translation units
  - tokens and preprocessing tokens
  - identifiers and keywords
  - literals
  - declarations and definitions
  - statements and expressions
- types, values, and objects
  - integer types
  - floating-point types
  - character types
  - Boolean values
  - void
  - enumerated types
  - sizeof and type widths
  - value ranges and representations
- expressions and operators
  - arithmetic, relational, logical, and bitwise operators
  - assignment, conditional, and comma operators
  - casts
  - precedence and associativity
  - evaluation order and side effects
- control flow
  - if and switch
  - loops
  - break and continue
  - goto and return
  - state-machine control flow
  - loop invariants and termination
- functions
  - declarations and prototypes
  - definitions
  - parameters and return values
  - variadic functions
  - recursion
  - function pointers
  - callbacks
  - reentrancy
  - function contracts
- arrays, strings, and buffers
  - array initialization
  - array-to-pointer conversion
  - multidimensional arrays
  - null-terminated strings
  - binary buffers
  - bounds and capacities
  - flexible array members
- structures, unions, and enumerations
  - structure declaration and initialization
  - nested structures
  - unions
  - tagged unions
  - enumerations
  - structure layout
  - padding and alignment
- declarations and declarators
  - declaration specifiers
  - pointer declarators
  - arrays of pointers
  - pointers to arrays
  - functions returning pointers
  - pointers to functions
  - typedef
  - incomplete and compatible types
- initialization
  - zero and static initialization
  - automatic initialization
  - designated initializers
  - compound literals
  - partial initialization
  - initialization order

### Semantics And Memory

- storage duration, scope, and linkage
- pointer fundamentals
- pointer arithmetic and bounds
- const, volatile, and restrict
- conversions, promotions, and aliasing
- object representation, alignment, and padding
- memory layout and allocation
- undefined behavior
- memory safety and lifetime
- ownership and bounds reasoning
- stack, heap, static, and thread-local storage

### Modular Design And APIs

- translation units and headers
- preprocessor and macros
- APIs and opaque types
- ownership and resource lifetimes
- error handling
- callbacks and function tables
- architecture patterns
- public versus private interfaces
- dependency direction
- driver and hardware abstraction boundaries
- compile-time and link-time configuration

### Standard Library And Ecosystem

- ISO C library overview
- memory and string APIs
- I/O, diagnostics, and errors
- numeric, time, and character APIs
- atomics, threads, and signals
- embedded libc implementations
  - newlib
  - picolibc
  - musl
  - glibc
  - vendor libc implementations
- POSIX and system interfaces
- common embedded libraries
  - vendor HALs
  - CMSIS-style interfaces
  - RTOS APIs
  - networking stacks
  - TLS libraries
  - USB and CAN stacks
  - filesystem libraries
  - serialization libraries
  - test and mocking frameworks

### Compilation, Linking, And ABI

- translation pipeline
- compiler modes, warnings, and optimization
- object files, symbols, and relocations
- static and dynamic linking
- linker scripts and memory layout
- startup, runtime, and main
- cross-compilation and sysroots
- ABI, calling conventions, and FFI
- debug information and binary inspection
- preprocessing, assembly, map files, and disassembly
- ELF, archives, shared libraries, and relocations

### Correctness, Quality, And Security

- coding practices
- portability
- testing strategy
- static analysis
- sanitizers and dynamic analysis
- debugging with GDB
- security
- safety standards and MISRA
- formal methods
- compiler diagnostics and quality gates
- fuzzing, fault injection, and coverage
- requirements and traceability

### Embedded C And Hardware

- freestanding C
- startup, reset, and vector tables
- memory-mapped I/O
- interrupts, exceptions, and faults
- DMA, cache, and memory barriers
- real-time constraints
- peripheral drivers
- RTOS integration
- bootloaders and firmware images
- GPIO, timers, UART, SPI, I2C, CAN, USB, Ethernet, ADC, DAC, flash, and storage
- watchdogs and recovery paths
- deterministic allocation and stack sizing

### Platform-Specific C

- microcontroller platforms
- ARM Cortex-M
- ARM Cortex-A and AArch64
- RISC-V
- x86-64
- embedded Linux
- multicore and heterogeneous systems
- compiler and vendor extensions
- processor ABIs
- MMU, MPU, cache, and privilege boundaries
- SMP, AMP, remote processors, and shared peripherals

### Advanced C

- advanced type system
- C memory model and concurrency
- compiler optimization and undefined behavior
- advanced data structures
- numerical and fixed-point C
- performance and code size
- protocols and serialization
- C library implementation
- C interoperability
- atomics and lock-free structures
- compiler-generated code
- zero-copy and bounded-memory designs

### Professional Practice And Capstones

- code review and maintainability
- product-quality C workflow
- portable C library capstone
- bounded protocol parser capstone
- bare-metal firmware capstone
- RTOS component capstone
- embedded Linux service capstone
- senior-level outcomes
- requirements, design records, coding standards, tests, static analysis, reproducible builds, and release evidence


## Algorithms And Data Models

Algorithmic foundations:

- problem statements
- input and output descriptions
- preconditions and postconditions
- invariants
- edge cases
- algorithm design vs implementation
- data model choice
- abstract data types
- operations over data
- representation independence
- correctness arguments
- example-driven validation

Algorithm design techniques:

- greedy algorithms
- exchange arguments
- dynamic programming
- memoization and tabulation
- amortized analysis
- recurrence relations
- algorithm testing and reference models
- fuzzing and invariant-based validation
- string and protocol parsing algorithms

Control flow and recursion:

- sequence
- branching
- iteration
- loop invariants
- loop termination
- recursion fundamentals
- base cases
- recursive cases
- recursive call stack
- recursion vs iteration
- divide-and-conquer thinking
- tail-recursive shape

Complexity and efficiency:

- runtime cost
- memory cost
- Big-O notation
- constant factors
- best-case, average-case, and worst-case behavior
- input-size growth
- time complexity
- space complexity
- reducing execution time
- reducing memory use
- reducing complexity by changing the algorithm
- reducing complexity by changing the data model

Basic algorithm schemes:

- summation
- counting
- minimum search
- maximum search
- existence and decision checks
- selection
- filtering
- accumulation
- linear search
- sentinel search
- ordered linear search
- binary search
- recursive binary search
- two-pointer and sliding-window patterns
- prefix sums and difference arrays
- monotonic stacks and queues

Searching and backtracking:

- search-space modeling
- exhaustive search
- guided search
- constraint checking
- pruning
- partial solutions
- backtracking
- recursive backtracking
- iterative backtracking with an explicit stack
- N-queens style problems
- failure propagation
- search order and heuristics

Sorting and ordering:

- why ordering changes algorithms
- comparison functions
- stable vs unstable sorting
- in-place vs out-of-place sorting
- insertion sort as a learning algorithm
- selection sort as a learning algorithm
- bubble sort as a learning algorithm
- merge-style thinking
- sorting as preprocessing
- ordered lookup
- maintaining sorted data
- partial ordering and priority

Graph algorithms:

- graph modeling
- vertices and edges
- directed and undirected graphs
- weighted and unweighted graphs
- adjacency lists
- adjacency matrices
- edge lists
- visited-state tracking
- depth-first search
- breadth-first search
- path reconstruction
- unweighted shortest paths
- Dijkstra's algorithm
- Bellman-Ford algorithm
- topological sorting and DAG processing
- strongly connected components
- minimum spanning trees
- disjoint-set union
- A* and goal-directed search

Tree algorithms:

- tree modeling
- root, parent, child, leaf, and subtree concepts
- binary trees
- tree representation
- expression trees
- preorder traversal
- inorder traversal
- postorder traversal
- level-order traversal
- prefix, infix, and postfix notation
- postfix conversion
- stack-based expression evaluation
- binary search tree operations
- balanced-tree tradeoffs
- tries and prefix trees
- lowest common ancestor
- tree aggregation and dynamic programming

Parallel and dataflow algorithms:

- data channels
- pipeline stages
- input partitioning
- fan-out and fan-in
- reductions
- associative operations
- parallel minimum and maximum search
- parallel sorting
- work distribution
- synchronization costs
- ordering constraints
- result merging
- work scheduling and load balancing
- work stealing
- atomic and lock-free algorithm patterns
- memory ordering and publication

Data structures for algorithms:

- arrays
- strings and byte buffers
- structs and records
- linked lists
- stacks
- queues
- ring buffers
- hash tables
- heaps and priority queues
- trees
- graphs
- bitsets and bitmaps
- intrusive data structures
- memory pools and fixed-size allocators
- deques
- disjoint-set union

Embedded Linux algorithmic constraints:

- bounded memory
- deterministic behavior
- recursion policy
- stack-depth limits
- cache-aware layout
- DMA-friendly buffers
- interrupt-safe queues
- real-time tradeoffs
- allocation failure behavior
- endianness
- alignment constraints
- data integrity checks
- checksums and CRCs
- watchdog-aware long-running operations
- corruption recovery and bounded retries
- watchdog-friendly chunked processing
## C++ For Systems And Embedded Linux

Beginner:

- C++ compilation model
- headers and source files
- declarations, definitions, and One Definition Rule
- namespaces
- name mangling and ABI basics
- object lifetime
- constructors and destructors
- references and value categories
- RAII and deterministic cleanup
- basic error handling without losing context
- standard library overview
- C interop with `extern "C"`

Intermediate:

- move semantics
- copy elision and ownership transfer
- smart pointers
- custom deleters for C handles
- STL containers and allocation behavior
- strings, spans, views, and lifetime
- exceptions policy
- RTTI policy
- templates and compile-time cost
- `constexpr` and compile-time computation
- wrapping C APIs safely
- cross-compiling C++ applications

Advanced:

- embedded allocation strategies
- no-heap and bounded-heap design
- polymorphism without uncontrolled allocation
- type erasure tradeoffs
- template metaprogramming boundaries
- `consteval`, concepts, and type traits
- C++ concurrency primitives
- lock-free data structure cautions
- logging and diagnostics in services
- ABI-stable plugin or shared-library boundaries
- C++ testing frameworks
- performance and code-size profiling

Embedded Linux focus:

- C++ daemon design
- RAII wrappers for file descriptors, sockets, GPIO, serial ports, and IPC handles
- POSIX and Linux API integration
- systemd integration
- CLI support tools
- hardware-facing userspace services
- cross-build and sysroot integration
- exceptions and RTTI policy for embedded products
- allocation visibility
- host and target tests

## Bash Programming

Beginner:

- when to use Bash
- shell background and interpretation model
- interactive shell usage
- startup files and login vs non-login shells
- history, Readline, and prompt basics
- aliases vs functions vs scripts
- terminal job control basics
- shell execution model
- command lookup
- shell builtins
- `type`, `command`, `builtin`, `hash`, and `enable`
- shell variables vs environment variables
- quoting
- variables and expansion
- parameter expansion
- basic command substitution
- arithmetic expansion
- arrays
- word splitting and `IFS`
- filename expansion and globbing
- shell options with `shopt`
- `nullglob`, `failglob`, `globstar`, `extglob`, and `dotglob`
- exit codes
- conditionals
- `case`
- loops
- loop control
- functions
- robust script structure
- Bash vs POSIX shell

Intermediate:

- pipes
- redirection
- file descriptors
- here documents and here strings
- process substitution
- command and process substitution patterns
- subshells
- reading lines safely
- `read` builtin options and status behavior
- NUL-delimited filename handling
- standard Unix tools from Bash
- `find`, `xargs`, `grep`, `sed`, `awk`, `sort`, `cut`, `tee`
- traps and cleanup
- signals
- retries and timeouts
- temporary files
- safe filesystem operations
- locking patterns
- shellcheck-driven cleanup
- logging from scripts
- argument parsing with `getopts`
- manual long-option parsing
- usage messages
- stdout vs stderr logging

Advanced:

- `errexit`, `ERR` traps, and `errtrace`
- advanced file descriptor handling
- `exec` redirection and descriptor lifetime
- `BASH_XTRACEFD`
- signals, process groups, and child processes
- background jobs and `wait`
- bounded parallelism
- advanced parameter expansion
- indirect expansion and namerefs
- associative arrays
- `eval`, injection risks, and shell security
- advanced debugging and tracing
- `PS4`, `BASH_SOURCE`, `LINENO`, and `FUNCNAME`
- `trap DEBUG`
- programmable completion
- Bash testing
- Bats test structure
- portability matrix across Bash versions and userlands
- GNU vs BSD/macOS vs BusyBox command differences
- Bash performance boundaries
- ShellCheck configuration and targeted suppressions

## Python Programming

- interpreter and runtime model
- types and objects
- virtual environments
- dependency management
- packaging basics
- pathlib
- argparse
- subprocess
- logging
- exceptions
- context managers
- dataclasses
- file parsing
- JSON and YAML
- regular expressions
- testing with pytest
- type hints
- static analysis
- automation scripts
- CLI tools
- interacting with Linux commands

## Build Systems

- build systems for embedded Linux roadmap
- Linux kernel build system
- U-Boot build system
- Yocto and OpenEmbedded
- TI Processor SDK Linux
- direct compiler invocation
- object files and linking
- preprocessing, compilation, assembly, and linking pipeline
- static libraries
- shared libraries
- symbol visibility
- ABI compatibility
- linker search paths
- runtime dynamic linker paths
- RPATH and RUNPATH
- debug symbols and build IDs
- Make basics
- recursive Make
- non-recursive Make
- generated dependency files
- parallel builds
- CMake basics
- modern CMake targets
- CMake presets
- CMake toolchain files
- CMake `find_package`
- CMake imported targets
- CMake exported targets
- CMake install rules
- CPack basics
- Ninja
- Meson
- Autotools
- Bazel-like hermetic build concepts
- remote build cache concepts
- pkg-config
- target `pkg-config`
- cross-compilation
- target triples
- sysroots
- staging directories
- toolchain files
- toolchain version pinning
- SDK sysroots
- generated SDKs
- kernel module builds
- Kbuild
- Kconfig
- defconfig
- U-Boot SPL and TPL
- FIT images
- dependency tracking
- source fetching
- source mirrors
- dependency vendoring
- Git submodules
- patch management
- quilt patch workflow
- generated code dependencies
- host tools in cross-builds
- code generators
- reproducible builds
- hermetic builds
- deterministic archives
- timestamp control
- build path reproducibility
- offline builds
- build containers
- build environment isolation
- ccache
- sstate cache
- download caches
- artifact caches
- build performance profiling
- incremental build correctness
- CI build checks
- static analysis integration
- compiler warnings policy
- unit test integration
- cross-test strategy
- emulator-based tests
- sanitizer integration
- coverage integration
- image size regression checks
- release artifact generation
- build provenance
- release manifests
- SBOM generation
- license manifest generation
- source archive generation
- debug symbol packages
- binary package feeds
- package signing
- artifact promotion
- artifact retention
- Yocto recipes
- Yocto layers
- Yocto images
- BitBake tasks
- BitBake task signatures
- BitBake fetchers
- Yocto SDK generation
- OpenEmbedded metadata
- TI Arago layers
- TI oe-layersetup
- TI Processor SDK image targets
- Buildroot packages
- Buildroot external trees
- Buildroot rootfs overlays
- Buildroot post-build scripts
- Buildroot post-image scripts
- PTXdist
- OpenWrt build system
- vendor BSP build flows
- root filesystem image tools
- RAUC
- SWUpdate
- Mender
- OSTree

## Linux Userspace And System Programming

### Environment And Mental Model

- hosted C process, libc, dynamic linker, system call, kernel subsystem, driver, and hardware
- user mode versus kernel mode
- POSIX interfaces versus Linux extensions versus vendor and product APIs
- manual-page sections, feature-test macros, `errno`, cancellation, and thread-safety contracts
- host/target differences in architecture, ABI, libc, rootfs, kernel, and init system
- repeatable host/target labs, serial access, symbols, logs, and `strace`

### Linux Runtime, Filesystem, And Rootfs

- filesystem hierarchy and path resolution
- `/etc`, `/proc`, `/sys`, `/dev`, `/run`, `/var`, `/tmp`, `/usr`, and `/lib`
- files, directories, inodes, metadata, symlinks, hard links, and mount points
- `stat`, `lstat`, `fstat`, `readlink`, `realpath`, directory iteration, and directory FDs
- `openat`, `fstatat`, `renameat`, `unlinkat`, safe temporary files, and TOCTOU resistance
- `procfs`, `sysfs`, `devtmpfs`, `tmpfs`, `debugfs`, and `configfs`
- mounts, `fstab`, initramfs, bind mounts, mount namespaces, overlayfs, and read-only rootfs
- volatile state, persistent data, configuration, caches, logs, sockets, and locks
- executable interpreters, ELF program headers, dynamic loader, shared-library lookup, and ABI
- static versus dynamic linking and target deployment

### Processes And Program Lifetime

- processes, programs, threads, tasks, PIDs, TIDs, process groups, sessions, and process trees
- `fork`, `vfork`, `clone`, copy-on-write, `execve`, `execveat`, and `posix_spawn`
- argument and environment vectors, current directory, umask, credentials, and inherited FDs
- `exit`, `_exit`, `atexit`, `waitpid`, `waitid`, `SIGCHLD`, exit status, zombies, and orphans
- reparenting, subreapers, PID 1, supervision, crash loops, backoff, and escalation
- controlling terminals and job control
- `/proc/PID` observation and process states
- `ps`, `top`, `pgrep`, `pkill`, `pidof`, `kill`, `nice`, `renice`, and `chrt`

### System Calls, Files, And File Descriptors

- libc wrappers versus direct system calls
- return values, `errno`, `EINTR`, `EAGAIN`, partial I/O, retries, and cancellation
- `open`, `openat`, `read`, `write`, `pread`, `pwrite`, `lseek`, `truncate`, and `close`
- file descriptors, open-file descriptions, offsets, status flags, descriptor flags, and ownership
- `dup`, `dup2`, `dup3`, `fcntl`, `FD_CLOEXEC`, `O_CLOEXEC`, and `O_NONBLOCK`
- stdio versus descriptor I/O
- file creation, metadata, `rename`, `unlink`, advisory locks, and race-resistant operations
- `fsync`, `fdatasync`, directory sync, disk-full behavior, and durability
- anonymous pipes, FIFOs, `pipe2`, EOF, `SIGPIPE`, `EPIPE`, buffering, and backpressure
- optional `sendfile`, `splice`, `copy_file_range`, direct I/O, `close_range`, and `pidfd`

### Process Memory And Mapping

- virtual address spaces, pages, protections, and user/kernel separation
- text, data, BSS, heap, stack, TLS, shared libraries, and mapped files
- `mmap`, `munmap`, `mprotect`, anonymous mappings, file-backed mappings, and shared memory
- copy-on-write, page faults, demand paging, guard pages, and stack limits
- ASLR, PIE, NX, RELRO, and process memory hardening
- RSS, virtual size, dirty pages, memory pressure, overcommit, and OOM
- `mlock` and real-time memory constraints
- documented device mappings versus unsafe physical-address access

### Time And Signals

- wall-clock, monotonic, boottime, raw, and real-time clocks
- `clock_gettime`, `clock_nanosleep`, `nanosleep`, POSIX timers, and `timerfd`
- absolute deadlines, drift, suspend behavior, RTC, NTP, PTP, and timeout design
- signal disposition, masks, pending signals, `sigaction`, and `SA_RESTART`
- `SIGTERM`, `SIGINT`, `SIGHUP`, `SIGCHLD`, `SIGPIPE`, and fatal signals
- async-signal-safe functions, self-pipe, `signalfd`, reload, and graceful shutdown

### Threads And Userspace Concurrency

- POSIX thread creation, joining, detaching, naming, attributes, stacks, and TLS
- mutexes, robust and recursive mutexes, condition variables, read/write locks, barriers, and semaphores
- predicates, spurious wakeups, lost wakeups, data races, deadlocks, and lock ordering
- C atomics versus POSIX synchronization
- cancellation, cleanup handlers, reentrancy, thread-safe libraries, and thread-local `errno`
- ownership and lifetime across worker threads
- bounded queues, producer/consumer designs, backpressure, and shutdown markers
- priority inversion, priority inheritance, CPU affinity, and real-time scheduling
- futexes as a conceptual kernel primitive

### IPC And Event-Driven Design

- pipes, FIFOs, Unix-domain sockets, datagrams, stream protocols, and `socketpair`
- shared memory, `shm_open`, `memfd_create`, and synchronization
- POSIX message queues, System V IPC, D-Bus, and maintenance tradeoffs
- `eventfd`, `timerfd`, `signalfd`, and netlink
- filesystem notifications with `inotify` and the limits of change-notification APIs
- message framing, size limits, byte order, alignment, serialization, and versioning
- request/reply, events, cancellation, duplicate requests, retries, and peer death
- peer credentials such as `SO_PEERCRED`
- queue bounds, backpressure, overload, reconnection, and failure containment
- `select`, `poll`, `ppoll`, `pselect`, and `epoll`
- level-triggered versus edge-triggered readiness, one-shot events, fairness, and close races
- optional `io_uring` with embedded-kernel support considerations

### Terminals, TTYs, And Serial Userspace

- terminals, TTYs, line disciplines, controlling terminals, and PTYs
- `termios`, canonical/raw mode, echo, signal generation, and input buffering
- baud, parity, stop bits, flow control, modem-control lines, `VMIN`, and `VTIME`
- serial framing, checksums, resynchronization, partial messages, and timeouts
- console UART versus application serial ports
- PTY-based testing and `stty`, `setserial`, `screen`, and `minicom`

### Userspace Networking

- socket domains, types, protocols, addresses, and byte order
- `getaddrinfo`, `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, and shutdown
- TCP streams, partial I/O, reset, half-close, keepalive, reconnect, and deadlines
- UDP boundaries, loss, duplication, ordering, broadcast, and multicast
- Unix sockets versus loopback TCP
- IPv4, IPv6, dual-stack behavior, interface binding, and resolver dependencies
- nonblocking sockets, socket options, ancillary data, and FD passing
- netlink overview, TLS integration, certificate verification, and offline behavior
- `ss`, `ip`, `ethtool`, `tcpdump`, and `/proc/net`

### Hardware-Facing Userspace And Kernel UAPI

- kernel driver versus subsystem ABI versus userspace helper versus firmware
- latency, interrupts, DMA, power, security, sharing, ownership, and recovery at the boundary
- `/dev`, major/minor numbers, character/block devices, `devtmpfs`, uevents, udev, and persistent names
- sysfs topology, attributes, modaliases, hotplug races, and deferred availability
- `read`/`write`, sysfs, `ioctl`, `poll`, `mmap`, vectored I/O, and event records
- blocking, nonblocking, timeout, cancellation, and close semantics
- pointer-free UAPI structures, fixed-width types, padding, reserved fields, endianness, and 32/64-bit compatibility
- ABI versioning, feature discovery, sequence numbers, timestamps, overflow, and lost-event recovery
- GPIO chardev v2, `i2c-dev`, `spidev`, serial/TTY, evdev, IIO, hwmon, RTC, SocketCAN, watchdog, LEDs, PWM, and power controls
- V4L2/media, ALSA, DRM, USB, PCIe, UIO, VFIO, remoteproc, and RPMsg as optional branches
- device discovery, permissions, standard APIs, and product limits of raw bus access

### Services, Init, And systemd

- PID 1, early userspace, BusyBox init, init scripts, and systemd
- unit types, targets, dependencies, ordering, conditions, conflicts, and device units
- `ExecStart`, `ExecStop`, `Environment`, `WorkingDirectory`, `User`, `Group`, and runtime directories
- `Type=simple`, `exec`, `forking`, `notify`, and `oneshot`
- restart policies, start limits, backoff, kill behavior, and graceful shutdown
- socket, timer, path, mount, and device activation
- journald, structured logs, rate limits, persistent logs, and privacy
- `systemctl`, `journalctl`, `systemd-analyze`, `sd_notify`, readiness, and watchdogs
- reload protocols, validation, idempotent startup, and update/restart behavior
- service resource limits, cgroups, capabilities, and filesystem sandboxing

### Identity, Privilege, And Userspace Security

- real, effective, saved, and filesystem IDs
- supplementary groups, permissions, umask, ACLs, and access checks
- capabilities and capability sets
- privilege dropping, `setuid`, `setgid`, `no_new_privs`, securebits, and set-user-ID risks
- Unix-socket peer authorization
- mount, PID, network, user, IPC, and UTS namespaces
- cgroups, seccomp, SELinux, AppArmor, and LSM concepts
- read-only system files, writable-data boundaries, service sandboxing, and least privilege
- safe device, sysfs, firmware, key, certificate, calibration, and update-artifact handling
- path traversal, symlink/TOCTOU attacks, unsafe temporary files, environment attacks, command injection, and parser bugs
- PIE, RELRO, stack protection, fortify, and production security evidence

### Persistent State, Storage, And Power Loss

- immutable files, configuration, runtime state, user data, caches, logs, and crash artifacts
- volatile versus persistent storage decisions
- atomic updates, `fsync`, rename, directory sync, recovery, and schema migration
- locks, PID files, Unix socket paths, stale state, and single-instance ownership
- ext4, squashfs, UBI/UBIFS, eMMC/NAND, flash wear, and write amplification
- log rotation, bounded logging, full/read-only/corrupt storage, and brownout behavior
- A/B updates, rollback, factory reset, and versioned state
- retaining diagnostic evidence without exhausting the recovery path

### Diagnostics, Performance, And Resource Limits

- failure classification, first-failure evidence, reproduction, reduction, and hypothesis-driven debugging
- startup, permission, dependency, device, protocol, hang, crash, corruption, and resource failures
- `strace`, `ltrace`, GDB, `gdbserver`, core dumps, `coredumpctl`, and symbols
- `/proc/PID`, `/sys`, `dmesg`, journald, service status, and kernel tracepoints
- `readelf`, `objdump`, `nm`, `file`, and loader diagnostics
- `lsof`, `fuser`, `mount`, `findmnt`, `df`, `du`, `ps`, `top`, `vmstat`, `iostat`, `pidstat`, and `/proc/pressure`
- `perf`, eBPF tools, flame graphs, sanitizers, Valgrind, static analysis, and fuzzing
- CPU, memory, stack, descriptor, thread, queue, syscall, copy, latency, throughput, jitter, and tail behavior
- `RLIMIT_*`, cgroup v2 CPU/memory/I/O/PID controls, OOM behavior, and target measurement
- serial captures, logic analyzers, oscilloscopes, and hardware traces

### Cross-Compilation, Packaging, And Target Integration

- target triples, ABI, endianness, floating-point ABI, CPU features, and sysroots
- native versus target tools, pkg-config, dynamic loader paths, and accidental host linkage
- debug symbols, build IDs, hardening, package layout, and runtime dependencies
- configuration paths, service files, udev rules, tmpfiles rules, users/groups, and permissions
- offline packages, post-install behavior, image composition, SDKs, and read-only-rootfs constraints
- Yocto application recipes, service integration, package splitting, and target tests
- exact kernel/DTB/rootfs/firmware/application identity
- version reporting, ABI compatibility, rollback behavior, provenance, licenses, SBOMs, and CVE evidence

### Testing And Architecture

- host unit, integration, sanitizer, coverage, fuzz, and reference-model testing
- dependency injection for files, clocks, sockets, devices, and commands
- pipes, socketpairs, PTYs, memfd, temporary filesystems, loopback, fake devices, and deterministic clocks
- target boot, service, device-discovery, peripheral, reset, watchdog, network-loss, storage, and power-cycle tests
- update, rollback, factory-reset, migration, and interrupted-update tests
- hardware-in-the-loop, board farms, serial capture, and artifact linking
- command-line utility versus daemon
- synchronous versus event-driven design
- threads versus process isolation
- polling versus notifications
- state machines, bounded queues, retry budgets, circuit breakers, readiness, and failure containment
- ownership, lifetime, compatibility, observability, and design assumptions

## Debugging And Diagnostics

Beginner:

- failure classification
- reproducing failures
- reducing failures
- first-failure evidence
- compiler diagnostics
- logging strategy
- terminal and command-line evidence
- GDB basics
- core dumps
- `strace`
- serial console workflow
- lab notes

Intermediate:

- binary search and bisecting
- hypothesis-driven debugging
- timeout and hang classification
- race-condition workflow
- `ltrace`
- `perf`
- ftrace overview
- dynamic debug overview
- `tcpdump`
- logic analyzer workflow
- hardware-in-the-loop testing
- boot log parsing

Advanced:

- crash dump workflow
- persistent logs
- support bundle design
- production diagnostics
- automated flashing tests
- board farm workflow
- field failure triage
- postmortems
- observability requirements
- remote diagnostics
- reproducer automation
- debug-data privacy and security

## Networking

- Ethernet basics
- IPv4
- IPv6
- routing
- DNS
- DHCP
- sockets
- TCP
- UDP
- netlink basics
- Linux network interfaces
- firewall basics
- tcpdump and packet inspection
- embedded network bring-up
- MAC vs PHY
- MDIO
- PHY addresses
- link negotiation
- RGMII/RMII interface modes
- PHY reset GPIOs
- PHY interrupt lines
- fixed-link
- device tree networking nodes
- U-Boot Ethernet vs Linux Ethernet
- static IP
- routes
- systemd-networkd
- NetworkManager tradeoffs
- interface naming
- ethtool
- ARP checks
- link-state checks
- VLANs
- bridges
- nftables overview
- NTP
- PTP overview
- service discovery overview

## Systems And Embedded Architecture

Foundations:

- CPU architecture
- memory hierarchy
- caches
- MMU and virtual memory
- interrupts
- DMA
- buses
- clocks, resets, regulators, and power rails
- boot chain
- firmware, kernel, and userspace responsibility split
- latency, throughput, and jitter
- observability and diagnostics

Embedded Linux architecture:

- bootloader-to-kernel handoff
- Device Tree as the hardware contract
- kernel driver vs userspace service decisions
- service architecture
- IPC architecture
- logging and persistent evidence
- read-only root filesystem architecture
- persistent state architecture
- update and rollback architecture
- recovery and rescue architecture
- watchdog strategy
- hardware and software partitioning

Advanced system design:

- real-time constraints
- fault containment
- safety failure modes
- secure boot and trust boundaries
- heterogeneous SoC partitioning
- remote firmware partitioning
- power management architecture
- networked device architecture
- manufacturing and provisioning architecture
- field diagnostics and support bundles
- compatibility contracts
- release architecture
- tradeoff analysis

## Embedded Linux

- boot chain
- BootROM
- SPL
- U-Boot
- SoC boot flow
- U-Boot build system
- U-Boot board defconfigs
- U-Boot SPL and TPL
- U-Boot FIT images
- Linux handoff
- kernel command line
- device tree overlays
- root filesystem layout
- image layout and storage
- SD/eMMC/NAND/NOR storage
- WIC images
- UBI and UBIFS
- init systems
- systemd basics
- BusyBox systems
- Yocto basics
- Yocto and OpenEmbedded
- BitBake
- Yocto layers
- Yocto recipes
- Yocto images
- Yocto machine configuration
- Yocto distro configuration
- TI Processor SDK Linux
- TI Arago layers
- TI oe-layersetup
- TI Processor SDK image targets
- TI Sitara platform notes
- AM62x
- AM64x
- AM335x
- PRU basics
- remoteproc and rpmsg
- TI CPSW Ethernet
- TI pinmux workflow
- TI TRM reading workflow
- Buildroot basics
- PTXdist basics
- OpenWrt build system basics
- vendor BSP build flows
- advanced build systems roadmap
- BSP build integration
- Linux kernel advanced build roadmap
- U-Boot advanced build roadmap
- Yocto/OpenEmbedded advanced build roadmap
- TI Processor SDK Linux advanced build roadmap
- kernel source tree and outputs
- kernel Kconfig and defconfig workflow
- Kbuild object selection
- kernel external module builds
- kernel device tree build targets
- U-Boot source tree and outputs
- U-Boot board defconfigs
- U-Boot generated config
- U-Boot SPL/TPL build flow
- U-Boot FIT image workflow
- Yocto build directory and configuration
- Yocto task workdirs
- Yocto images and packagegroups
- Yocto machine and distro configuration
- Yocto kernel and bootloader integration
- Yocto devtool workflow
- TI Processor SDK release model
- TI Processor SDK installed layout
- TI Arago layer setup
- TI deploy-ti artifacts
- TI SDK kernel customization
- TI SDK U-Boot customization
- TI SDK device tree customization
- TI SDK EVM to product board workflow
- root filesystem image tools
- update artifact systems
- cross-compilation
- toolchains
- kernel configuration
- board bring-up
- device tree for board bring-up
- DTS and DTSI layering
- pinmux
- regulators and clocks
- Ethernet PHY device tree debugging
- serial console
- networking bring-up
- storage layout
- partitioning
- A/B update schemes
- secure boot concepts
- verified boot
- field diagnostics
- production logging
- boot media selection
- boot scripts
- `extlinux.conf`
- FIT image handoff
- initramfs
- rootfs mount
- init and systemd startup
- serial log failure classification
- vendor EVM baseline
- source-built board baseline
- custom board delta list
- minimal rootfs boot
- peripheral bring-up
- strace
- ltrace
- gdbserver
- core dumps
- boot hangs
- kernel panics
- watchdog resets
- systemd unit files
- service dependencies
- systemd targets
- restart policies
- journald
- tmpfiles
- systemd timers
- read-only rootfs integration
- raw NAND
- NOR/QSPI/OSPI
- ext4
- squashfs
- overlayfs
- UUID and PARTUUID
- power-loss behavior
- rescue shell
- recovery boot path
- provisioning runtime flow
- GPIO
- I2C
- SPI
- UART
- CAN
- USB
- PCIe
- logic analyzer workflow

## Device Tree

- Foundations
  - what Device Tree solves
  - Device Tree as a hardware description rather than driver configuration
  - Device Tree bindings as a stable ABI across bootloaders, kernels, and operating systems
  - Device Tree vs ACPI and when each hardware-description model applies
  - DTS
  - DTSI
  - DTB
  - DTBO
  - overlays
  - flattened Device Tree structure
  - DTB header, memory reservation block, structure block, and strings block
  - source include structure and inheritance
  - labels
  - phandles
  - node names
  - unit addresses
  - properties
  - paths and aliases
  - comments and style
- Syntax, Values, And Source Composition
  - 32-bit cells and cell arrays
  - strings and string lists
  - byte arrays
  - empty and boolean properties
  - 64-bit values represented by multiple cells
  - property and node references
  - label references and path references
  - `/bits/`
  - `/delete-node/`
  - `/delete-property/`
  - `/include/` directives
  - C preprocessor includes and macros
  - DTS includes vs C preprocessor includes
  - overriding and extending nodes from included DTSI files
  - board, SoC, and shared-family source layering
  - source formatting and Linux DTS coding style
- Provider–Consumer Relationships
  - phandles with argument cells
  - zero-cell vs multi-cell providers
  - `#clock-cells`
  - `#reset-cells`
  - `#gpio-cells`
  - `#interrupt-cells`
  - other provider-specific `#*-cells` properties
  - consumer properties and `*-names` properties
  - mapping a consumer property to its provider binding
  - decoding specifier cells using the provider binding
  - provider–consumer relationships for clocks, resets, GPIOs, interrupts, DMA, IOMMUs, PHYs, power domains, and regulators
- Standard Nodes And Properties
  - root-node `compatible`
  - root-node `model`
  - `status`
  - `/aliases`
  - `/cpus`
  - CPU topology
  - `/memory`
  - `/chosen`
  - `stdout-path`
  - boot arguments
  - `/reserved-memory`
  - `/memreserve/`
  - `interrupts-extended`
  - `interrupt-map`
  - `interrupt-map-mask`
  - `dma-coherent`
  - `iommus`
  - `phys`
  - `phy-names`
- Addressing And Bus Modeling
  - `reg`
  - `ranges`
  - `#address-cells`
  - `#size-cells`
  - `interrupt-parent`
  - `interrupts`
  - `dma-ranges`
  - simple-bus
  - bus-specific child addressing
  - address translation across nested buses
  - PCI host bridge address mapping
  - PCI interrupt mapping
- Driver Matching
  - `compatible`
  - fallback compatible strings
  - board-compatible vs SoC-compatible fallback chains
  - backward compatibility and when to introduce a new `compatible`
  - `of_match_table`
  - platform devices
  - modalias
  - binding-driven driver expectations
  - optional vs required properties
- Pinctrl, GPIOs, And Interrupts
  - pinmux
  - pin configuration
  - GPIO controllers
  - GPIO consumers
  - active-high vs active-low
  - interrupt controllers
  - interrupt trigger types
  - reset GPIOs
- Clocks, Resets, Regulators, And Power
  - clock providers
  - clock consumers
  - reset controllers
  - fixed regulators
  - PMIC regulators
  - regulator constraints
  - power domains
  - wake sources
  - runtime PM dependencies
  - operating-points-v2 tables
  - CPU frequency relationships
  - thermal zones
  - cooling devices
- Common Peripheral Nodes
  - UART
  - I2C
  - SPI
  - CAN
  - USB
  - PCIe
  - MMC
  - SD
  - eMMC
  - Ethernet MAC
  - Ethernet PHY
  - MDIO
  - fixed-link
  - LEDs
  - keys and buttons
  - watchdogs
  - RTCs
  - hardware monitors
  - NVMEM providers and consumers
  - MTD devices and fixed partitions
- Graph Bindings And Complex Data Paths
  - `ports`
  - `port`
  - `endpoint`
  - local and remote endpoints
  - display pipelines
  - camera pipelines
  - audio routing
  - graph validation and endpoint consistency
- Memory, Firmware, And Heterogeneous SoCs
  - CMA
  - firmware nodes
  - IOMMU topology
  - DMA coherency
  - DMA address translation
  - remoteproc
  - RPMsg
  - PRU
  - R5/M4 cores
  - shared memory
  - trusted firmware
  - OP-TEE
  - secure-world reserved memory
- U-Boot And Bootloader Device Tree
  - U-Boot control DTB
  - SPL DTB
  - Linux DTB
  - U-Boot-specific properties
  - pre-relocation properties
  - overlays applied by U-Boot
  - FIT image DTB selection
  - environment-driven DTB loading
- Boot-Time Mutation And Ownership
  - bootloader fixups
  - firmware fixups
  - memory-size updates
  - MAC-address injection
  - serial-number injection
  - `/chosen` modifications
  - overlay application order
  - DTB relocation and available padding
  - ownership of each boot-time mutation
  - built DTB vs bootloader-visible tree vs Linux runtime tree
  - tracing the exact DTB and overlays selected during boot
- Binding Design And Stable ABI
  - describing hardware rather than Linux implementation details
  - avoiding nodes created only to instantiate drivers
  - complete hardware descriptions despite incomplete driver support
  - binding backward compatibility
  - compatible-string versioning
  - property naming and standard unit suffixes
  - standard property reuse
  - avoiding policy in Device Tree
  - board and product revision strategies
  - Devicetree ABI versioning across product revisions
  - binding review expectations
  - upstream binding submission workflow
  - submitting bindings before DTS users
- Writing And Validating Binding Schemas
  - YAML bindings
  - `dt-bindings`
  - `$id`
  - `$schema`
  - `maintainers`
  - `description`
  - `select`
  - `properties`
  - `patternProperties`
  - `required`
  - `$ref`
  - `allOf`
  - `oneOf`
  - conditional schemas
  - `additionalProperties` vs `unevaluatedProperties`
  - child-node schemas
  - property types
  - array cardinality
  - binding examples
  - vendor bindings
  - `dt_binding_check`
  - targeted validation with `DT_SCHEMA_FILES`
  - `dtc` warnings
  - `dtbs_check`
  - why invalid schemas can cause `dtbs_check` to skip checks
  - schema errors
  - undocumented properties
- Overlays In Depth
  - `/plugin/`
  - fragments and targets
  - label targets vs path targets
  - `__symbols__`
  - `__fixups__`
  - local fixups
  - base DTB symbol requirements
  - compiling overlays with symbols
  - bootloader-applied vs kernel-applied overlays
  - overlay stacking and removal dependencies
  - overlay compatibility across base DTB versions
  - lifetime hazards when dynamically removing overlay nodes
  - limitations of overlays as a board-variant mechanism
- Build And Diagnostic Tools
  - `dtc`
  - `fdtdump`
  - `fdtget`
  - `fdtput`
  - `fdtoverlay`
  - U-Boot `fdt` commands
  - compiler symbols with `-@`
  - compiler warning levels
  - kernel `W=1` and `W=2` Device Tree builds
  - preprocessing a DTS
  - tracing a generated DTB to its source and build rule
  - `libfdt`
  - firmware and bootloader use of `libfdt`
- Runtime Inspection
  - `/proc/device-tree`
  - `/sys/firmware/devicetree/base`
  - decoded DTBs with `dtc`
  - checking deployed DTB identity
  - `dmesg` probe logs
  - driver bind/unbind checks
  - comparing source DTS to runtime tree
  - inspecting NUL-terminated property values safely
  - inspecting binary cells and byte arrays
  - comparing DTB hashes across build, boot media, and target
  - inspecting the tree from U-Boot before kernel handoff
- Security And Production Lifecycle
  - DTB and DTBO integrity
  - FIT signing and authenticated Device Tree selection
  - measured boot and Device Tree
  - malicious or untrusted DTB risks
  - security impact of bootloader fixups and overlays
  - coordinating kernel, DTB, modules, and firmware versions
  - reproducible DTB builds
  - DTB provenance and release manifests
  - field update compatibility
- Board Porting Workflow
  - start from closest EVM
  - board delta list
  - minimal boot DTS
  - console first
  - boot media next
  - regulators and clocks
  - Ethernet
  - storage
  - remoteproc and reserved memory
  - overlays
  - board revision and product variant modeling
  - minimizing board-specific deltas
  - upstreaming bindings and DTS changes
  - validation checklist

## Linux Kernel Programming

- kernel foundations for driver developers
  - kernel mental model
  - kernel space vs user space
  - system calls
  - processes and tasks
  - interrupts
  - drivers as hardware integration
  - kernel C survival guide
  - `container_of`
  - intrusive lists
  - embedded structs
  - function pointers
  - callbacks
  - `ERR_PTR`
  - `IS_ERR`
  - `PTR_ERR`
  - `goto` cleanup style
  - kernel C is not userspace C
  - no libc in kernel code
  - kernel logging APIs
  - kernel memory allocation APIs
  - `__user` pointers
  - userspace copy helpers
  - `__iomem` pointers
  - small kernel stack
  - no floating point in normal kernel code
  - UAPI vs internal kernel API
  - reference ownership preview
  - `kref`
  - `refcount_t`
  - RCU preview
  - per-CPU variable preview
  - kernel taint flags
  - sparse
  - smatch
  - Coccinelle
  - reading kernel source
  - source navigation with `rg` and `git grep`
  - call-chain reading
  - struct-first reading
  - kernel development lab setup
  - VM and QEMU labs
  - spare board labs
  - serial console
  - recovery kernel
  - driver development workflow
  - debugging ladder
  - failure taxonomy
  - execution context primer
  - device model primer
  - small lab progression
  - kernel documentation reading guide for beginners
- kernel source, build, and tailoring
  - kernel source acquisition
  - upstream kernel source
  - vendor kernel source
  - source provenance
  - kernel configuration and tailoring
  - kernel build and install overview
  - external module build prerequisites
  - kernel image, DTB, and module artifacts
- Linux device driver fundamentals
  - kernel module lifecycle
  - built-in drivers vs loadable modules
  - device tree hardware description
  - device tree overlays
  - driver binding, probe, and remove
  - platform devices and platform drivers
  - device tree matching from drivers
  - `of_match_table`
  - `compatible` strings
  - resource lookup
  - `devm_*` managed allocation
  - character devices
  - device classes, uevents, and udev
  - sysfs attributes
  - kobjects and sysfs groups
  - pollable sysfs attributes
  - module parameters
  - driver logging with `dev_*`
  - user-space hardware access vs kernel drivers
- common driver interfaces
  - GPIO consumer API
  - GPIO controller drivers
  - GPIO expanders
  - legacy GPIO interfaces
  - interrupt processing model
  - interrupt domains
  - interrupt controller drivers
  - IRQ handling
  - threaded interrupts
  - I2C client drivers
  - SPI device drivers
  - UART and TTY integration
  - CAN driver integration
  - PWM driver overview
  - regmap
  - clocks
  - resets
  - regulators
  - pinctrl
  - DMA basics
  - IIO subsystem
  - IIO channels and sysfs
  - IIO triggers and buffers
  - input subsystem
  - polled input devices
  - IRQ-based input devices
- kernel execution and concurrency
  - interrupt context vs process context
  - sleepable vs atomic code
  - bottom halves
  - softirqs
  - tasklets
  - locking
  - atomic operations
  - workqueues
  - concurrency managed workqueues
  - timers
  - hrtimers
  - kernel timekeeping
  - time-related APIs
  - wait queues
  - completions
  - lifetime and reference counting
- kernel memory and I/O
  - kernel memory allocation
  - allocation flags
  - `kmalloc`
  - `vmalloc`
  - virtual memory areas
  - MMIO
  - register accessors
  - userspace copy helpers
  - `ioctl` ABI basics
  - DMA mapping basics
  - single-buffer DMA
  - scatter-gather DMA
- kernel configuration and platform policy
  - debug configs vs production configs
  - built-in vs module policy
  - kernel command line policy
  - watchdog-related options
  - module signing
  - kernel hardening options
  - namespaces and cgroups overview
  - LSM overview
  - initramfs-related options
  - config review workflow
- kernel debugging basics
  - `dmesg`
  - log levels
  - dynamic debug
  - ftrace
  - tracepoints
  - perf overview
  - debugfs
  - sysfs inspection
  - KGDB basics
  - oops, panic, and crash logs
  - watchdog reset diagnosis
  - probe failure debugging
- power management
  - runtime PM
  - suspend and resume
  - wake sources
  - cpuidle and cpufreq
  - power domains
  - regulator constraints
  - clock gating
  - device tree power dependencies
  - suspend and resume debugging
- remoteproc, RPMsg, and heterogeneous SoCs
  - remoteproc framework
  - firmware loading
  - reserved memory
  - virtio and RPMsg
  - PRU integration overview
  - R5 and M4 firmware lifecycle
  - remote core logs
  - crash handling
  - device tree nodes for remote cores
- kernel build mechanics are tracked under Build Systems
  - Linux kernel build system
  - Kbuild
  - Kconfig
  - defconfig
  - menuconfig
  - out-of-tree builds with `O=`
  - external modules with `M=`
  - kernel image outputs

## Embedded Productization

- embedded DevOps role
- embedded release engineering
- product versioning
- artifact naming
- release manifests
- build provenance
- reproducible product builds
- source manifests
- SBOM and license artifacts
- debug symbol handling
- SDK release artifacts
- factory image vs OTA image
- hardware-in-the-loop testing
- board farms
- serial console automation
- relay-controlled power cycling
- flashing automation
- network boot testing
- smoke tests on hardware
- peripheral tests
- factory provisioning
- factory flashing stations
- golden images
- serial number provisioning
- MAC address programming
- certificate and key provisioning
- calibration data
- production test logs
- device identity
- OTA update systems
- RAUC
- SWUpdate
- Mender
- OSTree
- A/B partitioning
- rollback logic
- signed update bundles
- interrupted update recovery
- secure boot and chain of trust
- ROM boot trust anchor
- SPL and U-Boot signing
- FIT image signing
- root filesystem integrity
- kernel module signing
- key management
- production diagnostics
- persistent logs
- watchdogs
- boot counters
- health checks
- update status reporting
- remote log retrieval
- read-only root filesystem strategy
- filesystem corruption prevention
- brownout and power-loss behavior
- runtime recovery workflow
- rescue shell policy
- compatibility metadata
- bootloader/user-space update contract
- filesystem permissions
- Linux capabilities
- users and groups
- SSH hardening
- secrets handling
- TPM overview
- TEE overview
- attack surface reduction
- version reporting on device
- support bundle generation
- remote diagnostics
- field health checks
- artifact provenance on device
- manufacturing and board test
- hardware test images
- EEPROM programming
- fixture-driven tests
- separation of manufacturing and production images
