---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Module Signing And Hardening

## What Problem Does This Solve?

Module signing and hardening options reduce the risk of unauthorized kernel code and exploit-friendly runtime behavior.

## Core Concepts

- signed modules
- trusted keys
- secure boot
- lockdown
- kernel address exposure
- hardened usercopy
- stack protector
- read-only memory protections
- attack surface

## Mental Model

Hardening choices affect development, field diagnostics, and update workflows. Treat them as product requirements with a clear exception path.

## Practice Skeleton

- Enable module signing in a lab build.
- Attempt to load an unsigned module.
- Review hardening options in the final config.
- Document development exceptions.

## Debugging Checklist

- Check key enrollment and trust chain.
- Check lockdown mode.
- Check module load errors in `dmesg`.
- Confirm diagnostic tooling still has an approved path.

## Related Topics

- [Built-In Vs Module Policy](built-in-vs-module-policy.md)
- [Config Review Workflow](config-review-workflow.md)
- [Embedded Productization](../../embedded-productization/index.md)
