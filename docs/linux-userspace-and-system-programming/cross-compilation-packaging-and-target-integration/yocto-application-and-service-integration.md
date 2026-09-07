---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Yocto Application And Service Integration

A Yocto recipe is the integration contract between source, toolchain, package contents, and image composition. Keep application build logic in the upstream build system where possible, and use the recipe to express target-specific dependencies, installation, packaging, and service integration.

Important recipe concerns include:

- `SRC_URI`, checksums, source revision, and license metadata;
- `DEPENDS` for build-time dependencies and `RDEPENDS` for runtime dependencies;
- `do_compile` and `do_install` with the target staging paths;
- `FILES:${PN}` for every installed file that belongs in a package;
- `inherit systemd`, `SYSTEMD_SERVICE:${PN}`, and enablement policy;
- separate development, debug, and runtime packages;
- layer priority and override behavior.

Do not hide missing dependencies by using host tools or absolute build-machine paths. Test with a clean build directory, inspect the package manifest, and verify the generated image rather than assuming a successful task means a usable target service. Record the layer revisions and configuration that produced the image.

For systemd integration, the unit must agree with the service user, state paths, device dependencies, watchdog/readiness behavior, sandbox, and update policy. A unit that works in a development shell may fail in the image because its users, directories, capabilities, or loader dependencies were not packaged.

## An integration fragment

The following is an illustrative fragment for an existing, source-pinned recipe
whose CMake project installs both the executable and the unit. It omits source
and license declarations, which remain required:

```bitbake
inherit cmake pkgconfig systemd useradd

DEPENDS += "systemd"

SYSTEMD_SERVICE:${PN} = "example.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

USERADD_PACKAGES = "${PN}"
USERADD_PARAM:${PN} = "--system --no-create-home --user-group example"

FILES:${PN} += "${systemd_unitdir}/system/example.service"
```

The unit must say `User=example`, and installation must use the distribution's
unit-directory variables. `DEPENDS` is appropriate here if the program links
libsystemd for notifications. It stages build dependencies; it is not a blanket
runtime package declaration. Shared-library dependencies are generally detected
during packaging, while commands or plugins invoked dynamically may require
explicit `RDEPENDS:${PN}`.

Use syntax and class behavior from the selected Yocto release. The source
unpack/work layout and available variables can vary between releases.
See [Yocto classes](https://docs.yoctoproject.org/ref-manual/classes.html#systemd).

## Verify each integration boundary

```sh
bitbake example
oe-pkgdata-util list-pkg-files example
bitbake your-image
```

Check that image composition actually includes the package: successfully building
a recipe alone does not add it to an image. Inspect effective metadata when an
override does not apply, then verify installed files, service account, enablement
and readiness on the target.

Use `RuntimeDirectory`/`StateDirectory` or appropriate init policy for
runtime paths. File installation, packaging, image selection, boot enablement and
application readiness are five distinct checks. Keep manual target repairs out
of the release proof by rebuilding them into the recipe or image policy.

## Related topics

- [Stage 15 overview](index.md)
- [ELF and loader diagnosis](../diagnostics-debugging-and-performance/elf-abi-and-loader-diagnostics.md)
