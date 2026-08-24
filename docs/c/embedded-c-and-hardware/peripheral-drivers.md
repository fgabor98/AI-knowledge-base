---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Peripheral Drivers

A peripheral driver translates a hardware contract into a software interface. It owns register access, initialization, state, buffering, interrupts/DMA, timeout, error recovery, and power transitions. A good driver does not merely expose every register; it provides a safe, testable policy for the use cases the product actually needs.

## Learning Objectives

- layer a driver from register access to application-facing service;
- handle initialization, reset, clocks, pin mux, and power state;
- design nonblocking and blocking interfaces with explicit context rules;
- integrate interrupt, DMA, queue, and timeout behavior;
- recover from peripheral errors and repeated start/stop operations;
- test drivers through host fakes, target tests, and fault injection.

## Driver Layers

```text
application policy
        |
device service: ownership, buffering, state, timeout
        |
primitive driver: register/DMA/ISR transactions
        |
HAL/BSP: clocks, pins, reset, addresses, IRQ mapping
        |
silicon peripheral and board wiring
```

Keep the application independent of register names and vendor headers. Keep the primitive layer small enough to review against the reference manual. Put protocol parsing, retries, and user policy above the hardware transaction layer unless the driver owns that policy by design.

## Common Driver Lifecycle

Define a state machine such as:

```text
RESET -> CLOCKED -> CONFIGURED -> IDLE -> ACTIVE
  ^         |          |           |       |
  +---------+----------+-----------+-------+
                   ERROR/RECOVERY
```

For each transition define:

- preconditions and ownership;
- register writes and required delays/readbacks;
- interrupt and DMA state;
- buffers and queues;
- timeout and rollback;
- behavior on repeated initialization or close;
- safe state during power loss and reset.

Make initialization idempotent where practical. A driver that only works after one exact boot sequence is difficult to recover in the field.

## Interface Design

Prefer operations that expose progress and failure:

~~~c
#include <stddef.h>
#include <stdint.h>

enum serial_status {
    SERIAL_OK = 0,
    SERIAL_INVALID = -1,
    SERIAL_BUSY = -2,
    SERIAL_TIMEOUT = -3,
    SERIAL_IO_ERROR = -4
};

struct serial_device;

enum serial_status serial_start(struct serial_device *device,
                                uint32_t baud_rate);
enum serial_status serial_write(struct serial_device *device,
                                const unsigned char *data,
                                size_t length,
                                size_t *accepted);
enum serial_status serial_stop(struct serial_device *device);
~~~

State the context for each function. `serial_write` might be task-only and blocking; `serial_try_write` might be ISR-safe and nonblocking; `serial_start` might require clocks and interrupts disabled. Do not make one function silently change behavior based on its caller context.

## GPIO, Timers, And PWM

GPIO drivers must define direction, pull, drive, alternate-function, electrical, and atomic set/clear behavior. Timer/PWM drivers must define clock source, prescaler, counter width, compare update timing, output polarity, synchronization, and behavior at wraparound. For a safety actuator, verify the reset and fault output state before enabling the peripheral.

## UART, SPI, I2C, And CAN

Serial drivers differ in their transaction contract:

- **UART** — framing errors, FIFO, baud tolerance, break, flow control, partial writes;
- **SPI** — chip-select ownership, mode, word width, full-duplex behavior, DMA boundaries;
- **I2C** — arbitration, clock stretching, ACK/NACK, bus recovery, multi-master behavior;
- **CAN** — arbitration, bit timing, acceptance filters, error states, bus-off recovery.

Make transaction ownership explicit. A shared SPI bus needs serialization around chip-select and transfer configuration; an I2C bus may need a recovery sequence for stuck lines; a CAN driver must not report “sent” when the hardware only queued a frame.

## USB, Ethernet, And Storage

Complex peripherals require layered state machines:

- USB enumeration, endpoint ownership, setup requests, power and suspend;
- Ethernet descriptor rings, cache coherency, link state, packet lifetime, and backpressure;
- flash/EEPROM erase/program alignment, wear, power loss, and verification;
- SD/eMMC initialization, command timing, DMA, filesystem boundaries, and removal/error state.

