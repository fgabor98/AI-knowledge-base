---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Packaging, QA, and Package Feeds

## What Problem Does This Solve?

Recipes install files into `${D}`, but products consume binary packages. Package splitting, dependency metadata, QA checks, alternatives, and feeds determine what can enter images or be updated later.

## Package Pipeline

```text
do_install -> ${D}
-> do_package
-> packages-split
-> package backend output
-> package feed/rootfs package manager
```

## Default Package Split

A typical recipe can emit:

```text
${PN}
${PN}-dev
${PN}-dbg
${PN}-staticdev
${PN}-doc
${PN}-src
```

Actual packages depend on classes, files, and distro policy.

## Worked Example: Runtime, CLI, And Configuration Packages

```bitbake
PACKAGES =+ "${PN}-cli ${PN}-config"

FILES:${PN} = "${sbindir}/product-daemon ${systemd_system_unitdir}/product.service"
FILES:${PN}-cli = "${bindir}/productctl"
FILES:${PN}-config = "${sysconfdir}/product"

RDEPENDS:${PN} += "${PN}-config"
RRECOMMENDS:${PN} += "${PN}-cli"
```

Use hard runtime dependencies only when the main package cannot function without them. Recommendations express optional but useful companions where package policy honors recommendations.

## Package Order

`PACKAGES` order can matter because files are assigned according to package patterns and ordering rules. Put specific package patterns before broad `${PN}` patterns when needed.

Inspect:

```text
${WORKDIR}/packages-split/<package>/
```

## `FILES`

Examples:

```bitbake
FILES:${PN} += "${libexecdir}/product/*"
FILES:${PN}-cli = "${bindir}/product-*"
```

Avoid overly broad patterns that steal files from `-dev`, `-dbg`, or another intended package.

## Dependency Types

```bitbake
RDEPENDS:${PN} += "required-package"
RRECOMMENDS:${PN} += "optional-package"
RCONFLICTS:${PN} += "incompatible-package"
RREPLACES:${PN} += "old-package"
RPROVIDES:${PN} += "virtual-product-service"
```

Use package-scoped dependencies, not recipe-scoped assumptions.

## Shared Library Dependencies

Yocto scans every packaged ELF file for `DT_NEEDED` entries (the `shlibs` step of `do_package`) and turns them into runtime dependencies on the packages that provide those libraries. The `board-tool` recipe declares no `RDEPENDS` at all, yet:

```text
$ . ~/yocto/kb-am62xx-wrynose/build/init-build-env >/dev/null && oe-pkgdata-util read-value RDEPENDS board-tool
glibc (>= 2.43+git0+562cd7badd)
```

