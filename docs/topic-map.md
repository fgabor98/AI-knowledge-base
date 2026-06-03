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

## Signal Processing

### Page Taxonomy

- Root page: `docs/signal-processing/index.md`
- Diagram instructions: `docs/signal-processing/diagram-instructions.md`
- Subtopic page pattern: each subtopic has `index.md`, `theory.md`, `programming.md`, and `references.md`
- Signals And Systems:
  - `docs/signal-processing/signals-and-systems/index.md`
  - `docs/signal-processing/signals-and-systems/theory.md`
  - `docs/signal-processing/signals-and-systems/programming.md`
  - `docs/signal-processing/signals-and-systems/references.md`
- Digital Signal Processing:
  - `docs/signal-processing/digital-signal-processing/index.md`
  - `docs/signal-processing/digital-signal-processing/theory.md`
  - `docs/signal-processing/digital-signal-processing/programming.md`
  - `docs/signal-processing/digital-signal-processing/references.md`
- Measurement And Instrumentation:
  - `docs/signal-processing/measurement-and-instrumentation/index.md`
  - `docs/signal-processing/measurement-and-instrumentation/theory.md`
  - `docs/signal-processing/measurement-and-instrumentation/programming.md`
  - `docs/signal-processing/measurement-and-instrumentation/references.md`
- Infocommunication:
  - `docs/signal-processing/infocommunication/index.md`
  - `docs/signal-processing/infocommunication/theory.md`
  - `docs/signal-processing/infocommunication/programming.md`
  - `docs/signal-processing/infocommunication/references.md`
- Theory pages own definitions, equations, algorithms, assumptions, and design tradeoffs.
- Programming pages own implementation sequence, exercises, deliverables, test strategy, and reusable code plans.
- References pages own source PDFs, confidence notes, extraction limits, and pending review tasks.

### Signals And Systems

Beginner:

- continuous-time signals
- discrete-time signals
- deterministic signal classes
- stochastic signal classes
- periodic, quasiperiodic, and transient signals
- unit step and unit impulse
- Dirac impulse
- Kronecker impulse
- shifted signals
- rectangular windows
- signal energy and power
- mean, variance, and RMS
- basic convolution
- impulse response
- step response

Intermediate:

- continuous-time LTI systems
- discrete-time LTI systems
- linearity, time invariance, causality, and stability
- first-order and second-order systems
- state-space simulation
- matrix exponential
- difference equations
- transfer functions
- transfer characteristics
- poles and zeros
- Bode diagrams
- Nyquist diagrams
- Fourier series
- Fourier transform
- Laplace transform
- DFT
- DTFT
- z-transform
- sampling theorem
- aliasing
- reconstruction
- zero-order hold
- sinc interpolation
- all-pass systems
- minimum-phase systems

Advanced:

- BIBO stability
- asymptotic stability
- eigenvalue stability checks
- pole/root stability checks
- continuous-to-discrete simulation
- bilinear transform
- nonlinear operating-point solving
- Newton-Raphson
- bisection
- explicit Euler
- implicit Euler
- small-signal linearization
- dynamic resistance, capacitance, and inductance
- stochastic sampling theory
- Parseval relations
- distortion-free transmission
- filter tolerance schemes

### Digital Signal Processing

Beginner:

- numerical signal arrays
- sample indexing
- sample period and sample rate
- quantization basics
- deterministic test signals
- ADC and DAC signal-chain model
- finite sample records
- complex numbers and phasors
- plotting time-domain signals
- plotting spectra
- simple averaging filters

Intermediate:

- FIR filters
- IIR filters
- direct-form realizations
- second-order sections
- numerical robustness in recursive filters
- FFT usage
- finite-record spectral analysis
- coherent and noncoherent sampling
- spectral leakage
- picket-fence and scalloping effects
- zero padding and FFT interpolation
- window functions
- periodogram
- autocorrelation and power spectral density
- moving average filters
- exponential average filters
- recursive DFT
- resonator-based Fourier analyzer
- adaptive Fourier analyzer
- band-selective and zoom FFT
- digital filter design
- Butterworth filters
- Chebyshev filters
- elliptic filters
- FIR window-method design
- equiripple/Remez FIR design
- quantization error model
- quantization noise whitening
- dither

