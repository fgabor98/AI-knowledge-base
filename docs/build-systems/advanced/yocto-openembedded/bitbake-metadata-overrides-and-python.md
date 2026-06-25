---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# BitBake Metadata, Overrides, and Python

## What Problem Does This Solve?

BitBake metadata is composed from many files, operators, overrides, classes, and Python functions. Understanding when and where values change is necessary to explain the final build instead of guessing from one recipe file.

## Assignment Operators

### Immediate Assignment

```bitbake
A = "one"
B := "${A}"
A = "two"
```

`B` captures the value expanded when `:=` is parsed, while deferred `=` values are expanded when used.

Use immediate expansion when the current value must be frozen. Avoid it as a reflex because it changes composition behavior.

### Default Assignment

```bitbake
MODE ?= "release"
```

Sets `MODE` only if it is not already defined.

Weak default:

```bitbake
MODE ??= "release"
```

Weak defaults interact differently with later operations. Inspect the final value and history when layers combine both forms.

### Append And Prepend

```bitbake
DEPENDS += " libfoo"
SRC_URI:append = " file://product.patch"
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
```

Whitespace is your responsibility. For list-like values, write examples so concatenation cannot merge tokens.

### Removal

```bitbake
PACKAGECONFIG:remove = "feature-x"
```

Use removal for deliberate policy, but investigate why the original item appears. Excessive removals can hide ownership problems.

## Overrides

Overrides scope metadata to active contexts.

Examples:

```bitbake
SRC_URI:append:product-board = " file://board.patch"
PACKAGECONFIG:append:class-native = " host-feature"
RDEPENDS:${PN}:append = " runtime-package"
```

Common override dimensions include:

- machine
- distro
- architecture/tune
- package name
- native/nativesdk/target class
- task context

Inspect active overrides:

```sh
bitbake -e <recipe> | grep '^OVERRIDES='
```

## Worked Example: Machine-Specific Configuration File

Layer layout:

```text
recipes-example/example/
  example_1.0.bb
  files/common.conf
  files/product-board/board.conf
```

Recipe:

```bitbake
SRC_URI += "file://common.conf"
SRC_URI:append:product-board = " file://board.conf"

do_install() {
    install -d ${D}${sysconfdir}/example
    install -m 0644 ${WORKDIR}/common.conf \
        ${D}${sysconfdir}/example/common.conf
}

do_install:append:product-board() {
    install -m 0644 ${WORKDIR}/board.conf \
        ${D}${sysconfdir}/example/board.conf
}
```

Verify:

```sh
bitbake -e example | grep '^SRC_URI='
bitbake example -c install
find "$(bitbake -e example | sed -n 's/^D="\(.*\)"/\1/p')" -type f
```

In practice, query variables using a reliable shell helper or inspect the printed path directly rather than embedding fragile parsing in production scripts.

## Variable Flags

Flags attach metadata to variables or tasks.

Examples:

```bitbake
do_compile[depends] += "tool-native:do_populate_sysroot"
do_deploy[dirs] = "${DEPLOYDIR}"
MYVAR[doc] = "Explains product metadata"
```

Common task flags affect:

- dependencies
- working directory
- network access
- fakeroot behavior
- task execution
- signature calculation

Use task flags only after understanding class-provided behavior.

Inspect flags in `bitbake -e` output or with BitBake datastore tools appropriate to the release.

## Shell Functions

```bitbake
do_install:append() {
    install -d ${D}${datadir}/product
    printf '%s\n' "${PRODUCT_ID}" > ${D}${datadir}/product/id
}
```

BitBake expands metadata variables before executing the generated shell task. Quote shell values and avoid host-specific commands unless provided as native dependencies.

## Python Functions

BitBake-style Python function:

```bitbake
python do_validate_product_metadata() {
    machine = d.getVar("MACHINE")
    product = d.getVar("PRODUCT_ID")
    if not product:
        bb.fatal("PRODUCT_ID must be set for %s" % machine)
}

addtask validate_product_metadata before do_configure after do_patch
```

