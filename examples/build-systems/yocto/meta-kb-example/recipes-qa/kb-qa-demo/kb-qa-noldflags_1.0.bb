SUMMARY = "QA demo: the link rule drops LDFLAGS"
require kb-qa-demo.inc
SRC_URI = "file://hello.c file://Makefile.noldflags"

do_compile() {
    oe_runmake -f Makefile.noldflags
}