Advanced:

- quantization theorems
- Sheppard corrections
- DFT as a filter bank
- DFT as an observer
- observer design for linear systems
- spectral-estimator variance
- Bartlett and Welch averaging
- equivalent noise bandwidth
- model fitting
- parameter estimation
- regression
- Wiener-Hopf equation
- adaptive linear combiner
- steepest descent
- LMS
- NLMS
- LMS-Newton
- adaptive IIR filters
- equation-error and output-error adaptation
- FxLMS
- adaptive line enhancement
- echo cancellation
- active noise control
- inverse filtering
- Tikhonov regularization
- Wiener filtering
- Kalman filtering
- Bayesian sensor fusion
- Dempster-Shafer fusion
- resonator-observer data compression
- real-time DSP processor workflow
- reproducible DSP testing

### Measurement And Instrumentation

Beginner:

- measurement-chain model
- sensor, signal conditioning, ADC, processor, and actuator path
- oscilloscope basics
- pulse parameters
- rise time and fall time
- overshoot and undershoot
- settling time
- amplitude measurement
- phase measurement
- decibel calculations
- averaging for noise reduction
- finite measurement records
- measurement saturation and dynamic range

Intermediate:

- frequency-domain signal analysis
- Fourier-series measurement from sampled data
- DFT and FFT scaling in instruments
- line spectrum interpretation
- window selection for measurements
- leakage and noncoherent sampling in instruments
- amplitude-characteristic measurement
- phase-characteristic measurement
- stepped-sine measurement
- transfer-characteristic measurement
- time-domain reflectometry
- distributed data acquisition
- wireless sensor-network measurements
- data packet timing
- local timestamps
- synchronization points
- linear timestamp transformation

Advanced:

- measurement-chain compensation
- clock offset and drift estimation
- timestamp transformation
- interpolation-based resynchronization
- interpolation-based synchronization correction
- distributed sensor-network synchronization
- resonator-observer measurement systems
- Fourier-coefficient based data compression
- active noise control measurement
- feedback over wireless sensor networks
- measurement-chain inverse filtering
- bandwidth extension by compensation
- equivalent-time sampling
- oscilloscope calibration by inverse filtering
- multisensor fusion for range and bandwidth extension
- real-time DSP card experiments
- neural-network spectral classification

### Infocommunication

Beginner:

- communication system model
- source, transducer, channel, receiver, sink
- analog vs digital signals
- sampling and reconstruction
- quantization and quantization noise
- ADC and DAC model
- channel classification
- bandwidth
- noise
- attenuation
- signal-to-noise ratio
- decibels
- information content
- source coding
- codeword length
- prefix-free codes
- Hamming distance
- error detection
- error correction

Intermediate:

- antenna properties
- antenna gain
- effective aperture
- free-space path loss
- two-ray propagation
- interference zones
- channel coding
- linear block codes
- generator matrix
- parity-check matrix
- syndrome decoding
- amplitude modulation
- DSB, DSB-SC, and SSB
- envelope detection
- coherent demodulation
- angle modulation
- FM and PM
- Carson bandwidth rule
- frequency-division multiplexing
- time-division multiple access
- CDMA
- OFDM
- baseband digital modulation
- PCM
- PAM
- symbol rate and bit rate
- inter-symbol interference
- Nyquist ISI criterion
- eye diagrams
- raised-cosine pulse shaping
- carrier digital modulation
- ASK
- PSK
- FSK
- QAM
- constellation diagrams
- frequency hopping

Advanced:

- matched transmit and receive filters
- channel equalization basics
- decision thresholds
- demodulation error from phase and amplitude offset
- bit error probability intuition
- audio perception and SPL
- psychoacoustic masking
- image perception and color spaces
- analog radio receiver architecture
- superheterodyne receiver
- image-frequency rejection
- preemphasis and deemphasis
- digital radio
- MPEG-style compression overview
- DVB-C, DVB-S, and DVB-T
- QAM with OFDM in broadcast systems
- cellular network architecture
- PLMN
- cell planning
- frequency reuse
- sectorization
- GSM channel structure
- GSM burst and TDMA frame concepts
- speech coding in mobile systems
- VoIP basics

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
