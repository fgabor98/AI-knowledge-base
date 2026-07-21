---
status: active
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Official Linux Kernel Documentation Reading Checklist

This is a progress tracker for a detailed reading of the
[official Linux kernel documentation](https://docs.kernel.org/). It follows the
official documentation table of contents, but changes the reading order so the
material needed for embedded driver and board work arrives first.

## Personal Project Focus

The priority order is tailored to the projects represented in this knowledge
base:

- TI Sitara AM335x, AM62x, and AM64x board bring-up
- Device Tree and binding validation
- platform, GPIO, I2C, SPI, UART, CAN, IIO, input, and PWM drivers
- clocks, resets, regulators, pin control, interrupts, DMA, and regmap
- Yocto and vendor-BSP kernel integration
- power management, suspend/resume, and wake-up debugging
- remoteproc, RPMsg, PRU, R5, and M4 firmware integration

## How To Use The Checklists

For every reading session, record the exact documentation baseline:

```text
Kernel documentation version/tag:
Kernel source commit:
Vendor kernel/version, if applicable:
Architecture/board:
Date:
```

Checkbox priorities:

- **P0**: read closely during the main path.
- **P1**: read after the main path or when starting the related project.
- **P2**: keep as a reference and read when the project requires it.

Mark a checkbox only after you can explain the document's main contract in your
own words. For P0 documents, also locate the relevant headers and at least one
in-tree driver.

Use the [Knowledge Guide Companion Checklist](knowledge-guide-companion.md) to
track all related Linux Kernel chapter pages in the same eight-stage order.

## Recommended Path

- [ ] [Knowledge Guide Companion Checklist](knowledge-guide-companion.md)
- [ ] 1. [Development Process And Kernel Source](01-development-process-and-source.md)
- [ ] 2. [Build System, Kconfig, And Development Tools](02-build-kconfig-and-devtools.md)
- [ ] 3. [Core APIs, Concurrency, Memory, And DMA](03-core-api-concurrency-memory.md)
- [ ] 4. [Driver Model, Device Tree, And Firmware](04-driver-model-devicetree-firmware.md)
- [ ] 5. [Embedded Driver Subsystems](05-embedded-driver-subsystems.md)
- [ ] 6. [Debugging, Tracing, And Testing](06-debugging-tracing-testing.md)
- [ ] 7. [Power Management And Heterogeneous SoCs](07-power-management-remoteproc.md)
- [ ] 8. [Architecture, Userspace ABI, Administration, And Optional Areas](08-architecture-userspace-optional.md)

## Official Table-Of-Contents Coverage

This mapping preserves the major sections of the official table of contents.

| Official documentation area | Checklist |
| --- | --- |
| Development process and submitting patches | 01 |
| Core API and locking | 03 |
| Driver APIs and subsystems | 04 and 05 |
| Licensing and writing documentation | 01 |
| Development tools, testing, hacking, tracing, and fault injection | 02 and 06 |
| Administration, build system, and reporting issues | 02, 06, and 08 |
| Userspace tools and userspace API | 08 |
| Firmware and Devicetree | 04 and 07 |
| CPU architectures | 08 |
| Livepatching, Rust, and unrelated subsystems | 08, mostly P2 |

## Deep-Reading Loop

For each P0 document:

```text
read the overview
-> write five to ten lines of notes
-> identify the public headers
-> find two in-tree users of the API
-> trace one call path into the implementation
-> run or design one lab
-> record version-dependent behavior
```

Useful note fields:

```text
Main contract:
Object/lifetime owner:
Execution context and sleepability:
Locking/concurrency rules:
Failure and teardown behavior:
Configuration dependencies:
Observable evidence:
Relevant source files and drivers:
Open questions:
```

## Refresh Policy

The unversioned `docs.kernel.org` site follows current kernel development and
its contents change over time. At the start of a serious project, compare these
checklists with both:

- the documentation built from the exact kernel tree used by the project; and
- the current upstream documentation for newer guidance and deprecations.
