---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Devtool and Recipe Development

## What Problem Does This Solve?

`devtool` provides a controlled workflow for adding recipes, modifying existing recipe sources, testing changes, and exporting those changes into a maintained layer. It bridges generated workdirs and persistent metadata.

## Core Concepts

- `devtool add`
- `devtool modify`
- `devtool build`
- `devtool deploy-target`
- `devtool finish`
- workspace layer
- source tree
- patch export
- recipe update
- reset

## Mental Model

```text
maintained recipe
-> devtool workspace source
-> edit/build/test rapidly
-> convert changes to recipe/patch metadata
-> finish into product layer
-> clean workspace
```

The workspace is temporary development state. The finished layer content is the durable result.

## Workspace Layer

`devtool` creates/uses a workspace layer that overrides normal recipe source handling for active workspace recipes.

Inspect:

```sh
devtool status
bitbake-layers show-layers
```

Do not forget that workspace state changes provider/source behavior compared with a clean build.

## Modifying An Existing Recipe

```sh
devtool modify <recipe>
```

This prepares a source tree for development and configures workspace metadata so builds use it.

Then:

```sh
devtool build <recipe>
```

or:

```sh
bitbake <recipe>
```

Inspect `devtool status` to confirm workspace ownership.

## Source Changes

Use normal source control practices inside the devtool source tree:

- focused commits
- clear commit messages
- separate fixes/features
- no generated output
- no unrelated formatting

Clean commits are easier to export as patches.

## Finishing Changes

```sh
devtool finish <recipe> ../meta-product
```

Depending on recipe/source state, this can update a recipe or create/update an append and patches in the destination layer.

Always review generated metadata:

- patch order
- filenames
- `SRC_URI`
- file search paths
- source revision/version changes
- destination layer ownership

`devtool finish` is not a substitute for code review.

## Resetting Workspace State

```sh
devtool reset <recipe>
```

Reset after finishing so normal layer metadata is tested without workspace overrides.

Then rebuild from the maintained layer to prove the exported change is complete.

## Adding A New Recipe

```sh
devtool add <name> <source-location>
```

`devtool` attempts to infer source/build metadata.

Review:

- license and checksum
- source revision
- build-system class
- dependencies
- install paths
- package split
- service integration
- reproducibility

Auto-generated recipes are starting points.

## Updating A Recipe

`devtool upgrade` can assist with moving to a new upstream version/revision where supported.

Review:

- license changes
- patch applicability
- renamed build options
- dependency changes
- package/file changes
- CVE/security implications

An automated version update still requires full product validation.

## Deploying To A Target

```sh
devtool deploy-target <recipe> root@target
```

Useful for rapid development, but it bypasses full image composition and can make target state differ from a clean image.

After testing:

- use undeploy where appropriate
- rebuild full image
- reflash/redeploy clean image
- validate package/service behavior

Never treat a manually modified target as release evidence.

## Kernel Development With Devtool

Kernel recipes can be modified with devtool workflows, though vendor layers may provide specialized tooling.

Workflow:

1. Identify `virtual/kernel` provider.
2. `devtool modify <kernel-recipe>`.
3. Make focused source commits.
4. Build provider/image.
5. Export patches to product layer.
6. Reset workspace.
7. Rebuild and validate final config/artifacts.

Kernel config fragments may require separate handling from source commits.

## U-Boot Development With Devtool

Apply the same pattern, but validate the complete stage set:

- SPL/TPL
- U-Boot proper
- U-Boot DTB/FIT
- firmware packaging
- saved environment interaction

Deploying only one file can create mixed boot-chain state.

## Patch Ownership

Before finishing, classify each change:

- generic upstream fix
- vendor BSP fix
- custom board support
- product policy
- temporary debug change

Destination and long-term maintenance differ for each category.

## Recipe Development Without Devtool

You can use:

```sh
bitbake -c devshell <recipe>
```

or a separate upstream clone. Devtool is useful but not mandatory.

Whatever workflow you choose, the final result must live in maintained source or metadata, not `tmp/work`.

## Common Mistakes

- Forgetting an active workspace override.
- Finishing generated patches without review.
- Editing devtool source but not committing changes cleanly.
- Deploying to target and skipping full image validation.
- Resetting before preserving changes.
- Mixing source patches and config policy without ownership.
- Testing a kernel/U-Boot component without matching related artifacts.

## Debugging Checklist

- What does `devtool status` show?
- Which source tree is BitBake using?
- Are changes committed and focused?
- Did `devtool build` use expected recipe config?
- What metadata did `finish` generate?
- Was the workspace reset?
- Does a clean layer-only build reproduce the change?
- Was a clean full image deployed after target-side testing?

## Related Topics

- [Recipes](recipes.md)
- [Tasks and Workdirs](tasks-and-workdirs.md)
- [Kernel and Bootloader Integration](kernel-and-bootloader-integration.md)
- [Vendor Kernel Patch Management](../linux-kernel/vendor-kernel-patch-management.md)

## References

- Yocto Project Development Tasks Manual
- Yocto Project devtool documentation
- BitBake User Manual
