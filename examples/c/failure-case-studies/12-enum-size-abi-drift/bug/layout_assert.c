/* The assertion the original header was missing: built with -fshort-enums,
 * this translation unit is rejected instead of silently disagreeing. */
#include "config.h"

_Static_assert(sizeof(struct log_config) == 8, "log_config ABI changed");
