# Design Review And Userspace Contracts

## Contract contents

A useful contract specifies data representation and limits, ownership and lifetime, synchronization, ordering, timeout/cancellation behavior, errors and errno mapping, version negotiation, permissions, persistence, and observability. Include what happens when the peer is old, malformed, slow, restarted, or absent.

For a device or IPC operation, answer: who may call it, what state is required, what can change concurrently, which side owns buffers, whether partial completion is possible, how retries avoid duplication, and how an operator correlates the result with logs.

## Review artifacts

Use a small decision record containing requirements, alternatives, selected design, rejected options, assumptions, failure matrix, resource budgets, security boundaries, and test plan. Draw the process/resource ownership and state-transition diagrams when prose hides relationships.

Review the unhappy paths explicitly:

| Area | Question |
| --- | --- |
| Startup | What if configuration, device, dependency, or storage is absent? |
| Runtime | What if input is malformed, slow, duplicated, or too large? |
| Failure | What if a call blocks, returns short, or the peer dies? |
| Recovery | What state is safe after restart, reset, or power loss? |
| Security | Which identity and policy authorize each effect? |
| Operations | What evidence identifies the exact failure and build? |

An architecture is ready when its contracts can be implemented by an independent component, tested with a fake and a real boundary, and operated during degradation. Revisit the record when measurements or target constraints invalidate an assumption.
