---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Virtio And RPMsg

## What Problem Does This Solve?

Virtio and RPMsg provide communication channels between Linux and remote cores.

## Core Concepts

- virtio device
- vrings
- RPMsg endpoint
- name service
- shared memory
- mailbox or interrupt transport
- userspace RPMsg interfaces
- protocol design

## Mental Model

RPMsg is a messaging transport, not a complete application protocol. Product code still needs message formats, versioning, flow control, and error handling.

## Practice Skeleton

- Identify RPMsg devices.
- Send a test message.
- Define a minimal request and response protocol.
- Handle remote endpoint disappearance.

## Debugging Checklist

- Check vring memory.
- Check mailbox interrupts.
- Check endpoint names.
- Check firmware and Linux protocol compatibility.

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Reserved Memory](reserved-memory.md)
- [Remote Core Logs And Crashes](remote-core-logs-and-crashes.md)
