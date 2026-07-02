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

Kernel code runs with the highest privilege in the system. If an attacker can load arbitrary modules or exploit easy memory-corruption primitives, normal userspace security boundaries are already lost.

Hardening is not one option. It is a product policy that combines:

- build-time configuration
- boot-chain trust
- module loading policy
- key ownership
- runtime diagnostics policy
- update workflow
- incident response and service exceptions

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

```text
development:
  fast iteration and diagnostic access

service:
  controlled exception path

production:
  enforce trust and reduce exploit surface
```

If the product needs strict module signing, field service must have a signed-module workflow. If field service needs unsigned modules, production must explicitly allow that risk or provide a separate service image.

## Module Signing Model

Module signing attaches a cryptographic signature to a `.ko` file. The kernel checks the signature when loading the module.

Important pieces:

| Piece | Purpose |
| --- | --- |
| private key | signs modules during build or release |
| public key/certificate | built into kernel or trusted keyring |
| module signature | appended to module file |
| enforcement mode | decides whether unsigned/untrusted modules load |
| module loader | `insmod`, `modprobe`, or kernel module loading path |

The trust decision is made by the kernel, not by userspace tooling.

## Important Config Symbols

Common options:

```text
CONFIG_MODULE_SIG
CONFIG_MODULE_SIG_FORCE
CONFIG_MODULE_SIG_ALL
CONFIG_MODULE_SIG_KEY
CONFIG_SYSTEM_TRUSTED_KEYS
CONFIG_SYSTEM_REVOCATION_KEYS
CONFIG_MODULE_COMPRESS
CONFIG_MODULE_UNLOAD
CONFIG_MODVERSIONS
```

Exact availability depends on kernel version and architecture.

Policy examples:

```text
development:
  CONFIG_MODULE_SIG=y
  # CONFIG_MODULE_SIG_FORCE is not set

production:
  CONFIG_MODULE_SIG=y
  CONFIG_MODULE_SIG_FORCE=y
  CONFIG_MODULE_SIG_ALL=y
```

Always verify behavior by attempting to load signed, unsigned, and wrongly signed modules.

## Permissive Versus Enforcing Signing

Permissive signing:

```text
unsigned or unknown-key modules may load
kernel may be tainted
good for development and transition periods
not strong protection
```

Enforcing signing:

```text
only validly signed modules from trusted keys load
field service must use trusted signing path
module load failures become operational incidents
```

Do not enable enforcement without planning:

- key custody
- signing automation
- key rotation
- revocation
- service modules
- rollback
- release archive

## Signing Key Ownership

Key policy should answer:

```text
Who can sign production modules?
Where is the private key stored?
Is signing done in CI, HSM, or offline release process?
Can developers sign debug modules?
How are service modules signed?
How are keys rotated?
How are compromised keys revoked?
Which public keys are built into which products?
```

Never leave production private keys in the kernel source tree or general build workspace.

## Secure Boot And Lockdown

On platforms with secure boot, the boot chain can affect kernel trust policy.

Common interactions:

- firmware verifies bootloader
- bootloader verifies kernel
- kernel trusts platform keys
- lockdown mode restricts some kernel interfaces
- module signing policy rejects untrusted modules

Lockdown can restrict access to kernel features that expose or modify kernel memory, depending on mode and distribution policy.

Review:

```sh
cat /sys/kernel/security/lockdown
dmesg | grep -i lockdown
dmesg | grep -i 'module verification'
```

Availability and exact behavior depend on kernel configuration and platform.

## Hardening Options

Hardening options vary by architecture and kernel version, but common areas include:

