typedef void (*vector)(void);

extern const vector vector_table[];

int main(void)
{
    vector_table[0]();          /* simulate the UART1 interrupt */
    return 0;
}
