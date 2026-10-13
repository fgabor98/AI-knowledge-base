/* Case 09, fixed: remember positions, not addresses, in growable storage. */
#include <stdio.h>
#include <stdlib.h>

struct name_list {
    const char **items;
    size_t count;
    size_t capacity;
};

static int list_push(struct name_list *list, const char *item)
{
    if (list->count == list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 2;
        const char **grown = realloc(list->items, capacity * sizeof *grown);

        if (grown == NULL) {
            return -1;
        }
        list->items = grown;
        list->capacity = capacity;
    }
    list->items[list->count++] = item;
    return 0;
}

int main(void)
{
    struct name_list list = { 0 };

    if (list_push(&list, "alpha") != 0) {
        return 1;
    }
    size_t first = 0;                       /* index stays valid across growth */
    if (list_push(&list, "beta") != 0 || list_push(&list, "gamma") != 0) {
        free(list.items);
        return 1;
    }

    printf("first = %s\n", list.items[first]);
    free(list.items);
    return 0;
}
