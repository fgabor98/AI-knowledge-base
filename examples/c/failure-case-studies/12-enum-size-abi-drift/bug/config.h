/* Case 12: a public struct whose layout depends on the enum size. */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

enum log_level { LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };

struct log_config {
    enum log_level level;       /* 4 bytes by default, 1 with -fshort-enums */
    uint8_t        flags;
    uint16_t       rate_limit;
};

void config_load(struct log_config *config);

#endif
