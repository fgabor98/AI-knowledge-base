---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# BSP Release Reproducibility

## What Problem Does This Solve?

BSP release reproducibility means a team can rebuild, inspect, test, and explain the exact artifacts shipped to a board. It is the difference between "we have an image" and "we can maintain this product."

This topic covers manifests, source provenance, checksums, debug artifacts, license outputs, and rebuild discipline for complete embedded Linux systems.

## Core Concepts

- reproducible build
- release manifest
- source manifest
- patch manifest
- config manifest
- artifact checksum
- toolchain version
- SDK release
- build environment
- debug symbols
- license manifest
- SBOM
- artifact retention

## Mental Model

A BSP release is a graph:

```text
source revisions
patches
configuration
toolchain
build environment
image metadata
-> build outputs
-> deployed artifacts
-> runtime version evidence
```

Reproducibility means recording enough graph inputs and outputs to rebuild and audit the release later.

## Minimum Release Manifest

Record:

```text
product name
product version
release date
build ID
build host/container identity
Git repositories and revisions
Yocto layers or Buildroot revision
TI Processor SDK version where used
MACHINE / board config
DISTRO / image target where used
toolchain version
kernel source revision
kernel config source
U-Boot source revision
U-Boot defconfig source
device tree source revisions
patch list
image layout metadata
artifact paths
artifact checksums
known compatible hardware revisions
```

This can be a text file, JSON, YAML, or generated release artifact. The format matters less than completeness and consistency.

## Artifact Set To Archive

Archive:

- bootloader artifacts
- kernel image
- DTBs and overlays
- rootfs image/archive
- partitioned image
- update bundle
- kernel modules
- firmware files
- package manifest
- debug symbols
- `vmlinux`
- `System.map`
- source archives
- license manifests
- SBOM where available
- flashing instructions
- release notes

For debugging field issues, `vmlinux`, `System.map`, debug symbols, and exact source revision are often as important as the bootable image.

## Source Reproducibility

For every source component, record:

```text
repository URL
commit hash
tag if used
patches applied
checksum for archives
mirror location
```

Avoid release builds from floating branches:

```text
bad: branch = main
good: commit = 0123456789abcdef
```

Tags are useful labels, but commit hashes are the precise identity.

## Configuration Reproducibility

Record configuration inputs, not only generated outputs:

Kernel:

```text
defconfig
config fragments
final .config
```

U-Boot:

```text
board defconfig
environment source
final .config
```

Yocto:

```text
local.conf policy that belongs to release
bblayers.conf or layer manifest
MACHINE
DISTRO
image recipe
```

Buildroot:

```text
Buildroot defconfig
BR2_EXTERNAL revision
post-build scripts
post-image scripts
rootfs overlays
```

TI SDK:

```text
SDK release
oe-layersetup config
MACHINE
image target
Arago/TI layer revisions
```

## Build Environment Reproducibility

Record:

- OS/container image
- required host packages
- compiler/toolchain version
- SDK installer version
- environment setup scripts
- important environment variables
- cache policy

For CI:

```text
CI job ID
runner image
pipeline revision
cache keys
artifact IDs
```

For local release builds, prefer a documented container, VM, or pinned SDK setup over a developer's untracked workstation state.

## Runtime Version Evidence

The release should make runtime identification easy:

```sh
cat /etc/os-release
cat /etc/build
uname -a
cat /proc/cmdline
tr -d '\0' < /proc/device-tree/model
```

U-Boot should expose a recognizable version string:

```text
version
```

Applications should expose versions:

```sh
app --version
```

Runtime version evidence should connect back to the release manifest.

## Common Scenarios

### Field Bug Requires Kernel Debugging

Needed artifacts:

- exact kernel source
- exact config
- `vmlinux`
- `System.map`
- module debug symbols
- kernel log
- hardware revision
- DTB used at runtime

If these were not archived, debugging becomes much slower.

### Rebuild Produces Different Image

Possible causes:

- timestamps embedded in files
- source branch moved
- package feed changed
- host tool version changed
- cache reused stale output
- generated keys or IDs changed
- file ordering changed

Debug:

- compare manifests
- compare source revisions
- compare package manifests
- compare build logs
- compare image contents before comparing raw image bytes

### Vendor SDK Upgrade

Before upgrading:

- preserve old release manifest
- list product patches
- classify patches by owner
- rebuild old baseline
- build new unmodified vendor baseline
- reapply product layer changes
- validate artifacts and boot flow

Do not mix old and new SDK layers casually.

## Common Mistakes

- Archiving only the final image.
- Not archiving debug symbols.
- Building releases from unpinned branches.
- Recording tags but not commit hashes.
- Losing layer revision information.
- Treating `local.conf` as undocumented developer state.
- Failing to distinguish factory image from OTA artifact.
- Not recording hardware revision compatibility.
- Assuming cache-backed rebuilds prove reproducibility.

## Debugging Checklist

- Can every artifact be traced to source?
- Are all source revisions pinned?
- Are all patches listed?
- Are final configs archived?
- Are input configs archived?
- Is the toolchain version recorded?
- Are debug artifacts archived?
- Are license/SBOM artifacts archived?
- Are checksums recorded?
- Can a clean build reproduce functionally equivalent artifacts?
- Can runtime version information identify the release?

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Artifact Flow and Provenance](artifact-flow-and-provenance.md)
- [Image Layout and Deployment](image-layout-and-deployment.md)
- [Embedded Productization](../../../embedded-productization/index.md)

## References

- Yocto Project reproducible builds documentation
- Yocto license and source archiving documentation
- Buildroot manual
- TI Processor SDK Linux documentation
- Linux kernel debugging documentation
