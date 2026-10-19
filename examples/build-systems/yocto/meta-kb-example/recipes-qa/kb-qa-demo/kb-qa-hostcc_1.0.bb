SUMMARY = "QA demo: the Makefile replaces the cross compiler with host gcc"
require kb-qa-demo.inc
SRC_URI = "file://hello.c file://Makefile.hostcc"

do_compile() {
    oe_runmake -f Makefile.hostcc
}
