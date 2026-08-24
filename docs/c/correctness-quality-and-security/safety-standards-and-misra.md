---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Safety Standards And MISRA

Safety standards define a development and assurance process for systems whose malfunction can cause unacceptable harm. Coding rules are one part of that process. MISRA C, CERT C, compiler diagnostics, static analysis, tests, traceability, configuration management, independence, and safety analysis each answer different questions.

This page is educational guidance, not a claim of compliance with any standard. The applicable edition, sector, safety integrity level, organization, assessor, and project plan determine the required evidence.

## Learning Objectives

- distinguish functional safety, secure coding, and general software quality;
- understand the role and limits of MISRA C and CERT C;
- create a rule configuration, deviation process, and traceability chain;
- connect coding rules to hazards, requirements, tests, analysis, and tool qualification;
- recognize when a rule is advisory, required, project-tailored, or not applicable;
- prepare evidence without confusing a clean report with a safe product.

## Safety Versus Security

| Concern | Primary question | Typical failure |
| --- | --- | --- |
| functional safety | can malfunction cause harm? | loss of control, hazardous actuator state |
| cybersecurity | can an adversary cause harm or gain unauthorized capability? | crafted packet, key theft, malicious update |
| reliability | does the system perform consistently? | wearout, timeout, resource exhaustion |
| quality | is the implementation understandable and maintainable? | regression, ambiguous ownership |

The concerns overlap. A memory corruption can be a random fault, a software defect, or an exploit. A safety mechanism that accepts unauthenticated commands can create a security hazard. Use the applicable processes together without treating one as a substitute for the others.

## MISRA C

MISRA C provides guidelines for using C in critical systems. The rules address dangerous or ambiguous language behavior, constrained subsets, essential type usage, control flow, pointer use, declarations, library use, and project configuration. The exact edition and amendments matter; use the official rule text and a licensed/commercial or approved analysis workflow as required by the project.

Understand rule categories:

- **required** rules generally need compliance or a documented deviation;
- **advisory** rules allow project judgment but still need a rationale when ignored;
- **mandatory** rules cannot normally be deviated from;
- **directive** rules concern process or broader design evidence;
- **single translation unit versus project-wide** scope changes how a tool can check them.

Do not copy a generic MISRA configuration into a different compiler, C dialect, RTOS, or hardware project without reviewing its assumptions.

## CERT C

CERT C focuses on secure, safe, and reliable C coding practices, including preprocessor use, declarations, expressions, integers, arrays, strings, memory management, I/O, error handling, APIs, concurrency, and platform-specific rules. CERT rules are valuable for threat and defect review, but they do not by themselves create functional-safety compliance.

Use CERT C to ask:

- can attacker-controlled data reach this sink?
- can arithmetic wrap or convert unexpectedly?
- is an error ignored or misinterpreted?
- can a resource leak or race alter security state?
- does a library call have the required contract on this target?

## Sector Standards

The sector determines the safety lifecycle and evidence expectations:

| Standard/family | Domain focus | Typical software evidence themes |
| --- | --- | --- |
| IEC 61508 | generic functional safety | lifecycle, hazard/risk, systematic capability, verification |
| ISO 26262 | road vehicles | item safety, ASIL, software lifecycle, freedom from interference |
| DO-178C | airborne software | planning, requirements, verification, structural coverage, configuration |
| IEC 62304 | medical-device software | lifecycle, risk, verification, maintenance |
| IEC 61511 | process-industry safety instrumented systems | safety lifecycle and instrumented functions |

Read the actual applicable standard and organizational procedures. A page in a learning repository cannot establish compliance, safety integrity, or tool qualification.

## Traceability

Traceability connects:

```text
hazard -> safety goal -> system requirement -> software requirement
       -> architecture/design -> implementation -> test/analysis evidence
       -> review result -> release configuration -> field feedback
```

Each link should be specific enough to audit. “Tested by integration suite” is weaker than a test identifier that exercises a requirement’s normal, boundary, failure, timing, and recovery behavior on a named configuration.

Traceability also works backward: every safety-critical line, assertion, diagnostic, and suppression should have a reason in the requirements or architecture; every requirement should have evidence; every test should identify the configuration it ran.

## Rule Configuration

Define a project profile containing:

