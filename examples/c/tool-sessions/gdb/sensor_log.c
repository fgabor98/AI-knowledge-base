/*
 * A sensor log with a latent memory-corruption bug, used for the GDB session.
 *
 * device_set_name() copies a caller-supplied name without checking it against
 * sizeof dev->name. A long name overwrites ring.head, and the program crashes
 * later, in ring_push(), far from the bad write.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RING_CAPACITY 8u

struct sample {
    uint32_t timestamp;
    int16_t value;
};

struct ring {
    size_t head;
    size_t count;
    struct sample slots[RING_CAPACITY];
};

struct device {
    char name[8];
    struct ring ring;
};

static void device_set_name(struct device *dev, const char *name)
{
    size_t length = strlen(name);

    for (size_t i = 0; i <= length; i++) {   /* no check against sizeof dev->name */
        dev->name[i] = name[i];
    }
}

static void ring_push(struct ring *ring, struct sample sample)
{
    ring->slots[ring->head] = sample;
    ring->head = (ring->head + 1u) % RING_CAPACITY;
    if (ring->count < RING_CAPACITY) {
        ring->count++;
    }
}

static int16_t read_sensor(uint32_t tick)
{
    return (int16_t)(200 + (int)(tick % 7u));
}

int main(int argc, char **argv)
{
    static struct device dev;
    const char *name = argc > 1 ? argv[1] : "temperature-sensor-0";

    device_set_name(&dev, name);
    for (uint32_t tick = 0; tick < 4u; tick++) {
        struct sample sample = { .timestamp = tick, .value = read_sensor(tick) };
        ring_push(&dev.ring, sample);
    }
    printf("%s: %zu samples\n", dev.name, dev.ring.count);
    return 0;
}
