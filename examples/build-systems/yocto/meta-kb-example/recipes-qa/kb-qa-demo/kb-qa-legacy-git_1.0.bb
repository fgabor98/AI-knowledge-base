SUMMARY = "QA demo: a scarthgap-era recipe that sets S to WORKDIR/git"
require kb-qa-demo.inc
SRC_URI = "file://hello.c"

# Required on scarthgap (5.0) for git sources. Since whinlatter (5.3) the
# default S is correct for git, and this assignment fails do_unpack.
S = "${WORKDIR}/git"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} ${UNPACKDIR}/hello.c -o ${B}/hello
}
