SUMMARY = "Board tool built from a plain Makefile"
DESCRIPTION = "The project from the Intermediate Make page, built with oe_runmake."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# files/make-project is the project from the Intermediate Make page (a
# symlink in this repository; run.sh copies the real files into the layer).
# A product recipe would fetch a tagged release with git:// instead.
SRC_URI = "file://make-project"
S = "${UNPACKDIR}/make-project"

# BitBake exports CC, CFLAGS, LDFLAGS, ... in the task environment. The
# Makefile keeps them because it only picks a compiler when origin is
# "default". Everything else is passed explicitly on the command line.
EXTRA_OEMAKE = "VERSION=${PV}"

do_compile() {
    oe_runmake
}

do_install() {
    oe_runmake install DESTDIR=${D} PREFIX=${prefix} bindir=${bindir}
}
