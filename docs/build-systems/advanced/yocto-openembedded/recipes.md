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

S = "${WORKDIR}/git"

inherit cmake
```

Production recipes should pin source revisions rather than following moving branch heads.

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
PV = "1.2+git${SRCPV}"
```

Avoid floating revisions in release builds. Network state must not silently change source inputs.

## Source And Build Directories

### `${S}`

Source directory.

### `${B}`

Build directory.

For CMake, out-of-tree builds are common. For some upstream projects, `${B}` may equal `${S}`.

Inspect:

```sh
bitbake -e example-app | grep -E '^(WORKDIR|S|B)='
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
    install -m 0644 ${WORKDIR}/example.conf \
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
    install -m 0644 ${WORKDIR}/example.service \
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
