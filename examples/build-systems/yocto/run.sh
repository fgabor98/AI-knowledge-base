#!/usr/bin/env bash
# Yocto session: real BitBake runs for the Yocto and TI Processor SDK pages.
#
# Usage: examples/build-systems/yocto/run.sh TOPDIR
#
# TOPDIR is a bitbake-setup top directory, for example ~/yocto. On the first
# run the script fetches bitbake-setup and creates TOPDIR/kb-am62xx-wrynose
# from kb-wrynose.conf.json (pinned bitbake, OE-Core, meta-yocto, meta-arm,
# meta-ti, plus the meta-kb-example layer next to this script). Downloads and
# shared state go to TOPDIR/.bitbake-setup-downloads and TOPDIR/.sstate-cache,
# shared by every setup under TOPDIR.
#
# The first run builds an AArch64 cross toolchain: plan for about 2 hours on
# 12 cores, 50 GB of disk, and network access to the upstream source mirrors.
# Later runs reuse shared state and take minutes.
#
# The session is too large for routine validation. It records its output in
# transcripts/*.txt (paths shown as ~/yocto and ~/AI-knowledge-base), and
# scripts/check-build-transcripts.py compares the docs with those files.
# Rerun this script after changing the layer or the pinned revisions.
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo=$(cd "$here/../../.." && pwd)
top=$(mkdir -p "${1:?usage: run.sh TOPDIR}" && cd "$1" && pwd)
setup=$top/kb-am62xx-wrynose
transcripts=$here/transcripts
bitbake_rev=6e260eb6ab1631607ea414d9ff8ede1155faa5d9

mkdir -p "$transcripts"
transcript=/dev/null
last_output=$(mktemp)
# Temporary recipes the session adds to the layer for a demonstration.
missing=$here/meta-kb-example/recipes-qa/kb-qa-demo/kb-qa-missing_1.0.bb
append=$here/meta-kb-example/recipes-apps/board-tool/board-tool_%.bbappend
legacy=$here/meta-kb-example/recipes-qa/kb-qa-demo/kb-qa-legacy-workdir_1.0.bb
r5away=$here/meta-kb-example/conf/machine/kb-am62-board-k3r5.conf.away
# restore - undo the session's temporary changes, even after a failure.
restore() {
    rm -f "$last_output" "$missing" "$append" "$legacy"
    if [[ -f $r5away ]]; then mv "$r5away" "${r5away%.away}"; fi
    if [[ -f $setup/build/conf/toolcfg.conf ]]; then
        sed -i 's|machine/kb-am62-board|machine/am62xx-evm|' "$setup/build/conf/toolcfg.conf"
    fi
}
trap restore EXIT

# record NAME - send the following commands to transcripts/NAME.txt.
record() {
    transcript=$transcripts/$1.txt
    : >"$transcript"
}

section() {
    printf '\n== %s\n' "$*" | tee -a "$transcript"
}

# normalize - stable, readable paths; drop server reconnection noise.
normalize() {
    sed -E -e "s|top-dir-prefix $(dirname "$top") --setting default top-dir-name $(basename "$top")|top-dir-prefix ~ --setting default top-dir-name yocto|" \
        -e "s|$top|~/yocto|g" -e "s|$repo|~/AI-knowledge-base|g" -e 's/ +$//' \
        -e 's/(log\.do_[a-z_]+)\.[0-9]+/\1/' |
        grep -Ev '^NOTE: (Retrying server connection|Reconnecting to bitbake server|Starting bitbake server)' || true
}

# run COMMAND - print and execute in the build environment; failures are
# recorded, not fatal, because several steps fail on purpose.
run() {
    printf '$ %s\n' "$*" | normalize | tee -a "$transcript"
    local status=0
    (cd "$setup/build" 2>/dev/null || cd "$top" && bash -c "$*") 2>&1 | normalize >"$last_output" || status=${PIPESTATUS[0]}
    if ((status != 0)); then
        printf '[exit status %d]\n' "$status" >>"$last_output"
    fi
    tee -a "$transcript" <"$last_output"
}

