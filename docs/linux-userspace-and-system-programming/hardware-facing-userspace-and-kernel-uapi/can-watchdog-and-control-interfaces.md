---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# CAN, Watchdog, And Control Interfaces

## What problem does this solve?

CAN and watchdog devices are safety-relevant shared interfaces. A userspace program
must define arbitration/error behavior, frame validity, watchdog ownership, feed
criteria, and the consequences of process or power failure.

## CAN

SocketCAN exposes CAN through network sockets. Configure the interface separately
from the application, bind filters deliberately, and validate CAN ID, DLC, flags,
payload, timestamp, bus state, and sequence. CAN delivery is not application-level
acknowledgement; the protocol must define response and timeout semantics.

```sh
ip link show can0
ip -details link show can0
ip -details -statistics link show can0
```

Handle bus-off, error frames, interface down, arbitration loss, and reconnect. Do not
reset a shared bus blindly from one service.

## Watchdogs

A watchdog should be fed only after meaningful service health is proven. Define open
ownership, timeout, magic-close behavior, pretimeout notification, nowayout policy,
and reboot evidence. A supervisor that feeds a watchdog while the application is
dead defeats its purpose.

```text
application progress + required dependencies healthy
    -> feed watchdog
failure / hang / missing feed
    -> pretimeout evidence or hardware reset
```

## Control interfaces

Reset, power, actuator, and safety controls need authorization, rate limits, safe
defaults, and idempotence. Treat a command accepted by the driver as distinct from
the physical state reached. On shutdown or fault, drive the documented safe state.

## Distinguish the two watchdog layers

A service-manager watchdog checks one application's notifications. A hardware
watchdog checks that its owner continues feeding a device and can reset the
machine. Assign a single owner to each; PID 1 may supervise applications and
own the hardware watchdog independently.

Opening `/dev/watchdog*` may arm a timer. Reading watchdog documentation or
sysfs identity is appropriate for discovery; opening the device is an operation
with consequences. If `nowayout` is enabled, ordinary close will not disarm
it. Magic-close behavior depends on driver support and policy. Design tests
with a recovery console and a known reset outcome.

SocketCAN applications should distinguish classic CAN and CAN FD payload
limits and structures. Query/enable the required socket mode, interpret
identifier flags separately from identifier bits, and subscribe to error
frames deliberately. A successful send does not prove another ECU applied the
command. Bus-off recovery also belongs to an explicit interface-owner policy:
two services trying to restart a shared interface can disrupt each other.

## Common mistakes

- Treating CAN send as remote acknowledgement.
- Ignoring bus-off and error frames.
- Feeding a watchdog from an independent thread with no health proof.
- Assuming watchdog close always disables/reset behavior.
- Exposing privileged control UAPIs to an untrusted service.
- Retrying actuator commands without idempotence or state query.

## Debugging checklist

- Record interface state, CAN ID/DLC, error counters, watchdog timeout, and owner.
- Inspect `ip -details`, kernel logs, UAPI flags, and service credentials.
- Test bus-off, interface loss, peer reset, watchdog timeout, and process kill.
- Verify safe-state behavior and reset evidence after power cycling.
- Check one-owner policy for shared control resources.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
- [Service Lifecycle, Readiness, And Restart](../services-init-and-systemd/service-lifecycle-readiness-and-restart.md)

## References

- [Linux SocketCAN documentation](https://www.kernel.org/doc/html/latest/networking/can.html)
- [`can(7)`](https://man7.org/linux/man-pages/man7/can.7.html)
- [`watchdog(4)`](https://man7.org/linux/man-pages/man4/watchdog.4.html)
- [Linux watchdog API](https://www.kernel.org/doc/html/latest/watchdog/watchdog-kernel-api.html)
