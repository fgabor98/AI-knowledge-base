#include <stdio.h>

struct config {
    int mode;
    int thresholds[3];
    int retries;
};

int main(void)
{
    struct config config = { 1, 2 };
    int table[6] = { [4] = 7, 8, [1] = 9 };

    printf("%d %d %d |", config.thresholds[0], config.thresholds[1], config.retries);
    for (int i = 0; i < 6; i++) {
        printf(" %d", table[i]);
    }
    putchar('\n');
    return 0;
}
