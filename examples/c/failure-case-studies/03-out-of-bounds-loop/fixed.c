/* Case 03, fixed: derive the bound from the array, use a half-open range. */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

int table[4];

__attribute__((noinline))
bool exists_in_table(int value)
{
    for (size_t i = 0; i < ARRAY_SIZE(table); i++) {
        if (table[i] == value) {
            return true;
        }
    }
    return false;
}

int main(void)
{
    printf("exists_in_table(42) = %d\n", exists_in_table(42));
    printf("exists_in_table(0) = %d\n", exists_in_table(0));
    return 0;
}
