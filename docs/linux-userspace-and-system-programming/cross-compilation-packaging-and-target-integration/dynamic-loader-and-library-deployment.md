---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Dynamic Loader And Library Deployment

An ELF executable names an interpreter and a set of shared objects. Deployment must provide the correct interpreter path, library search paths, symbol versions, and transitive dependencies.

Inspect the result:

```sh
readelf -l app | grep interpreter
readelf -d app
readelf --version-info app
```

Prefer the target's standard library directories and package manager over ad hoc copying. If private libraries are necessary, define ownership, update compatibility, search-path behavior, and security permissions. Understand the difference between RPATH and RUNPATH, and avoid writable library directories in a privileged service's search path.

Static linking can simplify deployment but changes update, NSS, resolver, licensing, memory, and security-update tradeoffs. Dynamic linking reduces duplication but requires an explicit dependency closure. Either choice needs a target smoke test that executes the packaged binary in a minimal root filesystem.

Keep loader diagnostics separate from application logs. A failure before `main` needs the executable checksum, interpreter, library paths, loader output, and target package database—not a backtrace from application code.

## Work through a dependency closure

Suppose app needs `libdevice.so.2`, which needs `libtransport.so.1`.
Shipping app and libdevice is incomplete. Preserve the SONAME files and
symlink layout expected by the target loader, inspect each object's needs,
and account for modules loaded at runtime.

A private RUNPATH such as `$ORIGIN/../lib` is interpreted by a supporting
loader relative to the containing object. The shell must not expand
`$ORIGIN` when passing the value to a build command. On glibc, RUNPATH
is used for direct dependencies; libdevice may need its own RUNPATH for
libtransport. RPATH has different inheritance/search behavior.
See [ld.so(8)](https://man7.org/linux/man-pages/man8/ld.so.8.html).

Use loader diagnostics on the actual compatible target or a controlled target
runtime. Running the host's loader against a foreign ELF is not equivalent.
For a trusted glibc application, `LD_DEBUG=libs,versions` can expose search
and version decisions. It does not apply identically to other libcs.

## Test the runtime, including optional paths

The minimal-rootfs smoke test should exercise name resolution, certificates,
locale/timezone dependencies, plugins and crash reporting when used. A binary
can reach `main` and still fail later when an optional module is first loaded.

Check deployment upgrades as a set. Updating an application while its old
private library remains can fail differently from updating the library alone.
Version private directories or deploy through an atomic image/package contract
when mixed generations are unacceptable.

## Related topics

- [Stage 15 overview](index.md)
- [ELF and loader diagnosis](../diagnostics-debugging-and-performance/elf-abi-and-loader-diagnostics.md)