The version constraint comes from the glibc that the binary was linked against, the same check the loader performs with symbol versions ([Shared Libraries, ABI, and Runtime Linking](../../shared-libraries-abi-and-runtime-linking.md#symbol-versions)).

Manual `RDEPENDS` should not duplicate correct automatic dependencies. If scanning fails, investigate library naming, packaging, or unusual loading behavior.

## Alternatives

When multiple packages provide the same command/path, use alternatives infrastructure rather than file conflicts.

Conceptual example:

```bitbake
inherit update-alternatives

ALTERNATIVE:${PN} = "product-tool"
ALTERNATIVE_LINK_NAME[product-tool] = "${bindir}/product-tool"
ALTERNATIVE_TARGET[product-tool] = "${bindir}/product-tool.full"
ALTERNATIVE_PRIORITY = "100"
```

Verify exact class syntax for the active release.

## QA Checks

Package QA (`insane.bbclass`) inspects every packaged file. Each check has a name; `ERROR_QA` lists the ones that fail the build and `WARN_QA` the ones that only warn. On wrynose `ERROR_QA` includes, among others:

| Check | Catches |
| --- | --- |
| `arch` | binary built for the wrong architecture, ABI, or word size |
| `ldflags` | binary linked without the distro `LDFLAGS` (detected through the missing `GNU_HASH` section) |
| `installed-vs-shipped` | file in `${D}` that no package's `FILES` claims |
| `rpaths` | `RPATH` pointing into the build directory |
| `dev-so` | `.so` development symlink in a runtime package |
| `already-stripped` | upstream build stripped the binary, so no debug package can be made |
| `buildpaths` | build directory paths embedded in output |
| `file-rdeps` | a file needs an interpreter or library no runtime dependency provides |
| `textrel` | text relocations in a shared object (missing `-fPIC`) |
| `license-checksum` | `LIC_FILES_CHKSUM` does not match the license text |

Treat QA as a correctness signal. Do not broadly disable checks to make a build green.

## Real QA Failures From Makefile Mistakes

The example layer has deliberately broken recipes; each reproduces a mistake from [Make Variables and Pattern Rules](../../make-variables-and-pattern-rules.md). Output from `examples/build-systems/yocto/run.sh`:

**The link rule ignores `LDFLAGS`** (`$(CC) $(CFLAGS) $< -o $@`):

```text
$ . ~/yocto/kb-am62xx-wrynose/build/init-build-env >/dev/null && bitbake kb-qa-noldflags 2>&1 | grep -E '^ERROR' | head -n 3 | cut -c1-400
ERROR: kb-qa-noldflags-1.0-r0 do_package_qa: QA Issue: File /usr/bin/kb-qa-noldflags in package kb-qa-noldflags doesn't have GNU_HASH (didn't pass LDFLAGS?) [ldflags]
ERROR: kb-qa-noldflags-1.0-r0 do_package_qa: Fatal QA errors were found, failing task.
ERROR: Logfile of failure stored in: ~/yocto/kb-am62xx-wrynose/build/tmp/work/aarch64-poky-linux/kb-qa-noldflags/1.0/temp/log.do_package_qa
```

The missing `GNU_HASH` is only the detector. The real loss is everything else in `LDFLAGS`: `--as-needed`, `-z relro`/`-z now` hardening, and any sysroot or library paths. Fix the Makefile to put `$(LDFLAGS)` in the link rule.

**The Makefile hard-codes `CC = gcc`**, which replaces the cross compiler exported by BitBake. The build "succeeds" and produces a host binary; it fails when packaging tries to split debug information with the target `objcopy`:

```text
$ . ~/yocto/kb-am62xx-wrynose/build/init-build-env >/dev/null && bitbake kb-qa-hostcc 2>&1 | grep -E '^ERROR' | head -n 3 | cut -c1-400
ERROR: kb-qa-hostcc-1.0-r0 do_package: Fatal errors occurred in subprocesses:
ERROR: Logfile of failure stored in: ~/yocto/kb-am62xx-wrynose/build/tmp/work/aarch64-poky-linux/kb-qa-hostcc/1.0/temp/log.do_package
ERROR: Task (~/AI-knowledge-base/examples/build-systems/yocto/meta-kb-example/recipes-qa/kb-qa-demo/kb-qa-hostcc_1.0.bb:do_package) failed with exit code '1'
$ grep -h 'Unable to recognise' tmp/work/*/kb-qa-hostcc/*/temp/log.do_package | tail -n 1 | cut -c1-300
Subprocess output:aarch64-poky-linux-objcopy: Unable to recognise the architecture of the input file `~/yocto/kb-am62xx-wrynose/build/tmp/work/aarch64-poky-linux/kb-qa-hostcc/1.0/package/usr/bin/kb-qa
$ file tmp/work/*/kb-qa-hostcc/*/image/usr/bin/kb-qa-hostcc | sed 's/, BuildID.*//'
tmp/work/aarch64-poky-linux/kb-qa-hostcc/1.0/image/usr/bin/kb-qa-hostcc: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2
```

The task error itself does not say "wrong compiler"; the log does, and `file` on the installed binary proves it. Whenever a packaging task fails with `objcopy`, `strip`, or `Unable to recognise the architecture`, check the architecture of the files in `image/` first. (If debug splitting is disabled, the `arch` QA check reports the same mistake as `Architecture did not match`.) The fix belongs in the Makefile (respect `CC` from the environment) or in the recipe (`EXTRA_OEMAKE = "CC='${CC}'"`); see [Recipes: Makefile-based software](recipes.md#makefile-based-software).

**A file is installed but no package owns it**:

```text
$ . ~/yocto/kb-am62xx-wrynose/build/init-build-env >/dev/null && bitbake kb-qa-unshipped 2>&1 | sed -n '/installed but not shipped/,/installed-vs-shipped/p' | cut -c1-400
ERROR: kb-qa-unshipped-1.0-r0 do_package: QA Issue: kb-qa-unshipped: Files/directories were installed but not shipped in any package:
  /opt
  /opt/kb
  /opt/kb/kb-qa-demo.conf
Please set FILES such that these items are packaged. Alternatively if they are unneeded, avoid installing them or delete them within do_install.
kb-qa-unshipped: 3 installed and not shipped files. [installed-vs-shipped]
```

`/opt` is not in any default `FILES` pattern. Either add `FILES:${PN} += "/opt/kb"`, install to a standard location (`${sysconfdir}`, `${datadir}`), or stop installing the file.

### `INSANE_SKIP` Is A Documented Exception, Not A Fix

Each check can be skipped per package:

```bitbake
INSANE_SKIP:${PN} += "already-stripped"
```

Use it only when the finding is understood and accepted, with a comment that says why and who decided. Valid examples: a prebuilt vendor binary that is shipped stripped and cannot be rebuilt (`already-stripped`); firmware for another core that is intentionally a different architecture (`arch`). Invalid: silencing `ldflags`, `rpaths`, or `installed-vs-shipped` for code you build. The QA demonstration recipes above are excluded from `world` builds instead of skipping checks, so their errors stay visible.

## Worked Example: Installed But Not Shipped

Failure says:

```text
... installed-vs-shipped ... /usr/share/product/schema.json
```

Debug:

1. Confirm file under `${D}${datadir}/product/`.
2. Decide package ownership.
3. Add:

   ```bitbake
   FILES:${PN} += "${datadir}/product/schema.json"
   ```

4. Rebuild package task.
5. Inspect `packages-split`.

Do not delete the file merely to silence QA if runtime needs it.

## Package Backends

Yocto supports package formats such as RPM, DEB, and IPK depending on configuration.

The backend influences:

- package files
- rootfs package manager
- feed tooling
- update workflows

Choose at distro policy level. Do not switch backend casually inside one product release line.

## Package Feeds

A feed publishes binary packages and repository metadata for target package managers.

Development use:

- iterate one application without reflashing
- install diagnostics temporarily
- test package dependencies

Production use requires deliberate security and update policy:

- signatures
- TLS/authentication
- version pinning
- rollback
- atomicity
- compatibility
- retention

A raw package feed is not automatically a robust product update system.

## Worked Example: Development Feed Flow

1. Configure package backend.
2. Build packages/image.
3. Publish deploy package directory through an internal HTTP server.
4. Configure target repository URL.
5. Refresh package metadata.
6. Install package.
7. Verify version and service.

Commands on target depend on RPM/DEB/IPK backend. Document the exact product backend rather than giving one universal command.

## Package Versioning

Version comparison can include epoch, version, and revision concepts.

When changing package contents without changing upstream version, ensure package revision/version policy causes feeds and images to recognize the new package.

Automated build/version metadata must remain deterministic and traceable.

## Package Manifests

Archive image package manifests to answer:

- which package versions shipped?
- why is a file present?
- which release introduced a dependency?

Pair manifests with artifact checksums and layer/source revisions.

## Common Mistakes

- Confusing recipe and package names.
- Broad `FILES:${PN}` patterns stealing development/debug files.
- Adding manual shared-library dependencies unnecessarily.
- Disabling QA globally.
- Using `RDEPENDS` where `RRECOMMENDS` expresses optional behavior.
- Publishing unsigned/uncontrolled feeds to production devices.
- Forgetting package version changes for feed updates.

## Debugging Checklist

- What files exist under `${D}`?
- What packages are in `PACKAGES` and in what order?
- Which `FILES` pattern owns the file?
- What does `packages-split` show?
- Which dependencies were auto-generated?
- What QA check failed and why?
- Which backend is active?
- Does the image manifest include expected package/version?
- Is feed/update policy appropriate for production?

## Related Topics

- [Recipes](recipes.md)
- [Images and Package Groups](images-and-packagegroups.md)
- [Debugging BitBake Builds](debugging-bitbake-builds.md)
- [Licensing, CVE, and SBOM Workflows](licensing-cve-and-sbom.md)

## References

- Yocto Project package documentation
- Yocto Project QA/error documentation
- Yocto Project Reference Manual
