/* Describe install in words, then check: does this compile? */
void (*install(int signal_number, void (*handler)(int)))(int);

static void on_alarm(int signal_number)
{
    (void)signal_number;
}

void (*previous)(int);

void setup(void)
{
    previous = install(14, on_alarm);
}
