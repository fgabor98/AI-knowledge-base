/* Written later by someone else: a counter that happens to share the name. */
unsigned int rx_buffer;

void stats_count(void)
{
    rx_buffer++;
}

unsigned int stats_get(void)
{
    return rx_buffer;
}