| Area | Example Symbols |
| --- | --- |
| stack protection | `CONFIG_STACKPROTECTOR`, `CONFIG_STACKPROTECTOR_STRONG` |
| usercopy checks | `CONFIG_HARDENED_USERCOPY` |
| read-only/executable permissions | `CONFIG_STRICT_KERNEL_RWX`, `CONFIG_STRICT_MODULE_RWX` |
| address randomization | `CONFIG_RANDOMIZE_BASE` |
| allocator hardening | `CONFIG_SLAB_FREELIST_RANDOM`, `CONFIG_SLAB_FREELIST_HARDENED` |
| format/string hardening | `CONFIG_FORTIFY_SOURCE` |
| BPF/JIT policy | BPF and JIT hardening symbols |
| devmem restriction | `CONFIG_STRICT_DEVMEM`, `CONFIG_IO_STRICT_DEVMEM` |
| debug exposure | `CONFIG_DEBUG_FS`, kptr restrictions, perf restrictions |

Treat this table as a review prompt, not a universal checklist. The final answer depends on architecture, threat model, performance, and support needs.

## Attack Surface Reduction

Reducing attack surface can mean:

- disabling unused filesystems
- disabling unused network protocols
- disabling unused USB gadget/function drivers
- disabling unused debug filesystems
- disabling module loading after boot when possible
- restricting BPF/perf access
- restricting `/dev/mem`
- using LSM policy
- limiting exported device nodes

Do not enable large feature groups "just in case" in production.

## Diagnostics Exception Path

Hardening can block useful support tools. That is expected; plan an exception path.

Examples:

| Need | Safer Policy |
| --- | --- |
| load field diagnostic driver | signed service module |
| inspect internal state | signed debug build or service image |
| use debugfs | service image with access controls |
| collect crash evidence | pstore/ramoops and archived symbols |
| run perf/ftrace | controlled diagnostic profile |

Avoid undocumented one-off boot arguments that disable protections in the field.

## Module Loading Failure Triage

When a module fails to load:

```sh
dmesg | tail -100
modinfo demo.ko
uname -r
cat /proc/sys/kernel/tainted
cat /proc/keys
```

Check:

- module was built for this exact kernel release
- dependencies are present
- signature exists
- signer is trusted
- module was not stripped after signing
- lockdown mode allows the operation
- vermagic and modversions match

## Release Gate

For production releases, verify:

```text
final .config has required signing/hardening symbols
unsigned test module is rejected when enforcement is expected
valid signed module loads when allowed
wrong-key module is rejected
private signing key is not in build artifact archive
public certificate identity is archived
debugfs policy matches product profile
lockdown state is documented
service exception path is tested
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| unsigned module loads in production | enforcement not enabled | final `.config`, command line |
| signed module rejected | key not trusted or module altered after signing | dmesg, keyring, file hash |
| service cannot load diagnostic module | no service signing workflow | key ownership |
| secure boot blocks debug workflow | lockdown/signing not planned | service image policy |
| product exposes kernel internals | debugfs/perf/devmem policy too permissive | mounted filesystems and sysctls |
| module works in lab, fails in release | different kernel release or vermagic | `modinfo`, `uname -r` |

## Practice Exercises

### Exercise 1: Signing Matrix

Build and test:

```text
unsigned module
module signed by development key
module signed by production key
module modified after signing
module built for wrong kernel release
```

Record expected load behavior under development and production profiles.

### Exercise 2: Hardening Audit

Create a required/forbidden symbol list for production hardening. Run it against the final `.config`.

### Exercise 3: Exception Path

Design a support process for one diagnostic module:

```text
who builds it
who signs it
how it is delivered
how it is revoked
what logs prove it was used
```

## Debugging Checklist

- Check key enrollment and trust chain.
- Check lockdown mode.
- Check module load errors in `dmesg`.
- Confirm diagnostic tooling still has an approved path.
- Check final `.config`, not only fragments.
- Check whether enforcement comes from config or command line.
- Check that modules are not stripped after signing.
- Check release artifacts for accidental private key leakage.
- Test service and rollback workflows.

## Related Topics

- [Built-In Vs Module Policy](built-in-vs-module-policy.md)
- [Config Review Workflow](config-review-workflow.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [Debug Vs Production Configs](debug-vs-production-configs.md)

## Official References

- [Kernel module signing facility](https://docs.kernel.org/admin-guide/module-signing.html)
- [Linux Security Module Usage](https://docs.kernel.org/admin-guide/LSM/index.html)
- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
