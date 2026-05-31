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

## Bash Programming

Beginner:

- shell execution model
- command lookup
- shell variables vs environment variables
- quoting
- variables and expansion
- parameter expansion
- command substitution
- arithmetic expansion
- arrays
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
- process substitution
- subshells
- reading lines safely
- NUL-delimited filename handling
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

## Linux Kernel Programming

- kernel source tree layout
- building the kernel
- kernel configuration
- kernel modules
- module parameters
- init and exit paths
- character devices
- device model
- device tree
- platform drivers
- probe and remove lifecycle
- sysfs
- debugfs
- procfs
- GPIO subsystem
- PWM subsystem
- I2C
- SPI
- pinctrl
- clocks
- resets
- regulators
- interrupts
- threaded IRQs
- workqueues
- timers
- completions
- wait queues
- locking
- atomic operations
- memory allocation
- DMA basics
- kernel logging
- ftrace
- dynamic debug
- kernel oops analysis

## Embedded Linux

- boot chain
- BootROM
- SPL
- U-Boot
- Linux handoff
- kernel command line
- device tree overlays
- root filesystem layout
- init systems
- systemd basics
- BusyBox systems
- Yocto basics
- Buildroot basics
- cross-compilation
- toolchains
- kernel configuration
- board bring-up
- serial console
- networking bring-up
- storage layout
- partitioning
- A/B update schemes
- secure boot concepts
- field diagnostics
- production logging

## Debugging

- reading compiler diagnostics
- reproducing failures
- reducing test cases
- GDB basics
- core dumps
- strace
- ltrace
- perf
- ftrace
- dynamic debug
- tcpdump
- logic analyzer workflow
- serial console workflow
- logging strategy

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

## Build Systems

- Make basics
- recursive Make
- CMake basics
- pkg-config
- cross-compilation
- sysroots
- toolchain files
- dependency tracking
- reproducible builds
- CI build checks
- Yocto recipes
- BitBake tasks
- Buildroot packages

## Patterns

- ownership and lifetime
- initialization and teardown
- state machines
- error propagation
- retry and timeout handling
- resource cleanup
- concurrency boundaries
- producer-consumer queues
- configuration layering
- logging levels
- feature flags
- compatibility shims
