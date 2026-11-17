---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Control Flow

Control flow determines which statements execute, how often they execute, and how execution leaves a function. In embedded systems a loop may need to service a watchdog, respect a hardware timeout, avoid blocking an interrupt context, or enter a safe state after a fault.

## Learning Objectives

- Write and review if, switch, and loop constructs.
- Make fall-through and loop termination intentional.
- Use early returns and goto for structured cleanup.
- Model a simple embedded state machine.
- Distinguish a bounded wait from an accidental infinite loop.
- Document blocking, timing, and recovery behavior.

## Blocks And Scope

A compound statement is a block delimited by braces:

<!-- c-check: skip -- compound-statement fragment -->

~~~c
{
    int local_value = 3;
    local_value++;
}
~~~

The block creates scope for identifiers declared within it. Use braces even for one-line branches in production embedded code. They prevent later statements from escaping the intended branch.

An if selects one statement when its condition is nonzero and optionally another when it is zero:

<!-- c-check: skip -- statement fragment -->

~~~c
if (error != 0) {
    record_error(error);
} else {
    continue_work();
}
~~~

The else attaches to the nearest unmatched if. Braces eliminate the ambiguity for human readers.

## switch

switch compares an integer or enumeration expression with case constants:

~~~c
void prepare_sample(void);
void collect_sample(void);
void disable_outputs(void);

enum device_state {
    DEVICE_IDLE,
    DEVICE_SAMPLING,
    DEVICE_FAULT
};

void service(enum device_state state)
{
    switch (state) {
    case DEVICE_IDLE:
        prepare_sample();
        break;

    case DEVICE_SAMPLING:
        collect_sample();
        break;

    case DEVICE_FAULT:
        disable_outputs();
        break;

    default:
        disable_outputs();
        break;
    }
}
~~~

Important rules:

- A case label does not create an implicit block.
- Execution continues into the next case unless break, return, or goto leaves the switch.
- Case values must be integer constant expressions and unique.
- default is optional but useful for defensive handling.
- A switch need not handle every value an enum can represent.

Intentional fall-through should be visible:

<!-- c-check: skip -- statement fragment inside a command handler -->

~~~c
switch (command) {
case CMD_START:
case CMD_RESUME:
    start_engine();
    break;
case CMD_STOP:
    stop_engine();
    break;
default:
    return STATUS_INVALID_COMMAND;
}
~~~

A compiler-specific fall-through attribute can document more complex cases, but it is an extension.

## for, while, And do while

A for loop groups initialization, continuation, and iteration:

<!-- c-check: skip -- statement fragment -->

~~~c
for (size_t i = 0u; i < count; ++i) {
    process(data[i]);
}
~~~

A while loop checks before each iteration:

<!-- c-check: skip -- statement fragment -->

~~~c
while (queue_has_work()) {
    process_one();
}
~~~

A do while loop executes at least once:

<!-- c-check: skip -- statement fragment -->

~~~c
do {
    sample = read_sample();
} while (sample < MIN_VALID_SAMPLE);
~~~

Use do while only when the first execution is always valid.

For every loop answer:

- What initializes the loop state?
- What makes progress?
- What ends normal operation?
- What happens on timeout or error?
- Can another execution context change the state?
- Is the maximum iteration count bounded?

## Polling And Timeouts

An embedded wait should normally include a timeout:

~~~c
#include <stdbool.h>
#include <stdint.h>

uint32_t clock_ticks(void);
bool peripheral_ready(void);
void service_background_work(void);

enum wait_result {
    WAIT_READY,
    WAIT_TIMEOUT
};

enum wait_result wait_until_ready(uint32_t start, uint32_t timeout)
{
    while ((uint32_t)(clock_ticks() - start) < timeout) {
        if (peripheral_ready()) {
            return WAIT_READY;
        }
        service_background_work();
    }

    return WAIT_TIMEOUT;
}
~~~

The unsigned subtraction pattern assumes the maximum wait is shorter than one counter wrap. Document whether service_background_work can block, re-enter the function, or observe changing hardware.

