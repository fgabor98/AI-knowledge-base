---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Secure Userspace Input And Files

Security failures often occur at the boundary between bytes and meaning. Treat command-line arguments, environment variables, configuration files, device data, IPC messages, network frames, and filenames as hostile until validated.

## Parse before acting

Define a grammar and resource limits before writing the parser. Check lengths before arithmetic, reject integer overflow and impossible combinations, enforce maximum nesting and item counts, and distinguish malformed input from an unavailable dependency. Never use an unbounded `%s`, implicit shell evaluation, or a parser whose error path has different authorization rules.

Validate semantics after syntax: enum values, ranges, units, encoding, referenced objects, and allowed transitions. Canonicalize only when the canonical form is well-defined; validation of one representation must not be bypassed through aliases, alternate encodings, or duplicate keys.

## Safe path handling

Avoid constructing a privileged pathname from untrusted text and then checking it with `access()`. Prefer a trusted directory descriptor plus `openat()`/`openat2()`, explicit flags such as `O_NOFOLLOW`, and resolution constraints appropriate to the policy. Consider symlink, mount-point, bind-mount, rename, and concurrent replacement attacks.

For temporary files, use `mkstemp()` or an equivalent exclusive-creation API, set restrictive permissions, and keep the descriptor open. Do not place sensitive temporary data in a predictable shared directory.

## Secrets and inherited state

Keep secrets out of argv, logs, crash dumps, world-readable environment snapshots, and unnecessarily inherited descriptors. Clear or constrain the environment for privileged programs; dynamic loaders and language runtimes may interpret environment variables before application code runs. Mark secrets with a lifecycle: acquire, use, rotate, revoke, and erase as far as the platform permits.

## Review questions

- What is the maximum input and allocation size?
- Can a malformed value trigger a privileged fallback?
- Is the file opened relative to a trusted object, and can it be swapped?
- Are error messages safe to expose and useful to operators?
- Are parsing, authorization, and side effects separated so they can be tested independently?

Security is a property of the whole data flow. A correct parser does not help if a later conversion truncates a value, and a safe open does not help if the contents are interpreted as shell syntax.

## Safe arithmetic before safe I/O

For a buffer of `capacity` bytes and untrusted `offset` and `length`,
validate `offset <= capacity && length <= capacity - offset` before deriving
a pointer. Checking `offset + length <= capacity` can accept an overflowing
sum. Apply the same discipline to element count times element size and header
plus payload size.

Separate decoding, semantic validation, authorization, and effects. For an
actuator command, a valid integer is insufficient: validate units and range,
authorize this client for this actuator, check current device state, and only
then submit the effect. A malformed request must not partially update the
active configuration.

## Path confinement example

Open a trusted directory once. For a relative untrusted name, Linux
`openat2` can apply `RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS` with deliberate
open flags. `O_NOFOLLOW` alone protects only the final component in a
normal open; it does not prohibit intermediate symlinks. The actual choice
may permit some symlinks, but it must state that policy.

`RESOLVE_BENEATH` confines traversal; it does not validate the resulting
file type, owner, content, or hard-link provenance. Inspect the opened FD
and use trusted directory ownership. On a target without `openat2`, do not
silently weaken the guarantee by falling back to an unrestricted open.
See [openat2(2)](https://man7.org/linux/man-pages/man2/openat2.2.html).

Tests should include `..`, absolute names, intermediate/final symlinks,
concurrent rename, oversized records, embedded NUL bytes, duplicate keys,
and numeric overflow. Check that failures leave active state and resource
counts unchanged.

## Related topics

- [Stage 12 overview](index.md)
- [Service sandboxing](../services-init-and-systemd/service-sandboxing-and-resource-controls.md)
