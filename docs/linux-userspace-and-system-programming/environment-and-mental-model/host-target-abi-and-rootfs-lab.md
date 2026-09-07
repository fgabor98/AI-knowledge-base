---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Host, Target, ABI, And Rootfs Lab

## What problem does this solve?

The host is where you build and investigate. The target is where the product must
run. They may use different CPU architectures, ABIs, libcs, kernels, rootfs layouts,
credentials, and devices. “It works on my machine” is therefore an observation about
one environment, not a deployment guarantee.

## Host and target are roles

| Role | Typical contents | What it is good for |
| --- | --- | --- |
| Build host | Full compiler, headers, debugger, tracing tools, source tree | Fast iteration, unit tests, static analysis, artifact inspection |
| Target | Product kernel, rootfs, service manager, constrained resources, real devices | Integration, timing, permissions, driver, power, boot, and deployment evidence |
| Target-like lab | Container, chroot, emulator, sysroot, VM, or board image | Reproducing selected ABI, rootfs, or service assumptions cheaply |

A container shares the host kernel, so it is useful for rootfs and process isolation
but cannot prove target-kernel behavior. An emulator can prove more architecture and
boot behavior but may not model the board’s electrical timing. A sysroot supplies
headers and libraries for linking; it is not itself a running target.

## The ABI stack

An executable must agree with more than the instruction set:

```text
CPU architecture / ELF machine
        |
calling convention, register use, alignment, endianness
        |
data-model widths: ILP32, LP64, ...
        |
libc ABI and symbol versions
        |
ELF interpreter and shared-library search paths
        |
kernel system-call ABI and UAPI structure layout
```

Examples of mismatches include:

- an x86-64 binary copied to an AArch64 target;
- a dynamically linked binary whose requested interpreter is absent;
- a glibc-linked program deployed to a musl rootfs without a compatible ABI layer;
- a 64-bit userspace passing a structure to a 32-bit-compatible UAPI incorrectly;
- a program relying on a newer libc symbol than the target exports;
- a compiler using a different `time_t`, `off_t`, alignment, or packing convention;
- a UAPI structure containing pointers or implicit padding across 32/64-bit builds.

Do not “solve” an ABI mismatch by applying `packed` to every structure. Packing can
create unaligned accesses and changes the contract. Use fixed-width fields where the
UAPI requires them, explicit reserved fields, and the subsystem’s compatibility
rules.

## The rootfs is part of the program

At runtime, the program depends on:

- the ELF interpreter, often under `/lib` or `/lib64`;
- every required shared object and its transitive dependencies;
- name-service configuration if it resolves users, hosts, or services;
- `/etc` configuration and certificates when relevant;
- `/dev`, `/proc`, `/sys`, `/run`, and other expected mounts;
- service users, groups, capabilities, limits, and writable directories;
- clock, locale, timezone, and entropy assumptions;
- firmware files and device nodes;
- an init/supervisor contract for startup, signals, logs, and restart.

An absolute path present in the source tree or host root does not imply that it exists
in the target’s mount namespace. A path may also exist but be the wrong object: a
regular file instead of a device node, a symlink into an absent mount, or a read-only
directory where the service expects runtime state.

## Inspect the artifact before deploying

On the build host:

```sh
file ./app
readelf -hW ./app
readelf -lW ./app | grep 'Requesting program interpreter'
readelf -dW ./app
readelf --version-info ./app
ldd ./app                 # inspection only; do not execute untrusted binaries
```

Prefer `readelf` and a target-rootfs-aware loader inspection in release diagnostics.
`ldd` is commonly a wrapper around the loader and can execute code for unusual
objects; never use it as a security boundary for an untrusted binary.

On the target, collect:

```sh
uname -a
cat /etc/os-release 2>/dev/null || true
getconf LONG_BIT 2>/dev/null || true
getconf GNU_LIBC_VERSION 2>/dev/null || true
file /usr/bin/app
readelf -lW /usr/bin/app | grep 'Requesting program interpreter'
```

If `readelf` is unavailable on a minimal target, inspect the artifact on the host and
collect `/proc/<pid>/maps`, `/proc/<pid>/exe`, loader diagnostics, and service logs.

## A repeatable lab

### 1. Define the matrix

Record this for every test result:

```text
source revision:
compiler and binutils:
build flags:
target triple:
ELF class / machine:
libc and version:
kernel release / configuration:
rootfs image or package revision:
init / service manager:
UID / GIDs / capabilities:
mount and network namespaces:
device / board revision / firmware:
environment and configuration:
```

### 2. Build a diagnostic artifact

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-environment-and-mental-model.c \
    -o /tmp/environment-probe
