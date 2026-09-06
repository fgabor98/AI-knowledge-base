# Target Triples, Sysroots, And ABI

## Build roles

The **build** machine executes the compiler. The **host** is where a generated tool runs; for a normal application this may be the target. The **target** is where a compiler-generated tool emits code. Confusing these terms causes native tools to be linked against target libraries or target binaries to be run during the build.

A target triple captures architecture, vendor, operating system, and ABI conventions, but it is not the complete contract. Record CPU baseline and extensions, endianness, floating-point ABI, data model, threading model, libc, loader, kernel minimum, and C++ ABI where applicable.

## Sysroot discipline

A sysroot is the target's view of headers, startup objects, libraries, and metadata. Compile and link with the intended sysroot; ensure `pkg-config` uses target `.pc` files and does not return host include or library paths. Keep generated tools and target libraries in separate prefixes.

Do not “fix” a missing target dependency by adding `/usr/include` or `/usr/lib` from the build host. That may produce a binary that links but fails on the target or silently uses the wrong structure layout.

## ABI checks

Check `sizeof` assumptions, alignment, signedness, time/off_t widths, structure packing, symbol versions, exception/RTTI policy, and syscall calling conventions. Compile a small ABI probe as part of the toolchain validation and inspect the final ELF with `file` and `readelf`. The ABI is a compatibility promise across every library boundary, not merely a compiler flag.