expect() {
    if ! grep -Eq -- "$1" "$last_output"; then
        printf '\nsession check failed: expected output matching /%s/\n' "$1" >&2
        exit 1
    fi
}

restore
last_output=$(mktemp)

# ---------------------------------------------------------------------------
record setup

if [[ ! -d $setup ]]; then
    tool=$top/.kb-bitbake-setup
    if [[ ! -d $tool ]]; then
        git clone -q -b 2.18 https://git.openembedded.org/bitbake "$tool"
        git -C "$tool" checkout -q "$bitbake_rev"
    fi
    section "Create the setup from the pinned configuration"
    run "$tool/bin/bitbake-setup --setting default top-dir-prefix $(dirname "$top") --setting default top-dir-name $(basename "$top") init --non-interactive --no-init-vscode $here/kb-wrynose.conf.json am62xx-evm 2>&1 | grep -Ev '^ *[0-9]+%|^ *\$'"
    expect "Setting up bitbake configuration"
fi

# Build settings used for the recorded run. They are host policy, so they go
# in local.conf, not in a layer.
if ! grep -q 'kb-example session' "$setup/build/conf/local.conf"; then
    cat >>"$setup/build/conf/local.conf" <<'EOF'

# kb-example session settings
BB_NUMBER_THREADS = "6"
PARALLEL_MAKE = "-j 6"
INHERIT += "buildhistory"
BUILDHISTORY_COMMIT = "1"
EOF
fi

# Every command below runs with the build environment sourced.
env_init=". $setup/build/init-build-env >/dev/null"

section "What bitbake-setup generated"
run "ls $setup $setup/build/conf"
expect "toolcfg.conf"
run "grep -v '^#' conf/bblayers.conf conf/toolcfg.conf"
expect "OE_FRAGMENTS \+= \"distro/poky machine/am62xx-evm\""
run "$env_init && bitbake-layers show-layers 2>&1 | sed 's/  */ /g'"
expect "meta-ti-bsp .* 6"

section "Fragments: what a fragment is, and switching MACHINE"
run "$env_init && bitbake-config-build list-fragments 2>&1 | grep -E 'machine/am62xx-evm|core/yocto/sstate-mirror-cdn|Available|Enabled' | head -n 8"
run "$env_init && bitbake-getvar --quiet MACHINE | grep -v '^#\$'"
expect "^MACHINE=\"am62xx-evm\""

# ---------------------------------------------------------------------------
record parse

section "Parse everything once"
run "$env_init && bitbake -p 2>&1 | grep -E '^(Loaded|Parsing of|ERROR)'"
expect "0 errors|Loaded [0-9]+ entries"

section "Where did a value come from? bitbake-getvar shows the history"
run "$env_init && bitbake-getvar --quiet PREFERRED_PROVIDER_virtual/kernel"
expect "linux-ti-staging"
run "$env_init && bitbake-getvar --quiet --value BBMULTICONFIG"
expect "k3r5"
run "$env_init && bitbake-getvar --quiet -r board-tool S"
expect "UNPACKDIR"
run "$env_init && for v in WORKDIR UNPACKDIR S B D; do printf '%-9s %s\n' \$v \"\$(bitbake-getvar --quiet -r board-tool --value \$v)\"; done"
expect "^S +.*/sources/make-project"

section "Providers: which recipe is the kernel and the bootloader"
run "$env_init && bitbake-layers show-recipes 'linux-ti-*' 'u-boot-ti-*' 2>&1 | grep -E '^[a-z]|^  meta' "
expect "u-boot-ti-staging"
run "$env_init && for v in PREFERRED_PROVIDER_virtual/kernel PREFERRED_VERSION_linux-ti-staging PREFERRED_PROVIDER_virtual/bootloader PREFERRED_VERSION_u-boot-ti-staging TI_PREFERRED_BSP KERNEL_IMAGETYPE SPL_BINARY UBOOT_MACHINE; do printf '%-40s %s\n' \$v \"\$(bitbake-getvar --quiet --value \$v)\"; done"
expect "UBOOT_MACHINE +am62x_evm_a53_defconfig"

