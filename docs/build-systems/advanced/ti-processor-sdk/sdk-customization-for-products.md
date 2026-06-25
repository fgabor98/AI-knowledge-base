---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# SDK Customization for Products

## Goal

Learn how to turn a TI SDK baseline into a maintainable product build.

## Product Layer Model

Long-lived changes belong in product-owned layers, not in generated output and not directly in vendor layers.

```text
meta-ti / meta-arago / community layers
-> meta-company-bsp
-> meta-company-distro
-> meta-company-apps
-> meta-company-product
```

A simple split:

- `meta-company-bsp`: machine, DTS patches, bootloader/kernel appends
- `meta-company-distro`: distro policy, security, package format, update policy
- `meta-company-apps`: application recipes and services
- `meta-company-product`: image recipes and package groups

Small projects may combine these, but the ownership categories should remain clear.

## What To Customize Where

| Change | Good location |
| --- | --- |
| Add userspace app | application recipe in product layer |
| Add systemd service | app recipe or service recipe |
| Add packages to image | package group or product image recipe |
| Change kernel config | kernel `.bbappend` with config fragment |
| Patch kernel driver | kernel `.bbappend` with patch |
| Change board DTS | kernel `.bbappend` or product BSP source strategy |
| Change U-Boot config | U-Boot `.bbappend` or patch |
| Add custom machine | product BSP layer |
| Change distro policy | product distro layer |
| Change WIC layout | product image/BSP layer |

## Example Product Image

```bitbake
SUMMARY = "Company product image"
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-openssh"

IMAGE_INSTALL:append = " \
    packagegroup-company-base \
    company-agent \
"
```

## Example Package Group

```bitbake
SUMMARY = "Company base package group"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    iproute2 \
    ethtool \
    company-agent \
"
```

Package groups make image content easier to review than one enormous `IMAGE_INSTALL`.

## Example Service Recipe Pattern

```bitbake
SUMMARY = "Company agent"
LICENSE = "CLOSED"

SRC_URI = " \
    file://company-agent \
    file://company-agent.service \
"

inherit systemd

SYSTEMD_SERVICE:${PN} = "company-agent.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/company-agent ${D}${bindir}/company-agent

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/company-agent.service ${D}${systemd_system_unitdir}/
}
```

## Local Configuration Is Not Product Policy

`local.conf` is useful for developer-specific settings:

- build parallelism
- local download directory
- local sstate directory
- temporary experiments

It is a poor home for product policy:

- required packages
- board selection
- kernel fragments
- bootloader patches
- distro security choices
- release versioning

Move successful experiments into layers.

## Common Mistakes

- Keeping product changes in `local.conf`.
- Editing vendor recipes directly and losing changes during SDK upgrades.
- Adding every package directly to the image recipe instead of using package groups.
- Building application binaries outside Yocto and copying them into the rootfs.
- Mixing debug/development image content into production images.
- Failing to record why a vendor patch exists.

## Related Topics

- [Layers](../yocto-openembedded/layers.md)
- [Images and Package Groups](../yocto-openembedded/images-and-packagegroups.md)
- [Release Engineering and SDK Upgrades](release-engineering-and-sdk-upgrades.md)