Keep media transaction errors separate from filesystem or protocol errors. A storage driver should not silently retry forever or claim persistence before the device reports completion.

## Interrupt And DMA Integration

An ISR should acknowledge the source, capture status, move or mark data, and notify deferred work. A DMA completion handler should validate descriptor status and length, perform cache/ownership operations, and return buffers to the correct owner. Avoid parsing or logging full payloads in the interrupt path.

The interface between ISR and task needs:

- event loss policy;
- buffer ownership;
- memory ordering;
- queue capacity and backpressure;
- cancellation and reset behavior;
- context switch request rules.

## Timeouts And Recovery

Every wait for hardware should have a deadline and a recovery action. Recovery may include:

1. stop new requests;
2. mask/clear interrupts;
3. stop DMA and wait for quiescence;
4. capture status and error evidence;
5. reset the peripheral or bus;
6. restore clocks/pins/configuration;
7. invalidate stale buffers and queues;
8. return a classified error or enter a safe state.

Do not reset a peripheral while another task can still access its registers or buffers. Serialize lifecycle transitions.

## Error Model

Separate errors that callers can recover from:

- invalid argument or state;
- busy/queue full/backpressure;
- timeout;
- protocol/framing error;
- hardware fault or bus error;
- power/offline state;
- permanent configuration failure.

Preserve low-level evidence in a diagnostic structure, but expose a stable application-facing status. Returning a generic `-1` makes retry, telemetry, and safety decisions ambiguous.

## Host Fakes And Target Tests

A fake should model register reads/writes, status changes, time, interrupts, DMA completion, and error injection. Tests should verify:

- exact initialization sequence;
- no access before clock/reset release;
- register masks and reserved bits;
- timeout and recovery;
- repeated start/stop;
- queue and buffer ownership;
- power transitions and reset cause;
- behavior under malformed or missing hardware response.

Use a logic analyzer, bus trace, or peripheral loopback to validate transactions on hardware. A fake cannot detect an incorrect pin mux, electrical level, or silicon erratum.

## Driver Review Checklist

- Is the reference manual version and silicon revision recorded?
- Are all registers accessed with correct width and alignment?
- Are reset values, reserved bits, and side effects handled?
- Is each API allowed from task, ISR, startup, and fault contexts?
- Are timeouts monotonic and bounded?
- Are DMA addresses, cache, and ownership correct?
- Can cancellation or reset race with completion?
- Are errors observable and classified?
- Is the driver safe after brownout, watchdog, and repeated initialization?
- Are host and target tests both present?

## Exercises

1. Design a UART driver API with task, ISR, and DMA variants.
2. Implement a fake register block with delayed status and injected errors.
3. Build an SPI bus manager and prove chip-select ownership under concurrency.
4. Add I2C stuck-bus recovery and test every line state.
5. Implement a DMA-backed receive path with explicit buffer ownership.
6. Interrupt a flash update and verify power-loss recovery.
7. Review a vendor HAL call sequence against the reference manual and target trace.

## Common Mistakes

- exposing raw registers as the entire driver API;
- hiding blocking or allocation behind a generic function;
- not defining repeated initialization and recovery;
- reporting queued data as transmitted or persisted;
- parsing in an ISR;
- resetting hardware while DMA or another task still owns it;
- ignoring bus/pin/clock/power state;
- using a fake that always returns ideal status;
- collapsing all errors into one value;
- treating a vendor HAL as proof that the sequence matches the silicon revision.

## Related Topics

- [Memory-Mapped I/O](./memory-mapped-io.md)
- [Interrupts, Exceptions, And Faults](./interrupts-exceptions-and-faults.md)
- [DMA, Cache, And Memory Barriers](./dma-cache-and-memory-barriers.md)
- [RTOS Integration](./rtos-integration.md)
- [API And Opaque Types](../modular-design-and-apis/api-and-opaque-types.md)

## References

- [CMSIS device header and peripheral access](https://arm-software.github.io/CMSIS_5/Core/html/device_h_pg.html)
- [Zephyr device driver model](https://docs.zephyrproject.org/latest/kernel/drivers/index.html)
- [FreeRTOS queues and ISR API](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/01-Queues)
- [Linux driver DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
