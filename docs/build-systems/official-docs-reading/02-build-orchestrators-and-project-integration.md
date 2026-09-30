---
status: active
reviewed: false
domain: build-systems
difficulty: intermediate
last_reviewed: null
---

# Build Orchestrators And Project Integration

Official reading for declarative project descriptions, generated backends,
configuration, testing, installation, and package consumption.

## Official Reading

- [ ] **P0** [GNU Make manual](https://www.gnu.org/software/make/manual/make.html): variables, functions, pattern rules, implicit rules, parallel execution, recursive Make, and debugging.
- [ ] **P0** [CMake reference and guides](https://cmake.org/cmake/help/latest/): the tutorial, buildsystem, language, generators, toolchains, packages, install rules, presets, CTest, and CPack.
- [ ] **P0** [Ninja manual](https://ninja-build.org/manual.html): the low-level build graph, pools, command lines, restat behavior, and regeneration.
- [ ] **P1** [Meson manual](https://mesonbuild.com/Manual.html): project options, dependencies, cross files, built-in targets, tests, and installation.
- [ ] **P1** [GNU Autoconf manual](https://www.gnu.org/software/autoconf/manual/autoconf.html) and [GNU Automake manual](https://www.gnu.org/software/automake/manual/automake.html): configure tests, generated files, portability, and install targets.

## Practice And Evidence

- [ ] Configure one project with CMake and build it with both Make and Ninja.
- [ ] Inspect the generated graph and identify which file owns each command.
- [ ] Build a small Meson project with a cross file and a test target.
- [ ] Explain when a project should expose a cache/configuration option,
  environment variable, toolchain file, or package-configured dependency.
- [ ] Run an install into a staging prefix and verify that the installed files
  are relocatable or intentionally target-specific.

## Exit Criteria

- [ ] I can choose an appropriate build description and generator for a project.
- [ ] I can debug configuration-time discovery separately from build-time and
  install-time failures.
