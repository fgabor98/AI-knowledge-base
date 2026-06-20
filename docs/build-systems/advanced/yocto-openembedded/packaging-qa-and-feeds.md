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

Yocto normally scans ELF files and generates runtime shared-library dependencies automatically.

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

Common QA findings include:

- installed but not shipped
- files/directories owned incorrectly
- host paths/RPATH contamination
- missing runtime dependencies
- development files in runtime package
- stripped binaries unexpectedly
- invalid license metadata
- architecture mismatch
- build paths embedded in output

Treat QA as a correctness signal. Do not broadly disable checks to make a build green.

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
