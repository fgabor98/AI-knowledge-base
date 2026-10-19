#include "config.h"

void config_load(struct log_config *config)
{
    config->level = LOG_WARN;
    config->flags = 0x01;
    config->rate_limit = 500;
}
