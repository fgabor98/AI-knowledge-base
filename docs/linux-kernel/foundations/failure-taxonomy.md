---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Failure Taxonomy

## What Problem Does This Solve?

Many kernel failures look similar at first. Classifying the failure narrows the investigation and prevents random debugging.

## Core Concepts

- build failure
- module load failure
- symbol mismatch
- probe failure
- missing firmware data
- missing provider
- resource lookup failure
- runtime bug
- oops
- hang
- watchdog reset
- race
- lifetime bug

## Mental Model

Before fixing anything, classify where the failure happens: build time, load time, bind/probe time, runtime operation, teardown, suspend/resume, or crash recovery.

## Practice Skeleton

- Collect one example from each failure class.
- Record the primary symptom and first command to inspect it.
- Build a decision tree for module load vs probe vs runtime failures.

## Debugging Checklist

- Check whether the code built.
- Check whether the module loaded.
- Check whether the device exists.
- Check whether the driver matched.
- Check whether resources were acquired.
- Check whether the first runtime operation failed.

## Related Topics

- [Probe Failure Debugging](../debugging/probe-failure-debugging.md)
- [Oops, Panic, And Crash Logs](../debugging/oops-panic-crash-logs.md)
- [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)
