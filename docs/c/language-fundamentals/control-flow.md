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

~~~c
{
    int local_value = 3;
    local_value++;
}
~~~

The block creates scope for identifiers declared within it. Use braces even for one-line branches in production embedded code. They prevent later statements from escaping the intended branch.

An if selects one statement when its condition is nonzero and optionally another when it is zero:

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

~~~c
for (size_t i = 0u; i < count; ++i) {
    process(data[i]);
}
~~~

A while loop checks before each iteration:

~~~c
while (queue_has_work()) {
    process_one();
}
~~~

A do while loop executes at least once:

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
#include <stdint.h>

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
