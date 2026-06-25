---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Build Environment Setup

## Goal

Set up a host environment that can reproduce a TI SDK build without hiding problems behind local machine state.

## Host Requirements

Processor SDK releases document supported host distributions and package requirements. Follow those requirements for the first successful build. Later you can containerize or adapt, but the first baseline should minimize variables.

Typical host concerns:

- Ubuntu release compatibility
- required build packages
- Python version
- `git`, `tar`, `xz`, `zstd`, `chrpath`, `diffstat`, `gcc`, `g++`, `make`
- disk space
- locale
- filesystem case sensitivity
- network access for source fetches
- proxy and certificate settings

Large SDK builds need substantial disk space. Running out of disk in `tmp/`, `downloads/`, or `sstate-cache/` can produce misleading failures.

## Environment Variables

Important variables include:

- `MACHINE`
- `DISTRO`
- `DL_DIR`
- `SSTATE_DIR`
- `TMPDIR`
- `BB_NUMBER_THREADS`
- `PARALLEL_MAKE`
- proxy variables where needed

Prefer checked-in templates or documented setup scripts over ad hoc shell history. If a build depends on an unrecorded environment variable, CI and coworkers will not reproduce it.

## Clean Baseline Setup

A disciplined setup flow:

```bash
mkdir -p ~/ti-sdk-work
cd ~/ti-sdk-work
git clone <ti-oe-layersetup-url> oe-layersetup
cd oe-layersetup
./oe-layertool-setup.sh -f <release-config>
cd build
. conf/setenv
MACHINE=<documented-machine> bitbake <documented-image>
```

The exact URL, setup config, environment script, machine, and image target must come from the selected TI release documentation.

## Downloads And Sstate

For repeated builds, move downloads and shared state outside disposable build directories:

```conf
DL_DIR = "/srv/yocto/downloads"
SSTATE_DIR = "/srv/yocto/sstate-cache"
```

For a single-user workstation, local paths are fine. For a team or CI system, use a managed mirror/cache strategy. Do not let every CI job fetch the internet from scratch.

## Containers

Containers can make SDK builds more reproducible, but they do not remove the need to follow TI host requirements. A good container captures:

- supported base OS
- package list
- user ID strategy
- locale
- shell initialization
- mounted downloads and sstate caches
- persistent workspace volume

Avoid container images that bake in a half-built `tmp/` directory. Cache `DL_DIR` and `SSTATE_DIR`, not unstable workdirs.

## First Build Checklist

Before starting the first build:

- confirm host OS and packages match docs
- confirm enough disk space
- initialize layers from release config
- record layer revisions
- set documented `MACHINE`
- set documented `DISTRO`
- build the documented image target
- save the first successful build log and deploy artifact list

## Common Setup Failures

| Symptom | Likely cause |
| --- | --- |
| Fetch failures | network, proxy, moved upstream source, missing mirror |
| Python errors during parse | unsupported host, wrong environment, old layer combination |
| Missing provider | wrong `MACHINE`, missing layer, wrong setup config |
| Disk full during rootfs | underestimated build size |
| Native tool build failure | missing host package or incompatible host distro |

## Related Topics

- [SDK Overview and Release Model](sdk-overview-and-release-model.md)
- [Reproducibility, Caches, and Mirrors](../yocto-openembedded/reproducibility-caches-and-mirrors.md)
- [Debugging TI SDK Builds and Boots](debugging-ti-sdk-builds-and-boots.md)
