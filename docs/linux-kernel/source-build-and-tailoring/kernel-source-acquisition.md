---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Source Acquisition

## What Problem Does This Solve?

Driver work must start from the kernel source that matches the target system closely enough for APIs, configuration, symbols, and module ABI to line up.

## Core Concepts

- upstream kernel source
- vendor kernel source
- stable kernel branches
- BSP kernel trees
- source provenance
- exact commit identity
- kernel headers
- generated headers

## Mental Model

The useful kernel source tree is the one that produced the runtime kernel or the one the product will ship. A random upstream tree is useful for reading, but not automatically useful for module loading on a vendor BSP.

## Practice Skeleton

- Identify the running kernel release.
- Locate the matching source tree and commit.
- Check whether local patches are applied.
- Record source provenance for later debugging.

## Debugging Checklist

- Compare `uname -r` with the build tree.
- Check `Module.symvers` availability for external modules.
- Check vendor patches before assuming upstream API behavior.
- Keep source, config, and artifacts tied to one build identity.

## Related Topics

- [Source Tree And Outputs](../../build-systems/advanced/linux-kernel/source-tree-and-outputs.md)
- [Vendor Kernel Patch Management](../../build-systems/advanced/linux-kernel/vendor-kernel-patch-management.md)
- [Kernel Release Artifacts](../../build-systems/advanced/linux-kernel/kernel-release-artifacts.md)
