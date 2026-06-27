---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# cpuidle And cpufreq

## What Problem Does This Solve?

cpuidle and cpufreq manage CPU idle states and frequency scaling to trade performance, latency, and power.

## Core Concepts

- idle states
- governors
- latency
- frequency policy
- OPP tables
- thermal interaction
- scheduler interaction
- platform firmware

## Mental Model

CPU power management is platform policy with workload impact. Tune it using measured latency, throughput, thermal, and power behavior.

## Practice Skeleton

- Inspect available CPU frequencies.
- Inspect idle states.
- Change governors in a lab.
- Measure workload and latency impact.

## Debugging Checklist

- Check Device Tree OPP data.
- Check thermal throttling.
- Check governor selection.
- Check latency-sensitive workloads.

## Related Topics

- [Power Domains](power-domains.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Perf Overview](../debugging/perf-overview.md)
