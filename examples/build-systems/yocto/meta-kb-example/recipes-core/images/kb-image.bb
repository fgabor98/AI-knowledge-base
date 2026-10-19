SUMMARY = "Example product image"
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL = "packagegroup-core-boot packagegroup-kb-tools"
IMAGE_FEATURES += "ssh-server-dropbear"
