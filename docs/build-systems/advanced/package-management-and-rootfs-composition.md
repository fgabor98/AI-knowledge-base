---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Package Management And Rootfs Composition

## What Problem Does This Solve?

Root filesystem content determines what runs on the device, what can be updated, what licenses apply, and what attack surface exists. Platform engineers need to understand packages, package feeds, manifests, split packages, and image composition.

## Core Concepts

- package recipe
- runtime dependency
- package split
- package feed
- image manifest
- debug package
- development package
- rootfs postprocess
- package manager database

## Rootfs Composition Flow

```text
recipes
-> packages
-> runtime dependency closure
-> image recipe/package groups
-> rootfs generation
-> image manifest
```

## Package Formats

Embedded build systems may use:

- rpm
- deb
- ipk
- no runtime package manager

The chosen format affects package feeds, updates, installed database size, and field diagnostics.

## Product Image Policy

Define:

- production package set
- debug package set
- development-only tools
- removable demo packages
- package feed policy
- license allowlist
- service enablement

Use package groups to make policy reviewable.

## Common Mistakes

- installing debug tools into production by accident
- adding packages in local configuration forever
- ignoring automatically pulled runtime dependencies
- removing package manager metadata without understanding update impact
- not archiving image manifests
- assuming two images are equivalent because they boot

## Related Topics

- [Yocto Images and Package Groups](yocto-openembedded/images-and-packagegroups.md)
- [Yocto Packaging, QA, and Package Feeds](yocto-openembedded/packaging-qa-and-feeds.md)
- [SDK Customization for Products](ti-processor-sdk/sdk-customization-for-products.md)

