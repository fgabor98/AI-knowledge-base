---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# ELF, ABI, And Loader Diagnostics

When a program fails before `main`, inspect the ELF and dynamic loader before debugging application logic.

```sh
file ./app
readelf -h ./app
readelf -l ./app | grep 'Requesting program interpreter'
readelf -d ./app
readelf --version-info ./app
```

Check architecture, endianness, ABI, ELF class, interpreter path, needed libraries, RPATH/RUNPATH, symbol versions, and executable stack/text-relocation properties. A binary built for the wrong target can produce `Exec format error`; a missing interpreter or library commonly produces `No such file or directory` even when the application path exists.

`ldd` may execute code for untrusted binaries on some systems; prefer static inspection with `readelf` for unknown artifacts. Use loader-assisted diagnostics only for trusted binaries in a controlled environment. `LD_DEBUG` is verbose and may expose paths or secrets.

Compare the target's loader and library set with the build sysroot. ABI compatibility includes calling convention, data-model widths, structure layout, symbol versions, thread-local storage, and kernel/libc assumptions—not just CPU architecture. Record the exact package provenance so a loader failure is reproducible.

## Distinguish exec failure from loader failure

| Failure point | Typical evidence | Next check |
| --- | --- | --- |
| Kernel rejects format | `execve` returns `ENOEXEC` | ELF machine/class, script shebang |
| Interpreter path absent | `execve` returns `ENOENT` although executable exists | `PT_INTERP` within target root |
| Loader cannot find a DSO | Loader message after exec | Needed SONAME, search path, package contents |
| Required version absent | “version ... not found” | Consumer version requirements and provider exports |
| Relocation unresolved | “undefined symbol” | Correct DSO version and load/visibility policy |
| Constructors/startup crash | Core/trace before `main` | Static initialization and matching symbols |

For `app -> libsensor.so -> libtransport.so`, listing app's `DT_NEEDED`
entries does not establish the whole dependency closure. Inspect each DSO.
Libraries loaded through `dlopen` may be absent from those entries entirely.
Plugins, resolver modules and optional backends need runtime-path tests.

On glibc, RUNPATH applies to direct dependencies, with each dependent object's
own search policy relevant afterward. Secure-execution mode restricts loader
environment variables. Do not assume a shell's `LD_LIBRARY_PATH` behavior
will match a privileged or supervised launch.
See [ld.so(8)](https://man7.org/linux/man-pages/man8/ld.so.8.html).

Preserve exact stderr and exec result. A shell's “not found,” a service manager's
exec failure, and a dynamic loader's missing-library report point to different
stages even when users describe all three as “the binary cannot start.”

## Related topics

- [Stage 14 overview](index.md)
- [Testing and verification](../testing-and-verification/index.md)
