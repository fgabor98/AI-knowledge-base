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
