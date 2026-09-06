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
