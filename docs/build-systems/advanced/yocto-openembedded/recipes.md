---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Recipes

## What Problem Does This Solve?

Recipes describe how source becomes installed files and binary packages. Understanding recipe anatomy is required to add applications, services, patches, firmware, configuration files, kernel changes, and third-party components to an embedded image.

## Core Concepts

- `.bb`
- `.bbappend`
- `PN`, `PV`, `PR`
- `SUMMARY`, `DESCRIPTION`, `HOMEPAGE`
- `LICENSE`, `LIC_FILES_CHKSUM`
- `SRC_URI`, `SRCREV`
- `S`, `B`, `D`
- inheritance
- dependencies
- packaging
- overrides

## Minimal Recipe

```bitbake
SUMMARY = "Example application"
DESCRIPTION = "Small example application for the target"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=<checksum>"

SRC_URI = "git://example.invalid/example.git;branch=main;protocol=https"
SRCREV = "<fixed-commit>"

inherit cmake
```

Production recipes should pin source revisions rather than following moving branch heads.

There is no `S` line. On wrynose the git fetcher unpacks into `${UNPACKDIR}/${BP}`, which is the default `S`. Recipes written for scarthgap and older set `S = "${WORKDIR}/git"`; on wrynose that line is a fatal QA error and must be deleted (see [Release Baseline](index.md#release-baseline)).

## Recipe Filename

Common format:

```text
<name>_<version>.bb
```

Example:

```text
example-app_1.2.0.bb
```

BitBake derives default values such as `PN` and `PV` from the filename, though metadata can alter them.

## Metadata Fields

### `SUMMARY`

Short package/recipe summary.

### `DESCRIPTION`

Longer explanation.

### `HOMEPAGE`

Upstream project location.

### `LICENSE`

Declared license expression.

### `LIC_FILES_CHKSUM`

Identifies and checksums source license text. A checksum change forces review rather than silently accepting an upstream license change.

## Sources With `SRC_URI`

`SRC_URI` can contain:

- Git repositories
- archives
- patches
- local configuration files
- service units
- scripts
- device tree files

Example:

```bitbake
SRC_URI = "https://example.invalid/example-${PV}.tar.gz \
           file://0001-fix-cross-build.patch \
           file://example.service \
           file://example.conf \
"
```

Every source entry should have clear ownership and purpose.

## Checksums

Archive fetches normally require checksums:

```bitbake
SRC_URI[sha256sum] = "<sha256>"
```

Checksums provide integrity and detect upstream replacement of source archives.

## Git Revisions

Pin a commit:

```bitbake
SRCREV = "0123456789abcdef..."
PV = "1.2+git"
```

Because `PV` contains `+`, packaging appends the short revision to the package version automatically (`1.2+git0+0123456789`). Older recipes write `PV = "1.2+git${SRCPV}"`; `SRCPV` is no longer needed since Yocto 4.3 and is defined as empty on wrynose, so the suffix is harmless but redundant.

Avoid floating revisions in release builds. Network state must not silently change source inputs.

## Source And Build Directories

### `${UNPACKDIR}`

Where `do_unpack` puts everything from `SRC_URI`: `${WORKDIR}/sources` on wrynose. Local `file://` files are here, not in `${WORKDIR}` as on scarthgap.

### `${S}`

Source directory. Default: `${UNPACKDIR}/${BP}` (for example `sources/board-tool-1.0`), which matches release tarballs and git checkouts. Set it only when the source unpacks somewhere else.

### `${B}`

Build directory.

For CMake, out-of-tree builds are common. For some upstream projects, `${B}` may equal `${S}`.

Inspect:

```sh
for v in WORKDIR UNPACKDIR S B D; do printf '%-9s %s\n' "$v" "$(bitbake-getvar --quiet -r example-app --value "$v")"; done
```

## Inheritance

Recipes inherit reusable behavior:

```bitbake
inherit cmake pkgconfig systemd
```

Classes can add:

- tasks
- task dependencies
- default variables
- packaging behavior
- QA checks

Before overriding a task, inspect what the inherited class already provides.

## Dependencies

Build dependencies:

```bitbake
DEPENDS += "libfoo"
```

Runtime package dependencies:

```bitbake
RDEPENDS:${PN} += "bash"
```

Do not add dependencies merely to change task order. Model the real build or runtime requirement.

## Task Functions

Example manual compile/install:

```bitbake
do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} ${S}/example.c -o example
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 example ${D}${bindir}/example
}
```

Use BitBake-provided compiler variables. Calling host `gcc` directly breaks cross-compilation.

## Installing Files

Install into `${D}` using target path variables:

```bitbake
do_install:append() {
    install -d ${D}${sysconfdir}/example
    install -m 0644 ${UNPACKDIR}/example.conf \
        ${D}${sysconfdir}/example/example.conf
}
```

`${D}` is staging. `${sysconfdir}` is the target path component.

## Packaging

Installed files must belong to packages.

Default package:

```bitbake
FILES:${PN} += "${sysconfdir}/example"
```

Additional package:

```bitbake
PACKAGES += "${PN}-tools"
FILES:${PN}-tools = "${bindir}/example-tool"
RDEPENDS:${PN} += "${PN}-tools"
```

Inspect package split output under `${WORKDIR}/packages-split/`.

## Systemd Service Example

```bitbake
inherit systemd

SRC_URI += "file://example.service"

SYSTEMD_SERVICE:${PN} = "example.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/example.service \
        ${D}${systemd_system_unitdir}/example.service
}

FILES:${PN} += "${systemd_system_unitdir}/example.service"
```

The distro must use compatible init-system policy for this to be meaningful.

## Overrides

Machine-specific metadata:

```bitbake
SRC_URI:append:product-board = " file://board.conf"
```

Package-specific metadata:

```bitbake
RDEPENDS:${PN} += "dependency"
```

Override order and syntax matter. Inspect final values with `bitbake -e`.

## Variable Operators

Common operators:

```bitbake
VAR = "value"
VAR ?= "default"
VAR ??= "weak-default"
VAR += " item"
VAR:append = " item"
VAR:prepend = "item "
```

They differ in timing and semantics. Do not treat `+=` and `:append` as interchangeable in all contexts.

## Recipe Appends

A `.bbappend` modifies a recipe from another layer.

Use it for product deltas:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI += "file://0001-product-fix.patch"
```

Keep appends small. If an append replaces most of a recipe, reassess ownership.

## Recipe Development Workflow

1. Identify upstream source and license.
2. Choose fixed source revision/version.
3. Select appropriate build class.
4. Add real build dependencies.
5. Verify `do_compile`.
6. Verify installed files in `${D}`.
7. Verify package split.
8. Add package to an image only after recipe/package correctness.
9. Run QA and license checks.

## Inspection Commands

```sh
bitbake-layers show-recipes example-app
bitbake -e example-app
bitbake -c listtasks example-app
bitbake -c fetch example-app
bitbake -c unpack example-app
bitbake -c compile example-app
bitbake -c install example-app
```

## Makefile-Based Software

Many embedded components (vendor tools, small daemons, BusyBox-style projects) have only a plain Makefile. There is no build class for that; the recipe calls `oe_runmake` itself. The example layer builds the project from [Intermediate Make](../../make-intermediate.md):

```bitbake
--8<-- "examples/build-systems/yocto/meta-kb-example/recipes-apps/board-tool/board-tool_1.0.bb"
```

How the pieces connect:

- `oe_runmake` runs `make ${PARALLEL_MAKE} ${EXTRA_OEMAKE} "$@"` and fails the task on error. Use it instead of calling `make` directly.
- BitBake **exports** the toolchain in the task environment: `CC` (with `--sysroot` and tuning flags), `CFLAGS`, `CPPFLAGS`, `LDFLAGS` (including `-Wl,--as-needed` and hash-style flags), `AR`, and so on. `EXTRA_OEMAKE` is empty by default.
- So the Makefile **must accept `CC` and flags from the environment**. A Makefile that writes `CC = gcc` or `CC := $(CROSS_COMPILE)gcc` replaces the cross compiler with the host compiler, and one that drops `$(LDFLAGS)` loses the distro's linker flags. Both fail package QA; the real errors are shown in [Packaging, QA, and Package Feeds](packaging-qa-and-feeds.md#real-qa-failures-from-makefile-mistakes).
- Install with `DESTDIR=${D}` and the path variables (`${prefix}`, `${bindir}`), never with absolute host paths.

If you cannot change a vendor Makefile, fix it from the recipe, in this order of preference:

1. Pass the variables on the make command line, which beats any assignment in the Makefile: `EXTRA_OEMAKE = "CC='${CC}' CFLAGS='${CFLAGS}' LDFLAGS='${LDFLAGS}'"`.
2. Use `EXTRA_OEMAKE = "-e"` to let environment variables override Makefile assignments. This is a blunt instrument: every exported variable then wins.
3. Patch the Makefile and send the fix upstream.

## Worked Example: CMake Application With pkg-config

```bitbake
SUMMARY = "Product telemetry client"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=<checksum>"

SRC_URI = "git://example.invalid/telemetry.git;branch=main;protocol=https"
SRCREV = "<fixed-commit>"
DEPENDS = "libcurl"

inherit cmake pkgconfig

EXTRA_OECMAKE = "-DENABLE_TESTS=OFF"
```

The recipe does not manually pass target include/library paths. The CMake and pkg-config classes prepare the cross environment and recipe sysroot.

## Worked Example: Install A Volatile Default

If a file must be regenerated or writable at runtime, do not overwrite persistent user configuration on every package upgrade. Install a factory default under `${datadir}` and let a first-boot/service policy copy it deliberately.

```bitbake
do_install:append() {
    install -d ${D}${datadir}/product/defaults
    install -m 0644 ${UNPACKDIR}/agent.conf \
        ${D}${datadir}/product/defaults/agent.conf
}
```

## Common Mistakes

- Calling host tools instead of BitBake-provided tools.
- Installing directly into host or final rootfs paths.
- Forgetting license checksums.
- Following a floating Git revision in a release.
- Confusing recipe and package names.
- Installing files but not assigning them to a package.
- Overriding entire tasks when an append or class hook is enough.
- Editing source under `tmp/work` and expecting persistence.

## Debugging Checklist

- Which layer provides the recipe?
- Which version is selected?
- What are `SRC_URI` and `SRCREV` after expansion?
- What are `${S}` and `${B}`?
- Which classes are inherited?
- Are build dependencies in the recipe sysroot?
- What files are under `${D}`?
- Which package owns each installed file?
- Did QA report unshipped files or host contamination?

## Related Topics

- [Layers](layers.md)
- [Tasks and Workdirs](tasks-and-workdirs.md)
- [Images and Package Groups](images-and-packagegroups.md)
- [Devtool and Recipe Development](devtool-and-recipe-development.md)

## References

- Yocto Project Development Tasks Manual
- Yocto Project Reference Manual
- BitBake User Manual