`d` is the BitBake datastore for the current context.

Common APIs:

```python
d.getVar("NAME")
d.setVar("NAME", "value")
d.appendVar("NAME", " value")
bb.note("message")
bb.warn("message")
bb.fatal("message")
```

Keep Python tasks deterministic and explicit about files/dependencies.

## Inline Python

```bitbake
PRODUCT_SUFFIX = "${@'-dbg' if d.getVar('DEBUG_BUILD') == '1' else ''}"
```

Inline Python is useful for small expressions. Complex logic belongs in named functions/classes for readability and testing.

## Anonymous Python

```bitbake
python __anonymous() {
    if d.getVar("MACHINE") == "product-board":
        d.appendVar("PACKAGECONFIG", " board-feature")
}
```

Anonymous Python runs during parsing. It can make metadata difficult to trace and increases parse-time complexity.

Prefer overrides or declarative metadata when possible.

## Python Helpers

Define reusable helpers in classes or library modules rather than copying anonymous Python across recipes.

Example helper function:

```bitbake
def product_artifact_name(d):
    return "%s-%s" % (d.getVar("MACHINE"), d.getVar("DISTRO_VERSION"))

PRODUCT_ARTIFACT = "${@product_artifact_name(d)}"
```

Scope and syntax depend on where the helper is defined; validate with parsing and `bitbake -e`.

## Datastore Inspection

The primary tool:

```sh
bitbake -e <recipe>
```

Worked investigation:

```sh
bitbake -e virtual/kernel > kernel.env
grep -n '^SRC_URI=' kernel.env
grep -n '^KERNEL_DEVICETREE=' kernel.env
grep -n '^OVERRIDES=' kernel.env
```

Variable history comments near values often identify contributing files and operators.

## Worked Example: Feature Controlled By Distro And Machine

Machine:

```bitbake
MACHINE_FEATURES:append = " can"
```

Distro:

```bitbake
DISTRO_FEATURES:append = " product-diagnostics"
```

Recipe:

```bitbake
PACKAGECONFIG ??= ""
PACKAGECONFIG:append = " ${@bb.utils.contains('MACHINE_FEATURES', 'can', 'can', '', d)}"
PACKAGECONFIG:append = " ${@bb.utils.contains('DISTRO_FEATURES', 'product-diagnostics', 'diagnostics', '', d)}"

PACKAGECONFIG[can] = "-Denable-can=true,-Denable-can=false,libsocketcan"
PACKAGECONFIG[diagnostics] = "-Ddiagnostics=true,-Ddiagnostics=false"
```

Inspect final `PACKAGECONFIG` and generated configure command.

## Task Signature Implications

Metadata read by a task can influence its signature. Python code that reads undeclared external state can create nondeterminism or incorrect cache reuse.

Avoid reading:

- current wall clock without policy
- arbitrary host files
- uncontrolled environment variables
- network resources
- mutable external directories

Model inputs as metadata, fetched sources, dependencies, or declared files.

## Common Mistakes

- Assuming `+=` and `:append` are identical.
- Forgetting whitespace in append operations.
- Using anonymous Python where an override suffices.
- Modifying metadata during task execution and expecting parse-time graph changes.
- Reading uncontrolled host state from Python.
- Using obsolete override syntax from an older Yocto release.
- Inspecting one recipe file instead of final datastore history.

## Debugging Checklist

- What is the final variable value?
- Which files/operators contributed?
- Which overrides are active?
- Is assignment immediate or deferred?
- Are spaces correct after append/prepend?
- Is Python running at parse time or task time?
- Does Python read deterministic declared inputs?
- Did metadata changes alter task signatures as expected?

## Related Topics

- [Recipes](recipes.md)
- [Layers](layers.md)
- [Tasks and Workdirs](tasks-and-workdirs.md)
- [Debugging BitBake Builds](debugging-bitbake-builds.md)

## References

- BitBake User Manual metadata syntax
- BitBake datastore API documentation
- Yocto Project Reference Manual
