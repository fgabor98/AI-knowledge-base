---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Power Management

This track covers runtime power control, system suspend and resume, wakeup behavior, and power dependencies in embedded Linux systems.

## Learning Materials

1. [Runtime PM](runtime-pm.md)
2. [Suspend And Resume](suspend-resume.md)
3. [Wake Sources](wake-sources.md)
4. [cpuidle And cpufreq](cpuidle-cpufreq.md)
5. [Power Domains](power-domains.md)
6. [Regulator And Clock Power Dependencies](regulator-clock-power-dependencies.md)
7. [Suspend And Resume Debugging](suspend-resume-debugging.md)

## Mental Model

Power management is a graph problem. Devices depend on clocks, regulators, resets, pin states, power domains, interrupts, and wakeup policy.

## Completion Criteria

- Explain runtime PM versus system sleep.
- Add basic suspend and resume callbacks.
- Identify wake sources.
- Debug a suspend failure with logs and tracing.

## Related Topics

- [Common Driver Interfaces](../driver-interfaces/index.md)
- [Device Tree](../../device-tree/index.md)
- [Embedded Linux](../../embedded-linux/index.md)
