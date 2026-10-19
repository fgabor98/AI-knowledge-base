---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 13. Misspelled Weak Handler

Startup files declare every interrupt handler as a weak alias of a default handler, and
the application overrides one by defining a function with the same name. If the name is
misspelled, the application defines a new, unused function, and the weak default stays
in the vector table. The project compiles, links, and boots normally.

## Symptom

A peripheral "never interrupts", or the system ends up in the default handler's
infinite loop as soon as the interrupt fires. Breakpoints in the driver's handler are
never hit. The driver code looks correct because it *is* correct. It is just never
called.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/13-misspelled-weak-handler/bug/vectors.c"
```

```c
--8<-- "examples/c/failure-case-studies/13-misspelled-weak-handler/bug/uart.c"
```

`main.c` calls `vector_table[0]()` to simulate the UART interrupt:

```text
$ gcc -O2 -Wall -Wextra bug/vectors.c bug/uart.c bug/main.c -o bug && ./bug
default_handler: unexpected interrupt
```

## What The Linker Did

```text
$ nm bug | grep -E "handler|hander"
0000000000001170 T default_handler
0000000000001170 W timer0_irq_handler
0000000000001180 T uart1_irq_hander
0000000000001170 W uart1_irq_handler
```

`uart1_irq_handler` is still `W` (weak), at the same address as `default_handler`.
`uart1_irq_hander` (missing `l`) is an ordinary global function that nothing
references. The linker does exactly what weak symbols are for: a weak definition is
used when no strong definition with the same name exists.

## The Rule

Weak symbols are not ISO C. They are an ELF feature (`STB_WEAK`) exposed by GCC's
`__attribute__((weak))` and used by CMSIS and most vendor startup files. In C, defining
a function with external linkage that nobody declares or calls is legal, so there is
nothing for the compiler to reject.

## Why Review And Testing Missed It

- One transposed or missing letter in a long, all-caps vendor name such as
  `USART1_IRQHandler` is hard to see.
- Host unit tests call the driver function directly by its (misspelled) name and pass.
- The link succeeds. Unused strong symbols are normal, and `--gc-sections` may remove
  the misspelled function silently.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra`, `-fanalyzer` | silent |
| `-Wmissing-prototypes` | `no previous prototype for 'uart1_irq_hander'` |
| `nm` on the linked image | the handler is `W` at the default handler's address |

`-Wmissing-prototypes` works here because a correctly spelled handler would have had
a prototype from a shared header. The fix is to make sure one exists.

## Fix

Declare every handler in one header that both the vector table and the drivers
include, and turn the missing-prototype warning into an error:

```c
--8<-- "examples/c/failure-case-studies/13-misspelled-weak-handler/fixed/handlers.h"
```

```text
$ gcc -O2 -Wall -Wextra -Werror=missing-prototypes \
      fixed/vectors.c fixed/uart.c fixed/main.c -o fixed && ./fixed
uart1: byte received
```

A misspelled definition now has no prototype and breaks the build.

## Keep It Fixed

Add a post-link check that every handler the product requires is implemented:

```bash
--8<-- "examples/c/failure-case-studies/13-misspelled-weak-handler/fixed/check_handlers.sh"
```

```text
$ check_handlers.sh fixed uart1_irq_handler
ok
$ check_handlers.sh bug uart1_irq_handler
error: uart1_irq_handler is not implemented (resolves to default_handler)
```

For production firmware, give the default handler a distinguishable behavior, such as
recording the active vector number (from `IPSR` on Cortex-M) before halting. An
unexpected interrupt then produces a diagnosis instead of a hang.

## Same Root Cause Elsewhere

- Weak callbacks in vendor SDKs, such as `HAL_UART_RxCpltCallback` and the other
  `__weak` callbacks in STM32Cube HAL: a typo silently keeps the empty default.
- Two strong definitions where one was meant to be weak: a link error, which is the
  safe failure.
- Static libraries: a strong override in an archive member that nothing else
  references is never pulled in, and the weak default wins even when the spelling is
  correct.

## Review Question

> For each weak symbol this image is supposed to override, what proves the override
> was linked?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Startup, Reset, And Vector Tables](../embedded-c-and-hardware/startup-reset-and-vector-tables.md)
- [Object Files, Symbols, And Relocations](../compilation-linking-and-abi/object-files-symbols-and-relocations.md)
- [ARM Cortex-M](../platform-specific-c/arm-cortex-m.md)

## References

- [GCC common function attributes: `weak`, `alias`](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [GCC `-Wmissing-prototypes`](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [CMSIS 6 (startup files and weak default handlers)](https://github.com/ARM-software/CMSIS_6)
