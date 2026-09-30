---
status: active
reviewed: false
domain: build-systems
difficulty: advanced
last_reviewed: null
---

# Product Builds, Supply Chain, And Release Delivery

Official reading for turning a working build into a maintainable, auditable,
secure product release.

## Official Reading

- [ ] **P0** [TI Processor SDK Linux documentation](https://software-dl.ti.com/processor-sdk-linux/esd/landing_page.html): release layout, build instructions, board support, and SDK artifacts.
- [ ] **P0** [Yocto Project release and layer compatibility documentation](https://docs.yoctoproject.org/ref-manual/): release cadence, supported hosts, layers, licensing, and configuration ownership.
- [ ] **P0** [SLSA specification](https://slsa.dev/spec/v1.0/): source, build, provenance, and verification requirements.
- [ ] **P0** [SPDX specifications](https://spdx.dev/specifications/): license and package identity data used in release manifests and SBOMs.
- [ ] **P1** [in-toto documentation](https://in-toto.io/): supply-chain layout, attestations, and verification.
- [ ] **P1** [U-Boot verified boot documentation](https://docs.u-boot.org/en/latest/): signing, key handling, and boot-time verification.
- [ ] **P1** [Yocto SDK manual](https://docs.yoctoproject.org/sdk-manual/): distributing a target SDK and extending it for product development.

## Practice And Evidence

- [ ] Produce a release manifest linking source revisions, patches, toolchain,
  layers, configuration, packages, images, checksums, and signatures.
- [ ] Build an SDK and prove that a consumer can compile against the same
  headers, libraries, sysroot, and target ABI used by the product image.
- [ ] Define ownership for BSP patches, product configuration, generated
  artifacts, signing keys, and update bundles.
- [ ] Validate one boot artifact and one OTA/update artifact from clean inputs,
  including rollback or recovery evidence.

## Exit Criteria

- [ ] I can explain what makes a release reproducible, traceable, updateable,
  and verifiable—not merely buildable.
- [ ] I can hand another engineer enough evidence to recreate or audit a
  product artifact without relying on an undocumented workstation.
