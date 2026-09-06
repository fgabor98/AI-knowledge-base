# Target Boot And Device Integration Tests

## Boot acceptance

On a clean target, verify image identity, boot completion, mount topology, writable paths, service enablement, user/group creation, capabilities, sandbox policy, clock handling, network setup, and log availability. Check both a normal boot and a boot with an expected dependency absent.

## Device contract

Exercise discovery, permissions, open/close, initialization, normal I/O, short or delayed I/O, hotplug/reconnect, reset, suspend/resume, and shutdown. Assert the exact errno or structured error where it is part of the contract. Verify that device nodes, sysfs attributes, udev rules, and service ordering agree.

Use a matrix of board revisions, kernel/configuration versions, firmware versions, peripherals, storage sizes, and network conditions. Record serial output, kernel messages, service state, device identity, and application logs with a common timeline.

Tests must leave the target recoverable. Bound retries, avoid filling persistent storage, and provide a rescue or reflash path when an intentionally induced fault prevents normal boot.