- C edition and compiler extensions;
- target architectures and ABI variants;
- generated-code rules;
- operating-system/RTOS and ISR conventions;
- permitted library and dynamic-allocation policy;
- integer, floating-point, pointer, and endian assumptions;
- naming, complexity, comment, and documentation rules;
- required diagnostics and analyzer versions;
- rule applicability, deviations, and evidence ownership.

Check the profile itself into version control. Treat a compiler, analyzer, rule-package, or configuration upgrade as a change requiring impact analysis.

## Deviations

Some rules conflict with required hardware, ABI, performance, or legacy integration. A good deviation record states:

1. exact rule and affected code/configuration;
2. why compliance is impractical or incorrect here;
3. hazard and security impact;
4. alternative control or compensating mechanism;
5. tests, analysis, assertions, or review that provide evidence;
6. scope and expiration/review condition;
7. independent approval where required.

Example: a narrow MMIO wrapper may require a compiler extension for an address-space or section attribute. The deviation should not authorize that extension throughout the project; it should constrain the wrapper, validate register width/order, and test the hardware boundary.

## Tool Confidence And Qualification

An analyzer can produce useful evidence without being qualified for every safety claim. Determine:

- what the tool actually checks;
- version and target support;
- known limitations and false-negative risks;
- configuration and model integrity;
- whether the project requires qualification or a validation argument;
- how tool output is reviewed and archived.

Independent review, diverse tools, tests, and manual analysis can reduce common-mode risk. Do not claim that a tool found all violations because its report is empty.

## Freedom From Interference

Safety-related components may need protection from interference by other software or hardware:

- memory and stack separation;
- CPU time and deadline budgets;
- controlled communication and API ownership;
- interrupt priority and latency limits;
- DMA and peripheral access control;
- shared-resource locking and failure containment;
- startup, reset, and watchdog independence;
- configuration and update separation.

On a small MCU, logical module separation alone may not provide freedom from interference. Use MPU/MPU-like hardware, scheduling analysis, linker regions, monitors, and integration tests where required.

## Defensive C For Safety

- make invalid states explicit and reject them early;
- use fixed-size or bounded resources in safety paths;
- avoid uncontrolled recursion and unbounded loops;
- define behavior for every error and timeout;
- make initialization and reset idempotent where possible;
- use diagnostic checks with known coverage and reaction time;
- avoid hidden global state and implicit startup dependencies;
- verify compiler/linker output and memory placement;
- instrument fault records without creating a second failure.

These practices support safety but do not replace hazard analysis, architecture, or system validation.

## Exercises

1. Select a small set of MISRA and CERT rules for a packet driver and justify each.
2. Create a rule profile for host, RTOS, and bootloader builds.
3. Write a deviation for one unavoidable compiler extension and include compensating evidence.
4. Build a requirements-to-test-to-analysis trace for an actuator command.
5. Identify interference paths between a safety task, logger, DMA engine, and bootloader.
6. Review a tool upgrade and classify changes in findings, configuration, and evidence.
7. Compare a clean coding report with a full safety argument and list what is still missing.

## Common Mistakes

- treating MISRA compliance as proof of functional safety;
- treating CERT guidance as a replacement for threat modeling;
- copying rule configurations without target and compiler review;
- suppressing required rules without a deviation record;
- leaving deviations permanent and unowned;
- claiming tool qualification from tool popularity;
- ignoring generated code, assembly, linker scripts, and startup;
- omitting timing, power, hardware, and interference evidence;
- failing to control the exact standard edition and amendments.

## Related Topics

- [Coding Practices](./coding-practices.md)
- [Static Analysis](./static-analysis.md)
- [Security](./security.md)
- [Formal Methods](./formal-methods.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)

## References

- [MISRA C resources](https://misra.org.uk/)
- [MISRA C:2012 Technical Corrigendum 2](https://misra.org.uk/app/uploads/2022/04/MISRA-C-2012-TC2.pdf)
- [MISRA C:2023 Addendum 2](https://misra.org.uk/app/uploads/2024/10/MISRA-C-2023-ADD2.pdf)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [IEC 61508 overview](https://www.iec.ch/functional-safety)
- [ISO 26262 functional safety](https://www.iso.org/publication/PUB200262.html)
- [DO-178C overview](https://www.rtca.org/products/do-178c/)
