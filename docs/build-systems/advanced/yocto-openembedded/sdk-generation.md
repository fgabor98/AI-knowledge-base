---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# SDK Generation

## What Problem Does This Solve?

Yocto SDKs let application developers build target software outside the full BitBake build while using a matching cross-toolchain, target sysroot, compiler flags, and development files. This avoids distributing arbitrary vendor toolchains and manually assembled sysroots.

## Core Concepts

- standard SDK
- extensible SDK
- cross-toolchain
- target sysroot
- host sysroot
- environment setup script
- SDK package population
- `populate_sdk`
- `populate_sdk_ext`

## Mental Model

```text
image and distro configuration
-> selected target development packages
-> cross-toolchain + target sysroot
-> installable SDK
-> application build outside BitBake
```

An SDK is tied to machine, distro, ABI, and package selection.

## Standard SDK

Generate:

```sh
bitbake <image> -c populate_sdk
```

Installer output normally appears under:

```text
tmp/deploy/sdk/
```

The installer creates:

- cross compiler/binutils
- environment setup script
- target sysroot
- host-side SDK tools
- development headers/libraries selected for the SDK

## Using The SDK

Install to an appropriate path, then source its environment script:

```sh
source /opt/product-sdk/environment-setup-<target-triplet>
```

Inspect:

```sh
echo "$CC"
echo "$CXX"
echo "$SDKTARGETSYSROOT"
echo "$PKG_CONFIG_SYSROOT_DIR"
echo "$PKG_CONFIG_PATH"
```

Build systems should consume these values rather than hardcoding compiler/sysroot paths.

## SDK Sysroot Contents

The target sysroot contains development files included by SDK policy:

- headers
- libraries
- pkg-config files
- CMake package files
- generated API headers

Runtime-only packages may not contribute development files. Ensure the corresponding `-dev` content is included through SDK population policy.

## Application Build Examples

### Make

```sh
make CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS"
```

Upstream Makefiles vary; prefer toolchain-aware interfaces.

### CMake

Yocto SDK environment commonly supplies enough information for appropriate cross configuration, but project/toolchain invocation depends on SDK and CMake setup.

Verify CMake does not discover host `/usr` packages.

### pkg-config

```sh
pkg-config --cflags --libs example
```

Check that output paths resolve through the SDK sysroot.

## Standard SDK Vs Extensible SDK

### Standard SDK

Best for:

- application compilation
- CI jobs that need a fixed toolchain/sysroot
- third-party application teams
- stable product API surfaces

### Extensible SDK

Generated with:

```sh
bitbake <image> -c populate_sdk_ext
```

It supports richer recipe-aware development workflows and can include `devtool` capabilities.

Use it when developers need to modify/build recipes rather than only compile applications against a fixed sysroot.

## SDK Population Policy

SDK contents derive from image and SDK-specific variables/classes.

Decide explicitly:

- which public product libraries are supported
- which headers are exposed
- whether static libraries are included
- whether debug symbols/source are available
- which host tools are shipped
- whether kernel headers or module build support are included

Do not treat "include every development package" as a sustainable SDK contract.

## Kernel Module SDK Considerations

External kernel module builds need more than generic userspace headers.

They may require:

- configured kernel build output
- generated headers
- `Module.symvers`
- exact compiler/toolchain
- matching kernel release/configuration

Define a dedicated module-development workflow rather than assuming the standard application SDK is sufficient.

## SDK Versioning

Record:

- product release
- machine
- distro
- SDK version
- layer revisions
- compiler version
- target triplet
- sysroot/package manifest

Applications built with one SDK should be traceable to the matching target image ABI.

## CI Use

SDK-based application CI can be faster than full image builds.

Recommended pipeline:

1. Publish versioned SDK from product build.
2. Verify installer checksum.
3. Install SDK in CI image/cache.
4. Source environment.
5. Build and test application.
6. Record SDK identity with application artifact.
7. Periodically validate application inside full image build.

The full BitBake build remains the release integration authority.

## Sysroot Drift

If an application compiles with the SDK but fails on target:

- SDK may not match image release
- runtime library package may be older/different
- ABI changed
- application copied outside package management
- loader/library search path differs

Compare SDK manifest, image manifest, library versions, and target architecture.

## Worked Example: SDK CMake Build Audit

After sourcing SDK:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --verbose
file build/product-app
readelf -l build/product-app | grep interpreter
```

Inspect verbose commands for SDK compiler and sysroot. Confirm output architecture and dynamic loader match target.

## Worked Example: Prove Runtime Compatibility

```sh
pkg-config --modversion libexample
```

Record SDK library version, then compare target package/image manifest. A successful link against SDK does not prove target rootfs contains the same ABI.

## Common Mistakes

- Using an SDK for the wrong machine or release.
- Forgetting to source environment setup.
- Letting CMake/pkg-config discover host libraries.
- Assuming runtime packages imply SDK development headers.
- Treating SDK as sufficient for arbitrary external kernel modules.
- Copying application binaries without packaging/runtime dependency tracking.
- Publishing unversioned SDK installers.

## Debugging Checklist

- Which image generated the SDK?
- What machine/distro does it target?
- Was the environment script sourced?
- What compiler is actually executed?
- What is `SDKTARGETSYSROOT`?
- Does pkg-config use the SDK sysroot?
- Are required development packages present?
- Does target image contain matching runtime libraries?
- Is the SDK version recorded with application output?

## Related Topics

- [Images and Package Groups](images-and-packagegroups.md)
- [Recipes](recipes.md)
- [Devtool and Recipe Development](devtool-and-recipe-development.md)
- [Target Triples and Sysroots](../../target-triples-and-sysroots.md)

## References

- Yocto Project SDK Manual
- Yocto Project Application Development documentation
- Yocto Project Development Tasks Manual
