---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# End-to-End Product Layer Lab

## Goal

Build a small product layer on top of a TI SDK baseline and prove that kernel, U-Boot, image, and runtime changes are owned by metadata.

## Prerequisites

You should already know:

- selected TI SDK release
- documented EVM `MACHINE`
- documented image target
- deploy artifact directory
- basic BitBake commands
- how to flash or boot the EVM

## Lab Scope

The lab creates:

- product layer
- product package group
- product image
- simple systemd service
- kernel config fragment
- U-Boot version string or config change
- optional DTS label/model change for proof
- release artifact checklist

Keep the changes intentionally small. The point is ownership, not feature complexity.

## Step 1: Create Product Layer

Example:

```bash
bitbake-layers create-layer ../sources/meta-company
bitbake-layers add-layer ../sources/meta-company
bitbake-layers show-layers
```

Review `conf/layer.conf`. Set dependencies deliberately if your layer depends on TI or Arago metadata.

## Step 2: Add Package Group

Create a package group for product base packages:

```bitbake
SUMMARY = "Company base package group"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    iproute2 \
    ethtool \
"
```

## Step 3: Add Product Image

Create a product image that inherits from a suitable base or core image:

```bitbake
SUMMARY = "Company product image"
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL:append = " \
    packagegroup-company-base \
"
```

For a real product, decide whether to inherit a TI image, a core image, or a company base image. That choice affects how much TI demo content enters your product.

## Step 4: Add A Service

Add a small service recipe and confirm it starts on target. The service can simply write a boot marker to `/run`.

Validation:

```bash
systemctl status company-agent
journalctl -u company-agent
```

## Step 5: Add Kernel Config Fragment

Add a kernel `.bbappend` with a small config fragment. Pick a harmless option that proves the mechanism.

Validation:

```bash
zcat /proc/config.gz | grep CONFIG_IKCONFIG
```

or inspect the final `.config` in the kernel workdir.

## Step 6: Add U-Boot Change

Add a minimal U-Boot patch or config change that is visible in the serial log or U-Boot prompt. Avoid functional boot changes for the first lab.

Validation:

```text
version
printenv
```

## Step 7: Build And Deploy

Build:

```bash
MACHINE=<evm-machine> bitbake company-product-image
```

Inspect:

```bash
ls -lh tmp/deploy/images/<machine>/
```

Flash the generated image using the SDK-documented method.

## Step 8: Runtime Validation

On target:

```bash
uname -a
cat /proc/cmdline
cat /etc/os-release
systemctl status company-agent
dmesg | grep -i firmware
```

From serial boot log, capture:

- early boot artifact names
- U-Boot version
- kernel version
- DTB model
- rootfs boot result

## Step 9: Release Bundle

Archive:

- layer manifest
- `local.conf` template
- `bblayers.conf` template or setup script
- image manifest
- license manifest
- boot artifact checksums
- WIC checksum
- serial boot log
- validation notes

## Completion Criteria

The lab is complete when:

- no product changes live only in `tmp/work`
- no product changes live only in `local.conf`
- image builds from a clean checkout
- generated WIC boots
- kernel change is visible
- U-Boot change is visible
- service is installed and running
- artifacts are traceable to layer revisions

## Common Mistakes

- Making the first lab a full board port.
- Using manual rootfs edits as shortcuts.
- Forgetting to add the product layer to `bblayers.conf`.
- Building the TI default image instead of the product image.
- Validating kernel changes without checking the booted kernel version.
- Declaring success without saving the boot log.

## Related Topics

- [SDK Customization for Products](sdk-customization-for-products.md)
- [Deployment and Flashing](deployment-and-flashing.md)
- [Release Engineering and SDK Upgrades](release-engineering-and-sdk-upgrades.md)