section "Appends and the task list"
run "$env_init && bitbake-layers show-appends base-files 2>&1 | grep -A2 '^base-files'"
expect "base-files_%.bbappend"
run "$env_init && bitbake -c listtasks board-tool 2>&1 | grep -E '^do_' | head -n 40"
expect "do_package_qa"

section "The task graph crosses into the k3r5 multiconfig"
run "$env_init && bitbake -g kb-image 2>&1 | grep -E '^(NOTE: PN|ERROR)'; ls *.dot pn-buildlist"
expect "task-depends.dot"
run "grep -oE '\"mc:k3r5:[a-z0-9.+-]+\.do_[a-z_]+\"' task-depends.dot | sed 's/\"//g; s/\.do_.*//' | sort -u | wc -l"
run "grep -oE '\"mc:k3r5:[a-z0-9.+-]+\.do_deploy\"' task-depends.dot | sort -u"
expect "mc:k3r5:u-boot-ti-staging.do_deploy"
run "grep -E '\"kb-image.do_image\" -> \"mc:k3r5' task-depends.dot"
expect "do_deploy"

section "Inside the k3r5 multiconfig"
run "$env_init && bitbake -e mc:k3r5:virtual/bootloader 2>/dev/null | grep -E '^(PN|MACHINE|UBOOT_MACHINE|TCLIBC|TMPDIR|DEPLOY_DIR_IMAGE)='"
expect "^MACHINE=\"am62xx-evm-k3r5\""

section "A product machine needs its k3r5 twin"
r5conf=$here/meta-kb-example/conf/machine/kb-am62-board-k3r5.conf
run "$env_init && bitbake-config-build enable-fragment machine/kb-am62-board >/dev/null 2>&1; bitbake-getvar --quiet --value MACHINE"
expect "^kb-am62-board$"
mv "$r5conf" "$r5conf.away"
run "$env_init && bitbake -p 2>&1 | sed -n '/sanity checker/,/is invalid/p'"
expect "MACHINE=kb-am62-board-k3r5 is invalid"
mv "$r5conf.away" "$r5conf"
run "$env_init && bitbake -p 2>&1 | grep -E '^(Loaded|Parsing of|ERROR)'; bitbake-getvar --quiet --value SOC_FAMILY"
expect "ti-soc:k3:am62xx"
run "$env_init && bitbake-config-build enable-fragment machine/am62xx-evm >/dev/null 2>&1; bitbake-getvar --quiet --value MACHINE"
expect "^am62xx-evm$"

section "Scarthgap-era recipes on wrynose"
run "$env_init && bitbake kb-qa-legacy-git -c unpack 2>&1 | grep -E '^ERROR' | head -n 1"
expect "should remove that assignment"
cat >"$legacy" <<'EOF'
SUMMARY = "QA demo: S set to WORKDIR, valid for file:// recipes on scarthgap"
require kb-qa-demo.inc
SRC_URI = "file://hello.c"
S = "${WORKDIR}"
EOF
run "$env_init && bitbake -p 2>&1 | grep -E '^ERROR' | head -n 1"
expect "is no longer supported"
rm -f "$legacy"

section "A missing file:// source is a parse error"
cat >"$missing" <<'EOF'
SUMMARY = "QA demo: SRC_URI names a file that is not in FILESPATH"
require kb-qa-demo.inc
SRC_URI = "file://not-in-the-layer.conf"
EOF
run "$env_init && bitbake -p 2>&1 | grep -E 'ERROR|searched|kb-qa-missing/' | sed 's/^Parsing recipes\.\.\.//' | head -n 3"
expect "Unable to get checksum for kb-qa-missing SRC_URI entry not-in-the-layer.conf"
rm -f "$missing"
run "$env_init && bitbake -p 2>&1 | grep -E '^(Loaded|Parsing of|ERROR)'"
expect "0 errors|Loaded [0-9]+ entries"

# ---------------------------------------------------------------------------
record build

