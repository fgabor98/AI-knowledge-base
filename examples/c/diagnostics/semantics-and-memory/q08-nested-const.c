/* Why does the first initialization draw a diagnostic, but not the second? */
void inspect(char **argv)
{
    const char **names = argv;
    const char *const *view = (const char *const *)argv;

    (void)names;
    (void)view;
}
