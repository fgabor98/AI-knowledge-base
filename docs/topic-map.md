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

- compilation model
- translation units
- declarations vs definitions
- storage duration and linkage
- integer types and promotions
- pointers
- arrays vs pointers
- pointer arithmetic
- structs and unions
- alignment and padding
- bit fields
- const correctness
- volatile
- restrict
- undefined behavior
- implementation-defined behavior
- strict aliasing
- memory layout
- stack vs heap
- static vs dynamic allocation
- ownership conventions
- error handling patterns
- bit manipulation
- function pointers
- callbacks
- macro hygiene
- header file design
- build flags and warnings
- debugging with GDB
- sanitizers
- linker basics
- linker scripts
- embedded C constraints

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

Beginner:

- Linux filesystem layout
- `/etc`, `/proc`, `/sys`, `/dev`, `/run`, `/var`, and `/tmp`
- users, groups, ownership, and permissions
- shells, commands, and exit status
- processes and PIDs
- environment variables
- signals
- sessions, process groups, and controlling terminals
- package and file deployment concepts
- logs with `dmesg`, syslog, and journald
- basic networking commands
- basic storage and mount inspection

Intermediate:

- system calls
- file descriptors
- pipes, FIFOs, and redirection
- sockets
- `poll`, `select`, and `epoll`
- terminals, TTYs, and PTYs
- services and daemons
- systemd units, dependencies, ordering, and restart policy
- device access through `/dev`, sysfs, udev, and devtmpfs
- Linux capabilities
- core dumps
- userspace diagnostics with `strace`, `lsof`, `ss`, `ip`, `journalctl`, and `gdbserver`

Advanced:

- embedded read-only root filesystems
- `tmpfs`, `overlayfs`, bind mounts, and persistent state
- userspace hardware APIs
- service supervision and watchdog integration
- IPC design
- update-aware daemon design
- `rlimit` and cgroup constraints
- namespaces
- cross-compiled userspace applications
- target diagnostics and support bundles
- production logging
- failure containment

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

- what Device Tree solves
- DTS
- DTSI
- DTB
- overlays
- source include structure
- labels
- phandles
- node names
- unit addresses
- properties
- `reg`
- `ranges`
- `#address-cells`
- `#size-cells`
- `interrupt-parent`
- `interrupts`
- `dma-ranges`
- simple-bus
- `compatible`
- fallback compatible strings
- `of_match_table`
- platform devices
- binding-driven driver expectations
- optional vs required properties
- pinmux
- pin configuration
- GPIO controllers
- GPIO consumers
- active-high vs active-low
- interrupt controllers
- interrupt trigger types
- reset GPIOs
- clock providers
- clock consumers
- reset controllers
- fixed regulators
- PMIC regulators
- regulator constraints
- power domains
- wake sources
- UART nodes
- I2C nodes
- SPI nodes
- CAN nodes
- USB nodes
- PCIe nodes
- MMC/SD/eMMC nodes
- Ethernet MAC nodes
- Ethernet PHY nodes
- MDIO
- fixed-link
- `/memory`
- `/chosen`
- reserved memory
- CMA
- firmware nodes
- remoteproc
- RPMsg
- PRU
- R5/M4 cores
- U-Boot control DTB
- SPL DTB
- Linux DTB
- U-Boot-specific properties
- pre-relocation properties
- FIT image DTB selection
- YAML bindings
- `dt-bindings`
- `dtc` warnings
- `dtbs_check`
- schema errors
- `/proc/device-tree`
- `/sys/firmware/devicetree/base`
- decoded DTBs with `dtc`
- runtime DTB identity checks
- board porting DT workflow

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
