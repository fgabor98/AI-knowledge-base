---
status: active
reviewed: false
domain: build-systems
difficulty: advanced
last_reviewed: null
---

# Debugging, Testing, Reproducibility, And CI

Official reading for turning a build into evidence: useful diagnostics, tests,
stable inputs, cache behavior, and repeatable outputs.

## Official Reading

- [ ] **P0** [GNU Make debugging](https://www.gnu.org/software/make/manual/make.html): `-n`, `-p`, `-d`, jobs, and environment/configuration inspection.
- [ ] **P0** [CMake testing documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html): test discovery, execution, reports, and dashboards.
- [ ] **P0** [Reproducible Builds documentation](https://reproducible-builds.org/docs/): timestamps, paths, locales, archives, toolchains, and verification.
- [ ] **P0** [Yocto reproducible builds documentation](https://docs.yoctoproject.org/test-manual/reproducible-builds.html): diffoscope workflows, sstate, source dates, and known variation.
- [ ] **P1** [GCC instrumentation and sanitizer options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html): coverage, sanitizers, and runtime diagnostics.
- [ ] **P1** [ccache documentation](https://ccache.dev/manual/latest.html): cache keys, direct mode, compiler checks, and correctness boundaries.
- [ ] **P1** [diffoscope documentation](https://diffoscope.org/): structured comparison of build outputs.

## Practice And Evidence

- [ ] Capture a failing build with its exact command, environment, dependency
  versions, and first meaningful diagnostic.
- [ ] Run unit, integration, static-analysis, sanitizer, and artifact checks at
  the layer where each question can be answered most directly.
- [ ] Build the same source twice in isolated directories and compare outputs;
  explain every intentional difference.
- [ ] Test a cold build, a warm cache build, and a source-mirror/offline build.
- [ ] Make CI publish logs, manifests, checksums, tool versions, and the exact
  source/configuration revisions needed for investigation.

## Exit Criteria

- [ ] I can turn a build failure into a minimal reproducible case.
- [ ] I can tell whether a cache improved performance without changing the
  correctness or provenance of the result.
- [ ] I can demonstrate reproducibility or document the remaining non-determinism.