A loop with no bound can be valid in a scheduler or RTOS task:

~~~c
void wait_for_event(void);
void handle_event(void);

void worker_task(void)
{
    for (;;) {
        wait_for_event();
        handle_event();
    }
}
~~~

Document who stops it, its stack budget, fatal-error behavior, and shutdown or reset behavior.

## break And continue

break exits the nearest loop or switch. continue skips to the next iteration of the nearest loop:

<!-- c-check: skip -- statement fragment -->

~~~c
for (size_t i = 0u; i < count; ++i) {
    if (!sample_is_valid(data[i])) {
        continue;
    }

    if (sample_is_fatal(data[i])) {
        break;
    }

    store_sample(data[i]);
}
~~~

Use them to express a clear policy. Check that continue does not skip required cleanup at the bottom of a loop.

## return And Early Exit

An early return can clearly reject invalid input:

~~~c
#include <stdbool.h>
#include <stddef.h>

struct config;

bool config_is_valid(const struct config *config);
void apply_config(const struct config *config);

int configure(const struct config *config)
{
    if (config == NULL) {
        return -1;
    }
    if (!config_is_valid(config)) {
        return -2;
    }

    apply_config(config);
    return 0;
}
~~~

Replace unexplained numeric errors with a documented status type or named constants. If the function acquired resources or changed hardware state, every exit must preserve cleanup and rollback.

## goto For Structured Cleanup

A forward jump to one cleanup block can prevent duplicated error handling:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int lock_device(void);
void unlock_device(void);
int begin_transaction(void);
void end_transaction(void);
int write_payload(const uint8_t *data, size_t length);

int update_device(const uint8_t *data, size_t length)
{
    int result = -1;
    bool locked = false;
    bool transaction_started = false;

    if (lock_device() != 0) {
        goto out;
    }
    locked = true;

    if (begin_transaction() != 0) {
        goto out;
    }
    transaction_started = true;

    if (write_payload(data, length) != 0) {
        goto out;
    }

    result = 0;

out:
    if (transaction_started) {
        end_transaction();
    }
    if (locked) {
        unlock_device();
    }
    return result;
}
~~~

Good cleanup usage jumps forward, makes ownership explicit, avoids jumping into the scope of an uninitialized object, and labels remaining cleanup. Do not use arbitrary backward jumps to imitate loops.

## State Machines

A state machine makes legal transitions and per-state actions visible:

~~~c
#include <stdbool.h>

void enable_motor_power(void);
void disable_motor_power(void);
bool motor_is_stable(void);

enum motor_state {
    MOTOR_STOPPED,
    MOTOR_STARTING,
    MOTOR_RUNNING,
    MOTOR_FAULT
};

enum motor_event {
    MOTOR_EVENT_START,
    MOTOR_EVENT_TICK,
    MOTOR_EVENT_STOP,
    MOTOR_EVENT_ERROR
};

static enum motor_state motor_step(enum motor_state state,
                                   enum motor_event event)
{
    switch (state) {
    case MOTOR_STOPPED:
        if (event == MOTOR_EVENT_START) {
            enable_motor_power();
            return MOTOR_STARTING;
        }
        break;

    case MOTOR_STARTING:
        if (event == MOTOR_EVENT_ERROR) {
            disable_motor_power();
            return MOTOR_FAULT;
        }
        if (event == MOTOR_EVENT_TICK && motor_is_stable()) {
            return MOTOR_RUNNING;
        }
        break;

    case MOTOR_RUNNING:
        if (event == MOTOR_EVENT_STOP) {
            disable_motor_power();
            return MOTOR_STOPPED;
        }
        if (event == MOTOR_EVENT_ERROR) {
            disable_motor_power();
            return MOTOR_FAULT;
        }
        break;

    case MOTOR_FAULT:
        disable_motor_power();
        break;
    }

    return state;
}
~~~

Keep transition policy separate from hardware actions when possible. The state function can then be host-tested while a driver layer owns timing and registers.

