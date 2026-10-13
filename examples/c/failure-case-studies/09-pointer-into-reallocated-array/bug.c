/* Case 09: a pointer cached into a growable array outlives a realloc(). */
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

    list_push(&list, "alpha");
    const char **first = &list.items[0];    /* cached element pointer */
    list_push(&list, "beta");
    list_push(&list, "gamma");              /* grows: the array may move */

    printf("first = %s\n", *first);
    free(list.items);
    return 0;
}
