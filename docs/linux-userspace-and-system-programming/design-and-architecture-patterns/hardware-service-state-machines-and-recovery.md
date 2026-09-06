# Hardware-Service State Machines And Recovery

Hardware-facing code becomes more reliable when lifecycle is modeled explicitly. Typical states include `INIT`, `PROBING`, `READY`, `DEGRADED`, `RECOVERING`, and `STOPPING`; events include discovery, reply, timeout, disconnect, reset, configuration change, and shutdown.

For each state/event pair define the action, next state, timeout, retry budget, error reported to clients, and safe output. Avoid a collection of booleans whose combinations have not been reviewed. A state machine makes impossible transitions and stale completions visible.

## Recovery policy

Use bounded retries with backoff and jitter where appropriate. Distinguish transient transport failure, protocol incompatibility, device absence, permanent configuration error, and unsafe hardware state. Reinitialize only what is safe, and serialize reset/recovery with normal I/O. Preserve the original error and generation so a late response cannot move a newer session back to `READY`.

Define safe outputs during degraded and stopping states. On restart, reconstruct state from authoritative device observations and persistent configuration rather than assuming the previous process state survived. Emit state-transition evidence with device identity, request ID, generation, and reason.

Test every transition, duplicate/late event, cancellation, timeout, reconnect, repeated failure, and recovery-budget exhaustion. Include power loss and partial initialization when the hardware can retain state.