section "Build the Makefile-based recipe (its shared state removed first, so every task runs)"
run "$env_init && bitbake -c cleansstate board-tool >/dev/null 2>&1; bitbake board-tool 2>&1 | grep -E '^(NOTE: Tasks Summary|Summary|ERROR|WARNING)'"
expect "Tasks Summary"
workdir=$(cd "$setup/build" && bash -c "$env_init && bitbake-getvar --quiet -r board-tool --value WORKDIR")
run "ls $workdir"
expect "packages-split"
run "ls $workdir/sources"
expect "make-project"
run "grep -E 'NOTE: make|gcc ' $workdir/temp/log.do_compile | head -n 3 | cut -c1-400"
expect "aarch64-poky-linux-gcc"
run "file $workdir/image/usr/bin/board-tool | sed 's/, BuildID.*//'"
expect "ARM aarch64"
run "find $workdir/packages-split -type f | sed 's|.*/packages-split/||' | sort"
expect "^board-tool/usr/bin/board-tool"

section "What packaging recorded: files, automatic runtime dependencies"
run "$env_init && oe-pkgdata-util list-pkgs -p board-tool"
expect "^board-tool-dbg"
run "$env_init && oe-pkgdata-util list-pkg-files board-tool"
expect "/usr/bin/board-tool"
run "$env_init && oe-pkgdata-util read-value RDEPENDS board-tool"
expect "glibc|libc6"
run "$env_init && oe-pkgdata-util find-path /usr/bin/board-tool"
expect "board-tool: /usr/bin/board-tool"

section "Buildhistory keeps a reviewable record of every package"
run "cat buildhistory/packages/*/board-tool/board-tool/latest"
expect "^RDEPENDS = "

section "The bbappend in action"
run "$env_init && bitbake base-files 2>&1 | grep -E '^(NOTE: Tasks Summary|ERROR)'"
expect "Tasks Summary"
run "$env_init && bitbake base-files -c install 2>&1 | grep -E '^(NOTE: Tasks Summary|ERROR)' && cat \$(bitbake-getvar --quiet -r base-files --value D)/etc/issue"
expect "KB example board"

# ---------------------------------------------------------------------------
record qa

section "QA: a Makefile that hard-codes CC = gcc"
run "$env_init && bitbake kb-qa-hostcc 2>&1 | grep -E '^ERROR' | head -n 3 | cut -c1-400"
expect "kb-qa-hostcc"
run "grep -h 'Unable to recognise' tmp/work/*/kb-qa-hostcc/*/temp/log.do_package | tail -n 1 | cut -c1-300"
expect "Unable to recognise the architecture"
run "file tmp/work/*/kb-qa-hostcc/*/image/usr/bin/kb-qa-hostcc | sed 's/, BuildID.*//'"
expect "x86-64"

section "QA: a link rule that drops LDFLAGS"
run "$env_init && bitbake kb-qa-noldflags 2>&1 | grep -E '^ERROR' | head -n 3 | cut -c1-400"
expect "ldflags"

section "QA: a file installed but not packaged"
run "$env_init && bitbake kb-qa-unshipped 2>&1 | sed -n '/installed but not shipped/,/installed-vs-shipped/p' | cut -c1-400"
expect "installed-vs-shipped"

# ---------------------------------------------------------------------------
record sstate

section "Nothing changed: everything comes from shared state or is already done"
run "$env_init && bitbake board-tool 2>&1 | grep -E '^(Sstate summary|NOTE: Tasks Summary)'"
expect "didn't need to be rerun"

section "Change one variable and ask why tasks reran"
printf 'EXTRA_OEMAKE += "CFLAGS=-Os"\n' >"$append"
run "$env_init && bitbake board-tool 2>&1 | grep -E '^NOTE: Tasks Summary'"
expect "Tasks Summary"
run "$env_init && bitbake-diffsigs -t board-tool do_compile 2>&1 | head -n 12"
expect "EXTRA_OEMAKE"
rm -f "$append"
run "$env_init && bitbake board-tool 2>&1 | grep -E '^(Sstate summary|NOTE: Tasks Summary)'"
expect "Tasks Summary"

printf '\nTranscripts written to %s\n' "$transcripts"
