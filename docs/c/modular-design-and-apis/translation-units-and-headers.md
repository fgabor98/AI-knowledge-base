---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Translation Units And Headers

A translation unit is the source presented to the compiler after preprocessing. A header is a mechanism for sharing declarations and compile-time definitions; it is not a separately linked module. Good header discipline keeps declarations consistent, dependencies visible, and rebuilds manageable.

## Learning Objectives

- Explain the relationship between source files, headers, translation units, and object files.
- Distinguish declarations, definitions, tentative definitions, and inline definitions.
- Write self-sufficient headers with include guards.
- Control external and internal linkage.
- Prevent dependency cycles and accidental public coupling.
- Inspect preprocessed output and include dependencies.

## Translation Unit Model

Each source file is compiled after its included headers and conditional compilation have been processed:

~~~sh
cc -std=c17 -E driver.c > driver.i
cc -std=c17 -c driver.c -o driver.o
~~~

The compiler checks driver.c and the declarations visible in its translation unit. The linker later combines driver.o with other object files and libraries. A header included in five source files contributes declarations to five translation units; it does not create five copies of a function definition by itself.

A build can therefore fail in several distinct places:

| Stage | Typical failure |
| --- | --- |
| Preprocessing | Missing header, macro branch, include path |
| Compilation | Syntax, type, constraint, or warning failure |
| Assembly | Unsupported instruction or assembler syntax |
| Linking | Missing symbol, duplicate definition, ABI mismatch |
| Startup/runtime | Wrong section initialization or platform contract |

Diagnose the stage before changing source.

## Declarations And Definitions

A declaration describes an entity:

~~~c
/* sensor.h */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

uint16_t sensor_read(void);

#endif
~~~

A definition supplies storage or a body:

~~~c
/* sensor.c */
#include "sensor.h"

uint16_t sensor_read(void)
{
    return 0u;
}
~~~

An external object needs one definition and any number of compatible declarations:

~~~c
/* status.h */
#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

extern uint32_t system_status;

#endif
~~~

~~~c
/* status.c */
#include "status.h"

uint32_t system_status;
~~~

Do not put a non-static object definition in a public header. Each translation unit that includes it may define a separate symbol or create a multiple-definition failure, depending on compiler mode and linker behavior.

## Include Guards

A guard prevents repeated inclusion within one translation unit:

~~~c
#ifndef SENSOR_H
#define SENSOR_H

/* declarations */

#endif
~~~

The guard macro should be unique to the project and path. A project-wide prefix reduces collisions with vendor and system headers.

Pragma once is supported by common compilers but is not an ISO C directive. Use it only when the project’s toolchain policy accepts it. Include guards remain useful for portable headers and generated code.

Include guards do not prevent two different headers from defining the same public name, and they do not solve dependency cycles.

## Self-Sufficient Headers

A header should compile when included first in an otherwise empty translation unit:

~~~c
/* sensor.h */
#ifndef SENSOR_H
#define SENSOR_H

#include <stddef.h>
#include <stdint.h>

struct sensor;

int sensor_read(struct sensor *sensor,
                uint8_t *destination,
                size_t destination_capacity);

#endif
~~~

Test this directly:

~~~c
#include "sensor.h"

int header_smoke_test(void)
{
    return 0;
}
~~~

Include the headers that provide the types used by the declarations. Do not rely on another header happening to be included first.

## Public And Private Headers

Public headers should contain only what callers need:

- stable public types;
- constants that are part of the contract;
- opaque declarations;
- function declarations;
- required standard or platform includes.

Private headers can share implementation details within a subsystem, but keep their inclusion graph narrow. A private header should not leak target registers into portable application code.

Use source-file static functions and objects for details needed by only one translation unit:

~~~c
static int validate_config(const struct config *config)
{
    return config != NULL && config->version != 0u;
}
~~~

## Dependency Direction

Header dependencies should form a deliberate graph:

- foundational types and small utility headers at the bottom;
- domain interfaces above them;
- platform adapters and applications above the interfaces.

Do not make a low-level driver include an application header to obtain a callback type. Move the callback type into the interface layer or use an opaque context.

Forward declarations break some cycles:

~~~c
struct logger;

void driver_set_logger(struct logger *logger);
~~~

A forward declaration is sufficient for a pointer, but not for an embedded object, sizeof, member access, or by-value parameter.

## Inline And Static Definitions

A static inline function in a header gives each translation unit its own internal definition:

~~~c
#ifndef LIMITS_H
#define LIMITS_H

static inline unsigned min_unsigned(unsigned left, unsigned right)
{
    return left < right ? left : right;
}

#endif
~~~

This avoids external duplicate symbols. The function still has a language and compiler linkage contract; do not use inline to hide a large dependency or a function whose address/linkage must be shared.

Macros, typedefs, enum constants, and static inline functions in headers all affect compile time and namespace usage. Keep the public surface intentional.

## Configuration Headers

A generated configuration header should expose validated choices:

~~~c
#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#define PROJECT_BOARD_REV 2
#define PROJECT_FEATURE_DMA 1
#define PROJECT_MAX_CHANNELS 4u

#if PROJECT_MAX_CHANNELS == 0u
#error "PROJECT_MAX_CHANNELS must be nonzero"
#endif

#endif
~~~

Keep generated configuration separate from hand-written public headers. Record the compiler flags, include directories, feature definitions, and board identity used to generate it.

Do not silently provide a default for a safety-critical or hardware-specific choice. Fail at compile time when no safe default exists.

## Include Order

A useful convention is:

1. the source file’s own header;
2. project headers;
3. platform headers;
4. standard library headers.

The exact policy may differ, but including the own header first catches missing self-sufficiency and declaration mismatches:

~~~c
#include "sensor.h"

#include "board_gpio.h"

#include <stddef.h>
#include <stdint.h>
~~~

Use tooling to enforce order and identify unused or transitive includes. Removing an include should not break a header that failed to include its own dependency.

## Exercises

1. Create a two-file module with a public header, private helper, external object, and unit test.
2. Compile every public header as the first include in a smoke-test translation unit.
3. Put a global definition in a header, observe the linker result, then correct it with extern.
4. Draw the include graph of a subsystem and identify cycles and unnecessary dependencies.
5. Generate a configuration header for two boards and make unsupported combinations fail at compile time.
6. Compare preprocessed output before and after removing a transitive include.

## Common Mistakes

- Treating a header as a compiled module.
- Defining global storage or non-inline external functions in headers.
- Relying on transitive includes.
- Omitting include guards.
- Using a forward declaration where a complete type is required.
- Letting application headers leak into drivers.
- Hiding configuration in undocumented command-line defines.
- Including a large platform header in a portable interface.
- Including the own header last instead of first.
- Assuming pragma once is portable ISO C.

## Debugging Checklist

1. Determine whether the failure is preprocessing, compilation, assembly, linking, or startup.
2. Generate preprocessed output and inspect the declarations actually seen.
3. Compile each public header in isolation.
4. Search for definitions in headers and duplicate external symbols.
5. Inspect include paths and generated configuration headers.
6. Draw dependency direction for any include cycle.
7. Verify the source file includes its own public header first.
8. Compare compiler and linker command lines across host and target builds.

## Related Topics

- [Modular Design And APIs overview](./index.md)
- [Preprocessor And Macros](./preprocessor-and-macros.md)
- [APIs And Opaque Types](./api-and-opaque-types.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Language Fundamentals](../language-fundamentals/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC preprocessor options](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html)
- [GCC header dependency generation](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html)
