---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel C Survival Guide

## What Problem Does This Solve?

Kernel code uses C patterns that are uncommon in small userspace programs. Recognizing those patterns makes driver code much less opaque.

## Core Concepts

- embedded structs
- `container_of`
- intrusive lists
- function pointers
- callbacks
- macros
- `ERR_PTR`
- `IS_ERR`
- `PTR_ERR`
- `goto` cleanup
- bit flags
- fixed-width integer types

## Mental Model

Kernel C is object-oriented by convention. Structs embed other structs, callbacks describe behavior, and macros encode common ownership and type patterns.

## Practice Skeleton

- Read a small driver struct and identify embedded kernel objects.
- Follow one callback table from registration to invocation.
- Decode one `container_of` use.
- Rewrite one cleanup path as a clear resource-unwind sequence.

## Debugging Checklist

- Check whether a pointer is an error pointer before dereferencing it.
- Check which object owns each embedded struct.
- Check callback lifetime.
- Check cleanup labels in reverse allocation order.

## Related Topics

- [Reading Kernel Source](reading-kernel-source.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)
- [Resource Lookup And Managed Allocation](../fundamentals/resource-lookup-and-devm.md)
