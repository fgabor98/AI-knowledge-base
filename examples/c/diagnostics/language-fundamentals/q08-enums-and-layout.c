#include <stdio.h>

enum mode { MODE_A, MODE_B = 5, MODE_C, MODE_D = MODE_B };

struct first { char tag; int value; char flag; };
struct second { int value; char tag; char flag; };

int main(void)
{
    printf("%d %d %zu %zu\n", MODE_C, MODE_D,
           sizeof(struct first), sizeof(struct second));
    return 0;
}
