---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Hardware-Service State Machines And Recovery

Hardware-facing code becomes more reliable when lifecycle is modeled explicitly. Typical states include `INIT`, `PROBING`, `READY`, `DEGRADED`, `RECOVERING`, and `STOPPING`; events include discovery, reply, timeout, disconnect, reset, configuration change, and shutdown.

For each state/event pair define the action, next state, timeout, retry budget, error reported to clients, and safe output. Avoid a collection of booleans whose combinations have not been reviewed. A state machine makes impossible transitions and stale completions visible.

## Recovery policy

Use bounded retries with backoff and jitter where appropriate. Distinguish transient transport failure, protocol incompatibility, device absence, permanent configuration error, and unsafe hardware state. Reinitialize only what is safe, and serialize reset/recovery with normal I/O. Preserve the original error and generation so a late response cannot move a newer session back to `READY`.

Define safe outputs during degraded and stopping states. On restart, reconstruct state from authoritative device observations and persistent configuration rather than assuming the previous process state survived. Emit state-transition evidence with device identity, request ID, generation, and reason.

Test every transition, duplicate/late event, cancellation, timeout, reconnect, repeated failure, and recovery-budget exhaustion. Include power loss and partial initialization when the hardware can retain state.

## A transition table for a single device owner

| State and event | Action | Next state |
| --- | --- | --- |
| DISCONNECTED, retry deadline | Open and identify device | PROBING or bounded backoff |
| PROBING, capabilities/config valid | Arm I/O and publish availability | READY |
| READY, admitted request | Assign request ID and deadline; submit | BUSY |
| BUSY, matching completion | Validate, respond, release request resources | READY |
| BUSY, deadline | Retire request; reconcile possible side effect | RECOVERING |
| Any active state, removal | Stop admission; invalidate generation; release safely | DISCONNECTED |
| Any state, stop | Disable retry, drain/cancel under shutdown budget | STOPPING |

Unexpected events require policy too. An unsolicited sample might be accepted
in READY, while a command reply without a live matching request is discarded
and counted. “Default: ignore” should not hide a protocol violation that
requires recovery.

## Generation tokens are not cancellation

Tag asynchronous work with device generation and request ID. Increment the
generation when replacing the device session. A late worker completion from
generation 4 must not mutate generation 5 state, but its resources still need
release and its external side effect may still require reconciliation.

For repeated failures, choose a capped backoff and retry budget. After budget
exhaustion, open a circuit: reject dependent work and schedule occasional
bounded probes or await an operator event. Define what successful observation
closes it. Resetting backoff after every successful open can still create a
rapid failure loop when configuration always fails.

## Recovery has its own failure paths

Readback may fail; reset may affect another device; closing a port may toggle
control lines. Record the required safe physical state and whether software
can establish it after losing communication. If it cannot, health must reflect
that uncertainty and a separate hardware mechanism may be required.

On startup, reconcile actual hardware state and durable configuration before
accepting commands. Test every transition with duplicate, late and out-of-order
events and verify that STOPPING cannot re-enter a reconnect path.

## Related topics

- [Stage 17 overview](index.md)
- [IPC protocols and versioning](../ipc-and-event-driven-design/ipc-protocols-and-versioning.md)
- [Lifecycle and hardware tests](../testing-and-verification/lifecycle-update-and-hardware-in-the-loop-tests.md)
