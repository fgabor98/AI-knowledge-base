---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Security

Security in C is the disciplined control of data, authority, memory, time, and failure. Embedded devices add physical access, hostile peripherals, untrusted firmware updates, secrets in nonvolatile memory, fault injection, and long-lived deployments. Security is a system property, but C implementation choices can create or remove the vulnerabilities that make an attack possible.

## Learning Objectives

- identify trust boundaries, assets, threats, and security invariants;
- validate lengths, arithmetic, encodings, state, and resource use before dangerous operations;
- design privilege, parser, crypto, key, random, logging, and update boundaries;
- use compiler, linker, hardware, and runtime hardening appropriately;
- handle secrets and sensitive data across lifetime, reset, and failure paths;
- build security evidence into testing, review, dependencies, and incident response.

## Threat Modeling For Embedded C

Start with assets and abuse cases:

| Asset | Threat | Control and evidence |
| --- | --- | --- |
| firmware authenticity | modified image or rollback | signature verification, anti-rollback, negative tests |
| device identity/key | extraction or misuse | secure storage, access control, zeroization policy |
| safety function | malicious or malformed command | authorization, bounds, safe state, fault injection |
| availability | packet flood or resource exhaustion | quotas, bounded work, backpressure, watchdog policy |
| sensor/actuator state | spoofed or stale data | freshness, range, plausibility, secure channel |
| debug interface | unauthorized control | lifecycle lock, authentication, production configuration |
| telemetry | secret or personal-data leakage | classification, redaction, access control |

Record attacker capability: remote network, local user, physical access, malicious peripheral, compromised update server, or fault-injection equipment. A control is meaningful only if its assumptions and failure behavior are tested.

## Validate Before Use

Validation is more than checking a maximum. Check the arithmetic used to compute sizes and offsets before allocation or copying:

~~~c
#include <stdbool.h>
#include <stddef.h>

static bool checked_add_size(size_t left, size_t right, size_t *result)
{
    if (result == NULL || right > (size_t)-1 - left) {
        return false;
    }
    *result = left + right;
    return true;
}

static bool checked_region(size_t offset,
                           size_t length,
                           size_t capacity)
{
    size_t end;
    return checked_add_size(offset, length, &end) && end <= capacity;
}
~~~

Use `SIZE_MAX` from `<stdint.h>` or a project size limit in production rather than relying on `(size_t)-1` when clarity matters. Validate before converting from a wider or signed input type, before multiplying element counts, and before adding headers or alignment padding. Avoid “check after wrap” logic.

## Buffer, String, And Format Safety

- carry a pointer and length together;
- distinguish capacity from current length;
- reject truncation unless the API explicitly defines it;
- use bounded formatting with a clear return-value policy;
- never pass untrusted data as a format string;
- treat binary payloads as counted bytes, not C strings;
- validate terminators, encodings, and nested lengths;
- avoid APIs whose failure semantics are ambiguous for the product.

This is unsafe when `input` is attacker-controlled:

~~~c
printf(input);
~~~

Use a fixed format and an explicit representation policy:

~~~c
#include <stdio.h>

static void log_event(unsigned int event_code)
{
    printf("event=%u\\n", event_code);
}
~~~

Formatted output can still leak secrets, block, allocate, or exhaust a log channel. Security and real-time review must cover the complete path.

## Parser Security

Treat a parser as an adversarial state machine:

1. bound total input and each nested field;
2. validate version and flags before interpreting optional fields;
3. check arithmetic before pointer or offset calculation;
4. reject trailing bytes or define their meaning;
5. cap recursion, nesting, decompression, and loop work;
6. authenticate before acting on privileged content;
7. make malformed input a nonfatal, observable result;
8. preserve state when a partial operation fails.

A checksum detects accidental corruption; it is not authentication. Do not make authorization decisions from an unauthenticated header merely because the packet has a valid CRC.

## Integer And Resource Attacks

An attacker can target arithmetic, memory, time, and persistent state:

- count multiplication wraps to a small allocation;
- signed input becomes a large unsigned size;
- a timeout conversion creates an immediate or infinite wait;
- a retry counter wraps and disables backoff;
- a decompressor expands a small input without a budget;
- queues fill and block a high-priority task;
- flash wear is exhausted through repeated writes;
- a log flood fills storage or hides the security event.

Make limits explicit and test at one below, at, and one above every limit. Apply budgets across the whole operation, not only one function.

## Privilege And Trust Boundaries

Separate untrusted parsing from privileged effects. A useful architecture is:

```text
untrusted transport -> validation/parser -> normalized command
                                      -> authorization
                                      -> bounded privileged adapter
```

