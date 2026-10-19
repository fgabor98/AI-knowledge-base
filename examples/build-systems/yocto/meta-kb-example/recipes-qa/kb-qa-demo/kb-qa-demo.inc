# Deliberately broken recipes: each one triggers one packaging QA error.
# Build one with "bitbake kb-qa-hostcc" and read the error it reports.
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
S = "${UNPACKDIR}"
EXCLUDE_FROM_WORLD = "1"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/hello ${D}${bindir}/${PN}
}
