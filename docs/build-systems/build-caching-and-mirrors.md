---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Build Caching and Mirrors

## What Problem Does This Solve?

Embedded Linux builds are slow and network-heavy. Caches and mirrors reduce rebuild time, make CI more reliable, and help reproduce releases even when upstream servers change or disappear.

This topic covers the main cache types used in C/C++ application builds, Yocto, Buildroot, and CI systems.

## Core Concepts

- compiler cache
- download cache
- source mirror
- artifact cache
- Yocto shared state
- Buildroot download directory
- CI cache
- cache key
- cache invalidation
- offline build
- mirror provenance

## Mental Model

Not all caches solve the same problem:

```text
download cache  -> avoid re-fetching source archives
compiler cache  -> avoid recompiling identical translation units
sstate cache    -> avoid rerunning Yocto tasks
artifact cache  -> reuse completed build outputs
mirror          -> provide controlled source availability
```

Speed is useful, but correctness comes first. A bad cache can hide broken dependencies or serve stale outputs.

## Syntax / API / Mechanism

Use `ccache`:

```sh
ccache gcc -Wall -Wextra -c main.c -o main.o
ccache -s
```

Make:

```sh
make CC="ccache gcc"
```

Buildroot download cache:

```sh
make BR2_DL_DIR=/data/buildroot-dl
```

Yocto common variables:

```conf
DL_DIR ?= "/data/yocto/downloads"
SSTATE_DIR ?= "/data/yocto/sstate-cache"
```

CI cache key examples:

```text
toolchain version
lockfile or manifest hash
Yocto branch and machine
compiler flags
source checksum
```

## Minimal Example

Local C project with `ccache`:

```sh
export CCACHE_DIR="$HOME/.cache/ccache"
make clean
make CC="ccache gcc"
ccache -s
```

If the inputs are unchanged, later builds can reuse cached compilation results.

## Real-World Example

A Yocto CI runner may mount persistent directories:

```text
/cache/yocto/downloads
/cache/yocto/sstate
/artifacts/images
```

The build configuration points to them:

```conf
DL_DIR = "/cache/yocto/downloads"
SSTATE_DIR = "/cache/yocto/sstate"
```

This reduces repeated fetches and rebuilds across CI jobs, but the cache must be scoped by Yocto release, machine, distro, and toolchain compatibility.

## Common Scenarios

### Download Cache

Use a shared download directory to avoid repeatedly fetching tarballs and Git mirrors.

Risk: if source checksums are not enforced, a changed upstream file can poison reproducibility.

### Source Mirror

A source mirror gives the organization control over source availability:

```text
upstream source -> internal mirror -> builds
```

This helps with offline builds and release rebuilds.

### ccache

`ccache` helps most with repeated local C/C++ compilation. It is less useful if every build changes paths, generated headers, compiler flags, or timestamps in ways that break cache keys.

### Yocto sstate

Yocto shared state cache stores task outputs. It can dramatically reduce build time, but cache reuse depends on task signatures. If metadata, configuration, or dependencies change, tasks are rebuilt.

### CI Cache Invalidation

Bad cache keys cause two failures:

- too broad: stale or wrong outputs reused
- too narrow: no useful cache hits

Cache keys should include the inputs that affect outputs.

### Offline Builds

Offline builds require more than a compiler cache. They need all source downloads, mirrors, layers, toolchains, and metadata available without network access.

## Common Mistakes

- Treating caches as a substitute for pinned sources.
- Sharing one cache across incompatible toolchains or distro releases.
- Ignoring cache invalidation rules.
- Letting CI rely on network access for release builds.
- Caching build directories that contain host-specific absolute paths.
- Assuming `ccache` helps linker-heavy or image-generation-heavy builds.
- Deleting caches manually without understanding CI impact.

## Debugging Checklist

- Disable the cache and confirm the build still works.
- Print cache locations in CI logs.
- Check cache hit/miss statistics.
- Check toolchain and metadata versions in cache keys.
- Verify source checksums.
- Rebuild from an empty cache for release validation.
- Test offline or mirror-only builds if release reproducibility matters.
- Separate local developer caches from release build provenance.

## Related Topics

- [Source Fetching and Patch Management](source-fetching-and-patch-management.md)
- [Yocto and OpenEmbedded Roadmap](yocto-openembedded-roadmap.md)
- [TI Processor SDK Roadmap](ti-processor-sdk-roadmap.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

## References

- ccache documentation
- Yocto Project Reference Manual
- Buildroot manual
- Git documentation
