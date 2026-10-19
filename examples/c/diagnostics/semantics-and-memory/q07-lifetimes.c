/* Which of these functions return a pointer that the caller may use? */
const char *name_a(void)
{
    return "sensor";
}

const char *name_b(void)
{
    char name[] = "sensor";
    return name;
}

const int *defaults_c(void)
{
    return (const int[]){ 1, 2, 3 };
}

const int *defaults_d(void)
{
    static const int defaults[] = { 1, 2, 3 };
    return defaults;
}