Use least privilege for processes, tasks, peripherals, DMA, debug access, and update agents. On an RTOS, task separation is not automatically a security boundary; on an MCU, an MPU or secure world may be required. Define which memory and registers each component may access and test attempted violations.

## Cryptographic API Use

Do not implement cryptographic primitives or protocols from memory. Use a maintained, reviewed library whose API defines:

- algorithm and mode;
- nonce/IV generation and uniqueness;
- authenticated versus unauthenticated data;
- key length, lifecycle, and storage;
- failure and padding behavior;
- constant-time requirements;
- hardware accelerator and DMA ownership;
- version and algorithm-agility policy.

Avoid comparing authentication tags with ordinary early-exit string comparison when timing can reveal information. Do not reuse nonces in schemes that forbid reuse. Authentication must cover the fields that influence authorization, version, destination, and rollback policy.

## Randomness And Keys

Use an approved operating-system, secure element, hardware TRNG, or cryptographic library source appropriate to the target. A timer, ADC noise sample, or `rand()` is not automatically a cryptographic random source. Define behavior when entropy is unavailable during boot.

Key handling includes generation, provisioning, storage, use, rotation, revocation, backup, zeroization, and recovery. C zeroization can be removed by optimization if written as an ordinary dead store; use a documented secure-memory API or compiler-supported mechanism and verify the target implementation.

## Updates And Supply Chain

Secure firmware update needs more than a signature:

- authentic image and metadata;
- version/rollback policy;
- correct target/product binding;
- atomic commit and power-loss recovery;
- key rotation and revocation;
- protected boot status and recovery path;
- dependency and toolchain provenance;
- vulnerability response and field diagnostics.

Verify signatures before executing privileged content and bind the signature to the exact image, configuration, hardware family, and version policy. Test interrupted downloads, corrupted metadata, invalid signatures, old images, storage-full conditions, and recovery failures.

## Hardening And Defense In Depth

Where supported and compatible with the platform, consider:

- stack protectors and stack guards;
- non-executable or read-only memory regions;
- MPU/MMU isolation;
- control-flow and return-address protection;
- position-independent or relocated code where appropriate;
- compiler warnings and fortification;
- safe default permissions and locked production debug;
- watchdog and rate-limit policy;
- secure boot and measured boot;
- tamper and fault detection.

Hardening does not repair an invalid pointer or missing authorization. Inspect the final ELF, memory permissions, startup configuration, and failure behavior.

## Sensitive Data And Logging

Classify data before logging or storing it. Redact keys, tokens, passwords, personal data, and raw authentication material. Logging code can cross privilege and timing boundaries, allocate, block, and expose memory after a fault. Use bounded records and deferred formatting for embedded fault paths.

When clearing sensitive memory, consider compiler elimination, copies made by libraries, caches, DMA, swap, crash dumps, and backup storage. The correct policy may be secure-element use and key invalidation rather than attempting to erase every CPU register.

## Exercises

1. Threat-model a bootloader, radio command, and debug-port attack surface.
2. Fuzz a nested-length parser with arithmetic and resource limits.
3. Add authorization after authentication and test every command/state combination.
4. Replace a weak random source and define entropy-unavailable behavior.
5. Interrupt updates at every write/commit stage and verify rollback.
6. Inspect a release image for hardening, debug, secrets, paths, and unexpected symbols.
7. Design a key lifecycle and incident-response plan for a long-lived device.

## Common Mistakes

- treating a checksum as authentication;
- checking only maximum lengths and ignoring arithmetic overflow;
- using untrusted strings as format strings;
- authenticating data after acting on it;
- using `rand`, time, or predictable IDs for key material;
- assuming an RTOS task boundary is a privilege boundary;
- logging secrets or accepting debug defaults in production;
- implementing crypto primitives or zeroization ad hoc;
- omitting rollback, power-loss, and revocation from update design;
- assuming compiler hardening replaces validation and authorization.

## Related Topics

- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [Common Embedded Libraries](../standard-library-and-ecosystem/common-embedded-libraries.md)
- [Bootloaders And Firmware Images](../embedded-c-and-hardware/bootloaders-and-firmware-images.md)
- [Safety Standards And MISRA](./safety-standards-and-misra.md)

## References

- [NIST Secure Software Development Framework SP 800-218](https://csrc.nist.gov/pubs/sp/800/218/final)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [MITRE CWE-190: Integer Overflow or Wraparound](https://cwe.mitre.org/data/definitions/190.html)
- [OWASP C-based development guidance](https://owasp.org/www-community/vulnerabilities/)
