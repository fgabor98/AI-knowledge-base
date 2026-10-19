/* Exactly one of the three marked lines is rejected. Which one? */
void update(void)
{
    int value = 1;
    int other = 2;
    const int *read_only = &value;
    int *const fixed = &value;

    read_only = &other;     /* line A */
    *fixed = 3;             /* line B */
    fixed = &other;         /* line C */
    (void)read_only;
}
