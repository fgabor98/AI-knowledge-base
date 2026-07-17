---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Runtime Inspection

This page covers how to prove which Device Tree reached a running system and compare it with build inputs.

## Topics Covered

- `/proc/device-tree`
- `/sys/firmware/devicetree/base`
- decoded DTBs with `dtc`
- checking deployed DTB identity
- `dmesg` probe logs
- driver bind/unbind checks
- comparing source DTS to runtime tree
- inspecting NUL-terminated property values safely
- inspecting binary cells and byte arrays
- comparing DTB hashes across build, boot media, and target
- inspecting the tree from U-Boot before kernel handoff

## Related Topics

- [Build And Diagnostic Tools](build-and-diagnostic-tools.md)
- [Boot-Time Mutation And Ownership](boot-time-mutation-and-ownership.md)
