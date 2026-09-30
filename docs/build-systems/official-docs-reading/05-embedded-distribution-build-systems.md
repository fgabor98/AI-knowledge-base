---
status: active
reviewed: false
domain: build-systems
difficulty: advanced
last_reviewed: null
---

# Embedded Distribution Build Systems

Official reading for systems that assemble a complete embedded Linux
distribution from toolchain through root filesystem and image artifacts.

## Official Reading

- [ ] **P0** [Buildroot User Manual](https://buildroot.org/downloads/manual/manual.html): quick start, configuration, toolchains, package infrastructure, filesystem images, rebuild behavior, and external trees.
- [ ] **P0** [Yocto Project documentation](https://docs.yoctoproject.org/): overview, workflow, reference manual, development tasks, and release process.
- [ ] **P0** [BitBake User Manual](https://docs.yoctoproject.org/bitbake/): metadata, tasks, recipes, dependencies, providers, signatures, and execution.
- [ ] **P0** [OpenEmbedded Layer Index](https://layers.openembedded.org/): layer compatibility, dependencies, and recipe provenance.
- [ ] **P1** [Yocto Project Mega-Manual](https://docs.yoctoproject.org/dev/overview-manual/index.html): concepts, configuration, images, packages, SDKs, and development workflow.
- [ ] **P1** [Wic documentation](https://docs.yoctoproject.org/ref-manual/kickstart.html): partitioned image composition and kickstart definitions.

## Practice And Evidence

- [ ] Build one minimal image with Buildroot and one with Yocto, recording the
  configuration, source revisions, host requirements, and output directory.
- [ ] Trace one package from source fetch through configure, compile, install,
  package creation, rootfs inclusion, and final image.
- [ ] Explain the different ownership models for Buildroot packages and Yocto
  recipes/layers, including where patches and configuration belong.
- [ ] Inspect the generated root filesystem, package manifest, license data,
  and image layout.

## Exit Criteria

- [ ] I can choose Buildroot or Yocto for a product constraint and justify the
  trade-offs around configuration, packages, updates, and long-term maintenance.
- [ ] I can identify whether a failure belongs to fetch, metadata parsing,
  task execution, packaging, rootfs assembly, or image generation.
