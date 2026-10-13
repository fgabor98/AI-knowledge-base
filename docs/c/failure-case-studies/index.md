---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Failure Case Studies

Fifteen C failures, each reduced to a program of a few dozen lines that you can build
and run. Every case shows the code that fails, what the compiler actually did with it,
the language or toolchain rule involved, which tools caught it and which stayed silent,
and a fix that is checked in CI.

The other chapters explain the rules. This chapter shows what happens when the rules
are broken in code that looks correct, passes review, and works in the debug build.

## How To Use This Chapter

Work through each case in this order:

1. Read only the **Reproducer** source and predict the output at `-O0` and `-O2`.
2. Run the case and compare the result with your prediction:

    ```sh
    bash examples/c/failure-case-studies/01-null-check-after-dereference/run.sh
    ```

3. Before you read **The Rule**, explain the result yourself from the
   generated code.
4. Read **Detection** and note which tools you would have relied on, and whether
   they would have caught it.
5. Answer the **Review Question** for a codebase you work on.

`bash examples/c/failure-case-studies/run-all.sh` runs every case. The
`scripts/validate-c-case-studies.sh` script checks that each buggy program still
compiles and that each fixed program builds with `-Wall -Wextra -Wconversion
-Wsign-conversion -Werror` and runs cleanly under AddressSanitizer and
UndefinedBehaviorSanitizer.

!!! note "Toolchain used for the transcripts"
    All output on these pages comes from GCC 13.3 and binutils 2.42 on x86-64 Ubuntu
    24.04. Other compilers, versions, and targets can give different results.
    Several cases exist to show exactly that: a program whose output depends on the
    compiler version or optimization level has no reliable behavior.

## The Cases

| # | Case | What goes wrong |
| ---: | --- | --- |
| 01 | [NULL Check After Dereference](./01-null-check-after-dereference.md) | A NULL test that follows a dereference is deleted |
| 02 | [Signed Overflow Check](./02-signed-overflow-check.md) | `a + b < a` compiles to `b < 0`, even at `-O0` |
| 03 | [Out-Of-Bounds Loop](./03-out-of-bounds-loop.md) | An off-by-one loop bound makes a lookup always return "found" |
| 04 | [Shift Count Equal To Width](./04-shift-count-width.md) | The same call returns two different values in one binary |
| 05 | [Unsigned Short Promotion](./05-unsigned-short-promotion.md) | `uint16_t * uint16_t` overflows a signed `int`; a check folds to `false` |
| 06 | [read() Error Passes A Length Check](./06-read-error-passes-length-check.md) | `-1 < sizeof(...)` is false, so an error is accepted as a huge frame |
| 07 | [snprintf() Accumulation](./07-snprintf-accumulation.md) | Truncation advances the write offset past the end of the buffer |
| 08 | [Strict Aliasing Type Pun](./08-strict-aliasing-type-pun.md) | A store through `uint32_t *` is invisible to a `float *` reload |
| 09 | [Pointer Into A Reallocated Array](./09-pointer-into-reallocated-array.md) | A cached element pointer survives `realloc()` and reads freed memory |
| 10 | [Struct Padding Leak](./10-struct-padding-leak.md) | `memcpy` serialization sends stale stack bytes |
| 11 | [Spin-Wait Data Race](./11-spin-wait-data-race.md) | A plain `bool` flag is read once; the loop never terminates |
| 12 | [Enum Size ABI Drift](./12-enum-size-abi-drift.md) | Two object files disagree on `sizeof(struct)`; no tool notices |
| 13 | [Misspelled Weak Handler](./13-misspelled-weak-handler.md) | A one-letter typo routes an interrupt to the default handler |
| 14 | [Common Symbol Merge](./14-common-symbol-merge.md) | Two unrelated globals with the same name share storage |
| 15 | [Fast-Math NaN Check](./15-fast-math-nan-check.md) | `-ffast-math` removes an `isnan()` guard |

Cases 01–05 cover undefined behavior that the optimizer relies on. Cases 06–07 cover
integer conversions at API boundaries, 08–10 the object model and lifetimes, 11
concurrency, and 12–15 the build, link, and ABI layer.

## Detection Matrix

What each tool reported **for these reproducers** with the toolchain above. A blank
cell means the tool ran and reported nothing; "n/a" means the tool does not apply.

| # | `-Wall -Wextra` | Other warnings | `-fanalyzer` | UBSan | ASan | TSan / other |
| ---: | --- | --- | --- | --- | --- | --- |
| 01 | | | only if a NULL caller is visible | reports | n/a | |
| 02 | | `-Wstrict-overflow=5`: silent | | reports | n/a | |
| 03 | | `-Warray-bounds=2`: silent | | reports | reports | |
| 04 | | | reports | reports | n/a | |
| 05 | | `-Wconversion`: silent | | reports | n/a | |
| 06 | reports (`-Wextra` only) | | | n/a | n/a | |
| 07 | | | | n/a | reports | `_FORTIFY_SOURCE` aborts |
| 08 | | `-Wstrict-aliasing=1` reports | | | n/a | |
| 09 | | | reports | n/a | reports | |
| 10 | | `-Wpadded` shows the holes | | | hides it | |
| 11 | | | | n/a | n/a | TSan reports at `-O0` |
| 12 | | `-Wlto-type-mismatch`: silent | n/a | n/a | n/a | `_Static_assert` |
| 13 | | `-Wmissing-prototypes` reports | | n/a | n/a | `nm` check |
| 14 | link error (GCC 10+) | `-flto` reports | n/a | n/a | n/a | `-Wl,--warn-common` |
| 15 | | | | | | |

Three patterns appear across the cases:

- **`-Wall -Wextra` reported one case out of fifteen.** It is a minimum,
  not a safety net. Most of the remaining cases need a specific warning, a
  sanitizer run, a static assertion, or a link-time check.
- **A sanitizer can change the behavior it is measuring.** In cases 03 and 05 the
  instrumented build reports the error *and* prints the correct answer, because
  instrumentation changes what the optimizer can assume. In case 10 ASan's stack
  layout zeroes the leaked bytes. Rely on the sanitizer's report, not on whether
  the program's output looks right.
- **Build, link, and ABI failures are invisible to source-level tools.** No
  warning or sanitizer saw case 12 or the `-fcommon` build of case 14. The protection
  has to be part of the build itself: layout assertions, consistent flags, symbol
  checks.

## Adding A Case

A new case belongs here when it meets all of these conditions:

- it reproduces deterministically on the documented toolchain;
- the buggy code is something a competent engineer would write and approve;
- the fix can be checked by `scripts/validate-c-case-studies.sh`;
- the page records which tools did and did not catch it, based on a real run.

Create `examples/c/failure-case-studies/NN-slug/` with `bug.c`, `fixed.c`, and
`run.sh` (or `bug/` and `fixed/` directories for multi-file cases). Then add a page
with the same section structure as the existing ones.

## Related Topics

- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [Compiler Optimization And Undefined Behavior](../advanced-c/compiler-optimization-and-undefined-behavior.md)
- [Sanitizers And Dynamic Analysis](../correctness-quality-and-security/sanitizers-and-dynamic-analysis.md)
- [Static Analysis](../correctness-quality-and-security/static-analysis.md)
- [Senior-Level Outcomes](../professional-and-capstone/senior-level-outcomes.md)

## References

- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf);
  clause numbers on these pages refer to it
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [GCC instrumentation options (sanitizers)](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
- [GCC static analyzer options](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html)
