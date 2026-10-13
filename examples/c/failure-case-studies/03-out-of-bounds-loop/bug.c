/* Case 03: an off-by-one loop bound turns a lookup into "always found". */
#include <stdbool.h>
#include <stdio.h>

int table[4];

__attribute__((noinline))
bool exists_in_table(int value)
{
    for (int i = 0; i <= 4; i++) {      /* reads table[4] on the last pass */
        if (table[i] == value) {
            return true;
        }
    }
    return false;
}

int main(void)
{
    printf("exists_in_table(42) = %d\n", exists_in_table(42));
    return 0;
}
