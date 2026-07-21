---
status: active
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# 1. Development Process And Kernel Source

Official section: [Working with the kernel development community](https://docs.kernel.org/process/index.html)

Knowledge-guide companion: [Stage 1](knowledge-guide-companion.md#stage-1-development-process-and-kernel-source)

## Orientation

- [ ] **P0** [HOWTO do Linux kernel development](https://docs.kernel.org/process/howto.html)
- [ ] **P0** [A guide to the kernel development process](https://docs.kernel.org/process/development-process.html)
- [ ] **P0** [Linux kernel coding style](https://docs.kernel.org/process/coding-style.html)
- [ ] **P0** [Programming language](https://docs.kernel.org/process/programming-language.html)
- [ ] **P0** [Deprecated interfaces, language features, attributes, and conventions](https://docs.kernel.org/process/deprecated.html)
- [ ] **P0** [Why the `volatile` type class should not be used](https://docs.kernel.org/process/volatile-considered-harmful.html)
- [ ] **P1** [Minimal requirements to compile the kernel](https://docs.kernel.org/process/changes.html)
- [ ] **P1** [Kernel driver statement](https://docs.kernel.org/process/kernel-driver-statement.html)
- [ ] **P1** [The Linux kernel driver interface](https://docs.kernel.org/process/stable-api-nonsense.html)
- [ ] **P1** [Linux kernel licensing rules](https://docs.kernel.org/process/license-rules.html)

## Patches And Upstream Work

- [ ] **P0** [Submitting patches](https://docs.kernel.org/process/submitting-patches.html)
- [ ] **P0** [Linux kernel patch submission checklist](https://docs.kernel.org/process/submit-checklist.html)
- [ ] **P0** [Applying patches](https://docs.kernel.org/process/applying-patches.html)
- [ ] **P0** [Backporting and conflict resolution](https://docs.kernel.org/process/backporting.html)
- [ ] **P0** [Everything about stable releases](https://docs.kernel.org/process/stable-kernel-rules.html)
- [ ] **P1** [Subsystem and maintainer-tree process notes](https://docs.kernel.org/maintainer/index.html)
- [ ] **P1** Read `MAINTAINERS` in the checked-out kernel tree.
- [ ] **P1** Practice `scripts/get_maintainer.pl` on a Device Tree and driver patch.
- [ ] **P1** Read the submission rules for one current project subsystem.

## Documentation And Bindings Contributions

- [ ] **P0** [Writing kernel documentation](https://docs.kernel.org/doc-guide/index.html)
- [ ] **P0** [Kernel-doc comments](https://docs.kernel.org/doc-guide/kernel-doc.html)
- [ ] **P0** [Documentation build](https://docs.kernel.org/doc-guide/sphinx.html)
- [ ] **P1** Build the documentation from the project's exact kernel source tree.
- [ ] **P1** Find the source `.rst` file for five rendered documentation pages.

## Bugs, Regressions, And Security

- [ ] **P0** [Debugging advice for kernel developers](https://docs.kernel.org/process/debugging/index.html)
- [ ] **P1** [Handling regressions](https://docs.kernel.org/admin-guide/reporting-regressions.html)
- [ ] **P1** [Reporting issues](https://docs.kernel.org/admin-guide/reporting-issues.html)
- [ ] **P1** [Security bugs](https://docs.kernel.org/process/security-bugs.html)
- [ ] **P1** [Linux kernel threat model](https://docs.kernel.org/process/threat-model.html)

## Source-Reading Exercises

- [ ] Trace one driver from `Kconfig` to `Makefile`, registration, match table, `probe()`, runtime callbacks, and teardown.
- [ ] Compare the same TI-related driver in upstream and the vendor BSP.
- [ ] Locate every public declaration used by one small platform driver.
- [ ] Use `git log --follow` and `git blame` to explain one non-obvious API pattern.
- [ ] Read the relevant commit discussion or lore thread for one recent API change.

## Completion Notes

```text
Kernel/version:
Completed:
Key lessons:
Source paths studied:
Questions to revisit:
```