## Exercises

1. Add braces to a nested if and explain which else the original binds to.
2. Write a switch over an enum with explicit expected states and a defensive default.
3. Implement a polling function returning ready, timeout, or hardware fault.
4. Refactor three duplicated cleanup paths into one forward cleanup block.
5. Model a UART receiver with idle, receiving, frame-ready, and error states.
6. State the invariant and maximum iteration count for every loop in a small driver.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: which if does the else bind to?"
    An `else` binds to the nearest unmatched `if` in the same block (C11 6.8.4.1p3), whatever
    the indentation says. Written without braces:

    ~~~text
    if (armed)
        if (fault)
            puts("A");
    else
        puts("B");
    ~~~

    This is parsed as the first function below, although the indentation suggests the second:

    ```c
    #include <stdio.h>

    void parsed_as(int armed, int fault)
    {
        if (armed) {
            if (fault) {
                puts("A");
            } else {
                puts("B");
            }
        }
    }

    void probably_intended(int armed, int fault)
    {
        if (armed) {
            if (fault) {
                puts("A");
            }
        } else {
            puts("B");
        }
    }
    ```

    GCC reports the ambiguous form (`-Wdangling-else`, part of `-Wall`). Braces on every branch
    remove the question. See [test-out Q5](./test-out.md#q5-nested-conditions).

??? note "Exercise 2: a switch with a defensive default"
    ```c
    enum link_state { LINK_DOWN, LINK_TRAINING, LINK_UP, LINK_ERROR };

    void link_power_off(void);
    void link_continue_training(void);
    void link_service(void);
    void link_recover(void);

    void link_step(enum link_state state)
    {
        switch (state) {
        case LINK_DOWN:
            link_power_off();
            break;
        case LINK_TRAINING:
            link_continue_training();
            break;
        case LINK_UP:
            link_service();
            break;
        case LINK_ERROR:
            link_recover();
            break;
        default:
            /* Not a valid enumerator: corrupted memory or a bad cast from an int. */
            link_recover();
            break;
        }
    }
    ```

    Every expected state has its own `case`, and `default` handles only values that should
    be impossible. Two details make this pattern safe:

    - An `enum` variable can hold any value of its underlying integer type, so `default` is
      reachable in practice, for example after `(enum link_state)raw_byte`.
    - `default` silences `-Wall`'s `-Wswitch` check. With a `default` and no `case` for
      `LINK_ERROR`, GCC 13 says nothing at `-Wall`. Only `-Wswitch-enum` still reports
      `enumeration value 'LINK_ERROR' not handled in switch`. Enable `-Wswitch-enum` when
      you use defensive defaults, so adding an enumerator breaks the build until every
      switch handles it.

??? note "Exercise 3: a polling function with three outcomes"
    ```c
    #include <stdbool.h>
    #include <stdint.h>

    #define STATUS_READY 0x01u
    #define STATUS_FAULT 0x80u

    enum poll_result { POLL_READY, POLL_TIMEOUT, POLL_FAULT };

    uint32_t read_status(void);
    uint32_t now_ticks(void);

    enum poll_result wait_ready(uint32_t timeout_ticks)
    {
        uint32_t start = now_ticks();

        for (;;) {
            uint32_t status = read_status();     /* read once per iteration */

            if ((status & STATUS_FAULT) != 0u) {
                return POLL_FAULT;
            }
            if ((status & STATUS_READY) != 0u) {
                return POLL_READY;
            }
            if ((uint32_t)(now_ticks() - start) >= timeout_ticks) {
                /* One last look: the task may have been preempted past the deadline
                   while the device became ready. */
                status = read_status();
                if ((status & STATUS_FAULT) != 0u) {
                    return POLL_FAULT;
                }
                return (status & STATUS_READY) != 0u ? POLL_READY : POLL_TIMEOUT;
            }
        }
    }
    ```

    - The status register is read once per iteration and both bits are tested on that
      value. Reading it twice can see two different states, and on some devices reading
      clears the fault bit.
    - Fault is checked before ready, so a device that reports both is not treated as
      successful.
    - `now_ticks() - start` is computed in `uint32_t`, so it stays correct when the tick
      counter wraps. `now_ticks() > start + timeout_ticks` would not.
    - The final read after the deadline avoids a false timeout when the caller was
      preempted for longer than the timeout.

??? note "Exercise 4: one forward cleanup block"
    Before: each failure path repeats the cleanup of everything acquired so far.

    ```c
    int clock_enable(void);
    void clock_disable(void);
    int dma_claim(void);
    void dma_release(void);
    int irq_request(void);
    void irq_free(void);
    int device_start(void);

    int open_duplicated(void)
    {
        if (clock_enable() != 0) {
            return -1;
        }
        if (dma_claim() != 0) {
            clock_disable();
            return -1;
        }
        if (irq_request() != 0) {
            dma_release();
            clock_disable();
            return -1;
        }
        if (device_start() != 0) {
            irq_free();
            dma_release();
            clock_disable();
            return -1;
        }
        return 0;
    }
    ```

    After: one exit path, with labels in reverse order of acquisition.

    ```c
    int clock_enable(void);
    void clock_disable(void);
    int dma_claim(void);
    void dma_release(void);
    int irq_request(void);
    void irq_free(void);
    int device_start(void);

    int open_device(void)
    {
        if (clock_enable() != 0) {
            goto fail;
        }
        if (dma_claim() != 0) {
            goto fail_clock;
        }
        if (irq_request() != 0) {
            goto fail_dma;
        }
        if (device_start() != 0) {
            goto fail_irq;
        }
        return 0;

    fail_irq:
        irq_free();
    fail_dma:
        dma_release();
    fail_clock:
        clock_disable();
    fail:
        return -1;
    }
    ```

    Each label undoes exactly one acquisition and falls through to the earlier ones, so
    the reverse order is visible in one place. Adding a fifth resource means one new label,
    instead of editing every later failure path, which is where the duplicated version
    eventually leaks a resource. The page's `update_device` example shows the other
    common style: one `out:` label plus a flag per resource.

??? note "Exercise 5: a UART receiver state machine"
    Frames start with `STX` (0x02), end with `ETX` (0x03), and carry at most 32 bytes:

    ```c
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    #define STX 0x02u
    #define ETX 0x03u
    #define FRAME_MAX 32u

    enum rx_state { RX_IDLE, RX_RECEIVING, RX_FRAME_READY, RX_ERROR };

    struct uart_rx {
        enum rx_state state;
        uint8_t frame[FRAME_MAX];
        size_t length;
    };

    /* Called for every received byte, or with line_error set on overrun/framing errors. */
    void uart_rx_byte(struct uart_rx *rx, uint8_t byte, bool line_error)
    {
        if (line_error) {
            rx->state = RX_ERROR;
            return;
        }
        switch (rx->state) {
        case RX_IDLE:
            if (byte == STX) {
                rx->length = 0;
                rx->state = RX_RECEIVING;
            }
            break;                          /* noise between frames is ignored */
        case RX_RECEIVING:
            if (byte == ETX) {
                rx->state = RX_FRAME_READY;
            } else if (rx->length < FRAME_MAX) {
                rx->frame[rx->length++] = byte;
            } else {
                rx->state = RX_ERROR;       /* overlong frame */
            }
            break;
        case RX_FRAME_READY:
            break;                          /* consumer has not taken the frame yet: drop */
        case RX_ERROR:
            if (byte == STX) {              /* resynchronize on the next start byte */
                rx->length = 0;
                rx->state = RX_RECEIVING;
            }
            break;
        default:
            rx->state = RX_ERROR;
            break;
        }
    }

    /* Consumer side: take a complete frame and return to idle. */
    bool uart_rx_take(struct uart_rx *rx, const uint8_t **frame, size_t *length)
    {
        if (rx->state != RX_FRAME_READY) {
            return false;
        }
        *frame = rx->frame;
        *length = rx->length;
        rx->state = RX_IDLE;
        return true;
    }
    ```

    The decisions worth checking in any receiver design are visible in the code:

    - what happens to bytes between frames (ignored in `IDLE`);
    - what happens to an overlong frame (error, with no write past the buffer);
    - what happens to bytes that arrive before the consumer takes the frame (dropped here;
      a double buffer would keep them);
    - how the receiver recovers from errors (it waits for the next `STX`).

    If `uart_rx_byte` runs in an interrupt and `uart_rx_take` in a task, `state` and the
    buffer are shared between contexts. Hand the frame over with an atomic state change or
    a short critical section (see
    [case study 11](../failure-case-studies/11-spin-wait-data-race.md)).

??? note "Exercise 6: invariants and iteration bounds"
    A small transmit path from a UART driver, with each loop's invariant and bound:

    ```c
    #include <stddef.h>
    #include <stdint.h>

    #define TX_FIFO_DEPTH 16u
    #define TX_TIMEOUT_TICKS 1000u

    int uart_tx_fifo_full(void);
    void uart_tx_put(uint8_t byte);
    uint32_t now_ticks(void);

    size_t uart_write(const uint8_t *data, size_t length)
    {
        size_t written = 0;
        uint32_t start = now_ticks();

        /* Loop 1 */
        while (written < length) {
            /* Loop 2 */
            while (uart_tx_fifo_full()) {
                if ((uint32_t)(now_ticks() - start) >= TX_TIMEOUT_TICKS) {
                    return written;
                }
            }
            uart_tx_put(data[written]);
            written++;
        }
        return written;
    }
    ```

    | Loop | Invariant | Maximum iterations |
    | --- | --- | --- |
    | 1 | bytes `data[0]` to `data[written - 1]` have been handed to the FIFO, and `written <= length` | `length`, because `written` increases by one each pass |
    | 2 | the FIFO was full at the last check, and less than `TX_TIMEOUT_TICKS` have passed since `start` | bounded by time, not by count: at most `TX_TIMEOUT_TICKS` ticks for the whole call |

    The useful questions this answers:

    - Loop 1 terminates because `written` increases every pass and is bounded by `length`.
    - Loop 2 does *not* terminate by its own condition if the hardware is stuck. The
      timeout is what bounds it, which is why every wait on hardware needs one.
    - The timeout covers the whole call, not each byte, so the total blocking time has a
      known upper limit. A per-byte timeout would allow `length × TX_TIMEOUT_TICKS`.

    Write these down as comments for every loop that touches hardware or untrusted data.

## Common Mistakes

- Omitting braces around a branch that will later receive another statement.
- Assuming switch automatically stops at the next case.
- Using default to hide an invalid state instead of recovering or recording it.
- Writing a polling loop without a timeout, progress condition, or watchdog policy.
- Forgetting that continue skips bottom-of-loop cleanup.
- Returning without releasing or rolling back resource and hardware state.
- Jumping backward or into an object’s scope.
- Using a giant switch instead of a documented state model.
- Relying on an enum’s apparent range for untrusted numeric input.

## Debugging Checklist

1. Instrument branch entry, loop iterations, and state transitions with bounded logging.
2. Test zero, one, timeout, invalid-input, and unexpected-event cases.
3. Check first and last valid indexes in every loop.
4. Verify whether break exits a switch or an enclosing loop as intended.
5. Verify cleanup on every return and error label.
6. Measure worst-case loop time on the target.
7. Check watchdog, interrupt, and task interaction for unbounded loops.
8. Capture the event and state sequence leading to a fault.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Expressions And Operators](./expressions-and-operators.md)
- [Functions](./functions.md)
- [Interrupts, Exceptions, And Faults](../embedded-c-and-hardware/interrupts-exceptions-and-faults.md)
- [Real-Time Constraints](../embedded-c-and-hardware/real-time-constraints.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [MISRA C guidance](https://www.misra.org.uk/misra-c/)
- [CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