```

Keep symbols in a separate archive or debug package if the target image is size
constrained. A stripped binary is suitable for production only when the matching
symbols and exact build identity are retained elsewhere.

### 3. Deploy without changing the proof

Copy the exact artifact and record its checksum:

```sh
sha256sum /tmp/environment-probe
# On a target with SSH; substitute the product transport as needed:
scp /tmp/environment-probe user@target:/tmp/environment-probe
ssh user@target 'sha256sum /tmp/environment-probe && /tmp/environment-probe'
```

Do not rebuild on the target unless the experiment is specifically about a native
toolchain. Do not copy host libraries into a target directory as an ad hoc fix; that
can create an untracked hybrid rootfs.

### 4. Compare execution evidence

```sh
strace -f -ttT -s 256 -o /tmp/environment-probe.strace /tmp/environment-probe
cat /proc/self/mountinfo
cat /proc/self/status
readlink /proc/self/exe
readlink /proc/self/ns/mnt
```

For a service, collect the same information from its actual PID and service context.
`/proc/self` means the process reading it; reading `/proc` from a shell does not
necessarily describe the service’s namespace or credentials.

### 5. Record a difference as a hypothesis

Use this format:

```text
Observation: open("/dev/my_sensor") -> ENOENT on target, succeeds in fixture
Changed:     target has no /dev entry; host has a udev-created node
Hypothesis:  device driver is not bound, or devtmpfs/udev policy is incomplete
Next test:   inspect /sys, kernel log, mounts, and device discovery policy
Resolution:  [fill from evidence]
```

This prevents “target is weird” from becoming an untestable explanation.

## Common runtime failure signatures

| Symptom | High-value checks |
| --- | --- |
| `No such file or directory` when the file exists | ELF interpreter, a needed loader path, symlink target, or a path in a different namespace |
| `Exec format error` | ELF class, machine, ABI, or script interpreter |
| `error while loading shared libraries` | `DT_NEEDED`, loader search paths, RPATH/RUNPATH, package contents, symbol versions |
| `Permission denied` | UID/GIDs, mode/ACL, capabilities, LSM, mount flags, service sandbox |
| `Function not implemented` / `ENOSYS` | Kernel or libc support, syscall availability, architecture compatibility |
| works interactively but not as a service | Environment, current directory, credentials, limits, namespaces, readiness, or fd inheritance |
| works after SSH but not at boot | Mount/order/network/device readiness, service dependencies, or missing environment |
| works on one board only | Board revision, firmware, calibration, wiring, device tree, timing, or persistent state |

## Minimal lab deliverable

Submit a small evidence bundle containing:

- the probe output;
- `file`, `readelf -h`, `readelf -l`, and dependency output;
- host and target `uname`, libc, and rootfs identifiers;
- the relevant `strace` excerpt;
- `/proc` identity and namespace observations;
- one successful run and at least three intentionally induced failures;
- a conclusion naming the first boundary that differed.

Useful induced failures include removing execute permission from a copy, using a
missing library in a test rootfs, running as a user without access to a fixture, and
opening an absent device path. Restore or discard temporary lab artifacts afterward.

## Common mistakes

- Treating the sysroot as a complete target environment.
- Copying libraries from the host until the program starts.
- Using `ldd` on untrusted objects.
- Recording only the source revision, not the generated binary and rootfs revision.
- Testing as root and assuming a service user has the same access.
- Assuming a container has the target kernel or devices.
- Stripping symbols without preserving an exact symbol artifact.
- Comparing logs without comparing clock source, timezone, and timestamp domain.

## Debugging checklist

- Verify artifact checksum and architecture before investigating application logic.
- Verify interpreter, `DT_NEEDED`, symbol versions, and loader search paths.
- Verify target kernel, libc, rootfs package set, mounts, and service identity.
- Verify current working directory, environment, umask, resource limits, and namespace.
- Verify device discovery, permissions, firmware, and readiness separately.
- Preserve the failing artifact, logs, `/proc` snapshot, and configuration used.

## Related topics

- [Stage 0: Environment And Mental Model](index.md)
- [Target Triples, Sysroots, And ABI](../../build-systems/target-triples-and-sysroots.md)
- [ELF Executables And Dynamic Linking](../linux-runtime-filesystem-and-rootfs/elf-executables-and-dynamic-linking.md)
- [Native Linux Userspace Builds](../../build-systems/native-linux-userspace-builds.md)
- [Target Boot And Device Integration Tests](../testing-and-verification/target-boot-and-device-integration-tests.md)

## References

- [`file(1)`](https://man7.org/linux/man-pages/man1/file.1.html)
- [`readelf(1)`](https://sourceware.org/binutils/docs/binutils/readelf.html)
- [`proc(5)`](https://man7.org/linux/man-pages/man5/proc.5.html)
- [Linux namespaces overview](https://man7.org/linux/man-pages/man7/namespaces.7.html)
- [Linux dynamic linking documentation](https://man7.org/linux/man-pages/man8/ld.so.8.html)
