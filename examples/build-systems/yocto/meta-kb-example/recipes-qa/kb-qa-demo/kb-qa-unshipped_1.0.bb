SUMMARY = "QA demo: a file is installed but no package owns it"
require kb-qa-demo.inc
SRC_URI = "file://hello.c file://kb-qa-demo.conf"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} ${S}/hello.c -o ${B}/hello
}

do_install:append() {
    install -d ${D}/opt/kb
    install -m 0644 ${UNPACKDIR}/kb-qa-demo.conf ${D}/opt/kb/kb-qa-demo.conf
}
