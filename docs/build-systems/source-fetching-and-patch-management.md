---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Source Fetching and Patch Management

## What Problem Does This Solve?

Complete embedded Linux systems build many upstream, vendor, and product-specific components. You need a controlled way to fetch source code, pin versions, apply patches, and reproduce the same source tree later.

This topic covers source provenance and patch workflows before they become Yocto recipes, Buildroot packages, vendor BSP forks, or release manifests.

## Core Concepts

- source archive
- Git revision
- tag vs commit hash
- checksum
- source mirror
- vendored dependency
- Git submodule
- patch series
- quilt-style patches
- downstream patch
- upstreaming
- source provenance

## Mental Model

Every built artifact should answer:

```text
Which source?
Which exact version?
Which patches?
Which configuration?
Which toolchain?
```

If you cannot answer the first three, you do not have a reproducible build.

## Syntax / API / Mechanism

Fetch a fixed tarball:

```sh
curl -LO https://example.com/libfoo-1.2.3.tar.xz
sha256sum libfoo-1.2.3.tar.xz
```

Fetch a fixed Git commit:

```sh
git clone https://example.com/libfoo.git
cd libfoo
git checkout 0123456789abcdef
```

Apply a patch:

```sh
patch -p1 < ../0001-fix-build.patch
```

Generate a patch:

```sh
git format-patch -1 HEAD
```

Apply a patch series:

```sh
git am patches/*.patch
```

## Minimal Example

```sh
git clone https://example.com/app.git
cd app
git checkout v1.4.2
git am ../patches/*.patch
```

Record:

```text
upstream: https://example.com/app.git
version: v1.4.2
patches: patches/*.patch
```

## Real-World Example

A product may use:

```text
upstream U-Boot
TI U-Boot branch
product board patches
factory boot environment patch
secure boot configuration patch
```

Good patch management keeps these layers visible:

```text
vendor baseline
-> board enablement patches
-> product policy patches
-> temporary workaround patches
```

Temporary workaround patches should be named and tracked so they can be removed after upstream or vendor fixes land.

## Common Scenarios

### Tarball With Checksum

Buildroot and Yocto both support fixed source archives with checksums. This is preferable to floating downloads.

Good:

```text
libfoo-1.2.3.tar.xz + sha256
```

Risky:

```text
download latest from main branch
```

### Git Tag vs Commit Hash

Tags are readable, but tags can theoretically move unless protected. Commit hashes are precise. For release builds, record the exact commit even when using a tag.

### Git Submodules

Submodules can be useful, but they add workflow complexity:

- recursive checkout required
- detached HEAD states
- CI setup must initialize them
- version updates span multiple repositories
- source archiving is harder

Use them deliberately, not as a default dependency manager.

### Yocto Patch Flow

Yocto recipes often use:

```bitbake
SRC_URI += "file://0001-fix-build.patch"
```

or a `.bbappend` in a product layer. Keep patches in the layer that owns the product change.

### Buildroot Patch Flow

Buildroot packages can apply package patches from package directories or external trees. Keep product-specific patches in `BR2_EXTERNAL` when possible.

### Vendor BSP Forks

Vendor trees often contain many downstream patches. Preserve the vendor baseline and keep product changes separate. Do not mix product changes directly into a vendor import without a way to rebase or audit them.

## Common Mistakes

- Building from an unpinned branch.
- Depending on a source URL without a checksum.
- Losing track of local patches.
- Editing downloaded source manually inside a build directory.
- Mixing vendor BSP changes and product changes in one branch with no structure.
- Carrying patches without descriptions.
- Keeping dead patches after upgrading upstream.
- Treating submodules as invisible implementation details.

## Debugging Checklist

- Identify the exact source URL and commit or tarball checksum.
- List every applied patch in order.
- Rebuild from a clean source checkout.
- Check whether a patch still applies after version upgrades.
- Check whether a patch is product-specific, vendor-specific, or upstreamable.
- Verify CI fetches the same source as local builds.
- Archive source inputs for release builds.

## Related Topics

- [Build Caching and Mirrors](build-caching-and-mirrors.md)
- [Yocto and OpenEmbedded](advanced/yocto-openembedded/index.md)
- [TI Processor SDK Linux](advanced/ti-processor-sdk/index.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

## References

- Git documentation
- GNU patch manual
- Yocto Project Development Tasks Manual
- Buildroot manual
