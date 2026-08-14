---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Pipeline And Dataflow Algorithms

A pipeline divides an algorithm into stages connected by data channels. Each stage transforms an item and passes it to the next stage. Dataflow algorithms make this movement explicit: work becomes available when its inputs are ready, rather than when one large sequential procedure reaches a particular line.

Pipelines can improve throughput, isolate responsibilities, and match hardware stages. They also introduce queue capacity, backpressure, ordering, cancellation, and shutdown problems that are part of the algorithm's correctness contract.

## Stage Model

A stage has:

- an input item type and output item type
- a transformation or validation rule
- a bounded processing cost, if required
- a failure status and ownership policy
- an input channel and output channel

A channel transfers ownership or a copy. Decide which before implementation. If a stage keeps a pointer after enqueueing an item, the producer must not reuse the storage until ownership returns.

## Pipeline Invariants

Useful invariants include:

- every queued item belongs to exactly one channel or stage
- a channel count never exceeds capacity
- a stage does not process an item twice
- a failed item is either reported, discarded by policy, or routed to an error channel
- shutdown eventually reaches every stage
- no stage waits forever for input after an upstream terminal condition

These invariants cover both data correctness and lifecycle correctness.

## Backpressure Policies

When a channel is full, choose explicitly:

- block the producer until space is available
- return a full status to the caller
- drop the newest item
- drop the oldest item
- overwrite a ring-buffer slot
- apply upstream throttling
- route the item to an overflow or error path

Dropping is not an implementation detail. It changes the algorithm's result and must be visible to monitoring or callers when data loss matters.

## Ordering

Pipelines often process items out of order even when the input was ordered. Preserve order only when the output contract requires it. Common techniques are:

- attach a sequence number to every input
- allow stages to reorder before final output
- use one worker for ordered sections
- emit completion records and let a merger hold later items until gaps close

Holding results for missing sequence numbers consumes memory and can deadlock if a dropped item is never reported.

## Programming Examples

### C: Bounded Single-Thread Pipeline

This example models acquisition, filtering, and output as explicit stages. The single-thread form makes channel ownership and full behavior visible without hiding synchronization inside a library.

```c
#include <stddef.h>

enum {
    PIPELINE_CAPACITY = 8
};

enum pipeline_status {
    PIPELINE_OK = 0,
    PIPELINE_EMPTY,
    PIPELINE_FULL,
    PIPELINE_ERR_NULL,
    PIPELINE_DROPPED
};

struct sample {
    unsigned int sequence;
    int value;
};

struct sample_queue {
    struct sample items[PIPELINE_CAPACITY];
    size_t head;
    size_t count;
};

static enum pipeline_status sample_queue_push(struct sample_queue *queue,
                                              struct sample item)
{
    size_t index;

    if (queue == NULL)
        return PIPELINE_ERR_NULL;
    if (queue->count == PIPELINE_CAPACITY)
        return PIPELINE_FULL;

    index = (queue->head + queue->count) % PIPELINE_CAPACITY;
    queue->items[index] = item;
    queue->count++;
    return PIPELINE_OK;
}

static enum pipeline_status sample_queue_pop(struct sample_queue *queue,
                                             struct sample *out_item)
{
    if (queue == NULL || out_item == NULL)
        return PIPELINE_ERR_NULL;
    if (queue->count == 0)
        return PIPELINE_EMPTY;

    *out_item = queue->items[queue->head];
    queue->head = (queue->head + 1) % PIPELINE_CAPACITY;
    queue->count--;
    return PIPELINE_OK;
}

enum pipeline_status pipeline_run(const struct sample *input,
                                  size_t input_count,
                                  struct sample *output,
                                  size_t output_capacity,
                                  size_t *out_count,
                                  int minimum_value)
{
    struct sample_queue filter_queue = { 0 };
    size_t written = 0;

    if ((input == NULL && input_count > 0) ||
        (output == NULL && output_capacity > 0) ||
        out_count == NULL)
        return PIPELINE_ERR_NULL;

    for (size_t i = 0; i < input_count; i++) {
        enum pipeline_status status;
        struct sample item;

        status = sample_queue_push(&filter_queue, input[i]);
        if (status != PIPELINE_OK)
            return PIPELINE_FULL;

        while (sample_queue_pop(&filter_queue, &item) == PIPELINE_OK) {
            if (item.value < minimum_value)
                continue;
            if (written == output_capacity)
                return PIPELINE_FULL;
            output[written++] = item;
        }
    }

    *out_count = written;
    return PIPELINE_OK;
}
```

The example drains each item immediately, so its queue never grows beyond one item. A multi-thread implementation would retain the channel between stage workers and must define synchronization and shutdown separately. The fixed queue itself provides O(1) push/pop and O(capacity) storage.

### C: Stage Function With Explicit Failure

```c
enum stage_result {
    STAGE_EMIT = 0,
    STAGE_DROP,
    STAGE_FAIL
};

static enum stage_result filter_sample(struct sample input,
                                       int minimum_value,
                                       struct sample *out_sample)
{
    if (out_sample == NULL)
        return STAGE_FAIL;
    if (input.value < minimum_value)
        return STAGE_DROP;
    *out_sample = input;
    return STAGE_EMIT;
}
```

Making drop and failure distinct lets the pipeline count expected filtering from operational errors.

### Python: Generator Pipeline

```python
def acquire(values):
    for sequence, value in enumerate(values):
        yield sequence, value


def filter_values(samples, minimum):
    for sequence, value in samples:
        if value >= minimum:
            yield sequence, value


def collect(samples):
    return list(samples)


result = collect(filter_values(acquire([3, 8, 2]), minimum=4))
assert result == [(1, 8)]
```

Generators express lazy dataflow but do not model bounded queues or concurrent shutdown. They are useful as a compact semantic reference.

## Fan-Out And Fan-In

Fan-out sends work to several workers. Fan-in combines their results. The design must answer:

- how work is assigned
- whether each item is processed exactly once
- how worker failure is reported
- whether output order matters
- how the merger knows all workers are finished

A completion marker per worker is often simpler than relying on a shared count whose synchronization and lifetime are unclear.

## Cancellation And Shutdown

Cancellation should be checked at bounded points such as:

- before taking a new item
- after a stage completes an item
- before blocking on a full or empty channel

Shutdown should stop new input, drain or discard queued work by policy, wake blocked stages, and release every owned item. A pipeline that computes correct values but cannot terminate is not operationally correct.

## Common Mistakes

- Allowing an inter-stage queue to grow without a limit.
- Dropping items without counting or reporting the loss.
- Preserving output order accidentally by unbounded buffering.
- Blocking a stage while holding a lock needed by the downstream stage.
- Treating stage failure as ordinary end-of-stream.
- Reusing a buffer before the next stage has finished with it.
- Forgetting to wake blocked workers during shutdown.

## Embedded And Systems Angle

- bound every queue between stages
- decide where backpressure, dropping, and retry policy live
- preserve ordering only where the result requires it
- keep interrupt-facing stages short and non-blocking
- design cancellation and shutdown before adding worker threads
- expose queue high-water marks, drops, and stage failures

## Related Topics

- [Parallel And Dataflow Algorithms](index.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Synchronization Costs And Result Merging](synchronization-costs-and-result-merging.md)
- [Interrupt-Safe Queues And Buffers](../embedded-linux-algorithmic-constraints/interrupt-safe-queues-and-buffers.md)
