/* Case 12, fixed: fixed-width storage in the ABI, and a layout assertion. */
#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdint.h>

enum log_level { LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };

struct log_config {
    uint8_t  level;             /* holds an enum log_level value */
    uint8_t  flags;
    uint16_t rate_limit;
};

/* Every translation unit that includes this header re-checks the contract. */
_Static_assert(sizeof(struct log_config) == 4, "log_config ABI changed");
_Static_assert(offsetof(struct log_config, rate_limit) == 2, "log_config ABI changed");

void config_load(struct log_config *config);

#endif
