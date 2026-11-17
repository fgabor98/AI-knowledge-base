---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Arrays, Strings, And Buffers

Arrays provide contiguous storage for a fixed number of elements. A string is a convention—a sequence of characters terminated by a null character—not a distinct built-in C type. A buffer may contain text, binary data, samples, register images, or partially filled DMA storage. The type alone does not tell you its length, capacity, encoding, or ownership.

## Learning Objectives

- Declare and initialize arrays with known bounds.
- Explain when an array converts to a pointer.
- Pass buffers with explicit length and capacity.
- Distinguish strings from byte buffers.
- Handle multidimensional arrays and row-major layout.
- Recognize the difference between sizeof, capacity, and string length.
- Design bounded operations for embedded input and output paths.

## Array Objects

An array contains a fixed number of elements of one type:

~~~c
#include <stdint.h>

uint16_t samples[8];
uint8_t packet[64];
char label[12];
~~~

The array object owns storage for every element. sizeof samples is the size of all eight elements, and sizeof packet is 64 C bytes.

Initialization can infer the bound:

~~~c
#include <stdint.h>

static const uint8_t header[] = {0xA5u, 0x5Au, 0x01u};
static const char name[] = "sensor";
~~~

The first array has three elements. The second has seven: six visible characters plus the terminating null.

## Array-To-Pointer Conversion

In most expressions an array converts to a pointer to its first element:

~~~c
#include <stddef.h>
#include <stdint.h>

void inspect(void)
{
    uint8_t bytes[4] = {1u, 2u, 3u, 4u};
    uint8_t *first = bytes;
    (void)first;

    size_t element_count = sizeof bytes / sizeof bytes[0];
    (void)element_count;
}
~~~

Important exceptions include sizeof, unary address-of, and a string literal used to initialize an array. Once an array is passed to a function, the function receives a pointer and cannot infer the caller’s bound.

<!-- c-check: expect-warning -->

~~~c
#include <stddef.h>
#include <stdint.h>

void receive(uint8_t data[32], size_t length)
{
    (void)sizeof data;
    for (size_t i = 0u; i < length; ++i) {
        data[i] = 0u;
    }
}
~~~

The bound in a parameter declaration is documentation unless another mechanism enforces it.

## Buffer Contracts

A robust mutable-buffer interface names pointer, count, and capacity:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool append_byte(uint8_t *buffer,
                 size_t capacity,
                 size_t *length,
                 uint8_t value)
{
    if (buffer == NULL || length == NULL || *length >= capacity) {
        return false;
    }

    buffer[*length] = value;
    ++*length;
    return true;
}
~~~

The contract is that buffer points to writable storage with capacity elements, length points to the initialized count, and 0 <= length <= capacity on entry. The function writes one element only on success. The caller retains ownership.

For binary data, zero is a valid byte and cannot mark the end. Use pointer plus count for a view of existing storage, and pointer plus capacity plus current length for a mutable builder.

## Strings

A C string is a character sequence ending at the first null character:

~~~c
#include <stddef.h>

size_t bounded_string_length(const char *text, size_t capacity)
{
    if (text == NULL) {
        return 0u;
    }

    for (size_t i = 0u; i < capacity; ++i) {
        if (text[i] == '\0') {
            return i;
        }
    }

    return capacity;
}
~~~

Returning capacity means no terminator was found within the inspected range; the caller must decide whether that is an error.

For a writable character array:

~~~c
char name[8] = "sensor";
~~~

There is room for six characters, the terminator, and one remaining zero element. Writing eight non-null characters leaves no terminator; writing a ninth overflows.

A string literal initializes an array but must not be modified through a pointer:

~~~c
const char *message = "ready";
char mutable_message[] = "ready";
~~~

The first refers to non-writable literal storage; the second creates a writable array.

## sizeof Versus strlen

sizeof is an object or type size operation. strlen scans for a terminator and requires a valid string:

~~~c
#include <string.h>

void compare_sizes(void)
{
    char text[16] = "abc";
    size_t capacity = sizeof text;
    size_t length = strlen(text);
    (void)capacity;
    (void)length;
}
~~~

When text is a pointer, sizeof reports pointer size, not the referenced storage size. Never call strlen on a binary buffer or on input not proven to contain a terminator.

## Multidimensional Arrays

A multidimensional array is an array of arrays:

~~~c
#include <stddef.h>
#include <stdint.h>

static void clear_matrix(uint16_t matrix[3][4])
{
    for (size_t row = 0u; row < 3u; ++row) {
        for (size_t column = 0u; column < 4u; ++column) {
            matrix[row][column] = 0u;
        }
    }
}
~~~

The parameter is adjusted to a pointer to an array of four uint16_t elements. The second dimension is required for address calculation. C stores elements in row-major order.

Variable-length array parameters can express relationships between dimensions:

~~~c
#include <stddef.h>
#include <stdint.h>

void clear_matrix_n(size_t rows, size_t columns,
                    uint16_t matrix[rows][columns])
{
    for (size_t row = 0u; row < rows; ++row) {
        for (size_t column = 0u; column < columns; ++column) {
            matrix[row][column] = 0u;
        }
    }
}
~~~

These declarations still do not validate storage. Check multiplication for overflow before allocating or indexing a dynamically sized matrix.

## Binary Buffers

Binary buffers need an explicit length and representation contract:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool read_frame(const uint8_t *frame, size_t frame_length,
                uint16_t *sequence)
{
    if (frame == NULL || sequence == NULL || frame_length < 2u) {
        return false;
    }

    *sequence = (uint16_t)(((uint16_t)frame[0] << 8) | frame[1]);
    return true;
}
~~~

The function does not assume the frame is a structure in memory. It checks length and decodes defined byte order explicitly.

For DMA or peripheral buffers, also document producer, consumer, ownership handoff, cache maintenance, alignment, access width, concurrent modification, and when the length becomes stable.

## Flexible Array Members

A flexible array member is an incomplete array at the end of a structure:

~~~c
#include <stddef.h>
#include <stdint.h>

struct message {
    uint16_t type;
    uint16_t length;
    uint8_t payload[];
};

size_t message_size(size_t payload_length)
{
    return offsetof(struct message, payload) + payload_length;
}
~~~

The structure does not include payload storage. Allocate enough contiguous storage, validate arithmetic overflow, and obey lifetime and alignment rules. The member must be last.

## Memory Functions And Overlap

The standard byte-copy functions have strict contracts:

- memcpy requires non-overlapping regions;
- memmove supports overlap;
- memcmp compares bytes, not semantic values;
- memset writes one repeated byte and is not a universal typed initializer.

For structures with padding, memcmp may report inequality even when named members compare equal.

## Exercises

1. Append bytes to a bounded buffer and test empty, full, and one-byte-free cases.
2. Implement a length-prefixed frame parser rejecting truncated and overlong frames.
3. Create a two-dimensional sample buffer and verify row-major indexing.
4. Copy a string into caller-provided storage and report truncation without losing termination.
5. Demonstrate why sizeof works for a local array but not an array parameter.
6. Design a DMA-buffer contract naming producer, consumer, ownership, alignment, and cache operations.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: a bounded append with edge-case tests"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <string.h>

    struct byte_buffer {
        uint8_t *data;
        size_t capacity;
        size_t length;
    };

    /* All or nothing: appends every byte, or leaves the buffer unchanged. */
    bool buffer_append(struct byte_buffer *buffer, const uint8_t *bytes, size_t count)
    {
        if (count > buffer->capacity - buffer->length) {   /* cannot overflow: length <= capacity */
            return false;
        }
        memcpy(buffer->data + buffer->length, bytes, count);
        buffer->length += count;
        return true;
    }

    int main(void)
    {
        uint8_t storage[4];
        struct byte_buffer buffer = { storage, sizeof storage, 0 };
        const uint8_t two[2] = { 1, 2 };
        const uint8_t one[1] = { 3 };

        assert(buffer_append(&buffer, two, 0));          /* empty append on an empty buffer */
        assert(buffer.length == 0);
        assert(buffer_append(&buffer, two, 2));
        assert(buffer_append(&buffer, one, 1));          /* now exactly one byte free */
        assert(!buffer_append(&buffer, two, 2));         /* one free, two needed: rejected */
        assert(buffer.length == 3);                      /* ...and nothing was written */
        assert(buffer_append(&buffer, one, 1));          /* fills the last byte */
        assert(buffer.length == buffer.capacity);
        assert(!buffer_append(&buffer, one, 1));         /* full */
        assert(buffer_append(&buffer, one, 0));          /* zero bytes still fit when full */
        return 0;
    }
    ```

    The check `count > capacity - length` cannot overflow, because `length <= capacity` is
    the buffer's invariant. The tempting `length + count > capacity` can wrap when `count`
    comes from untrusted input. The tests cover the three named cases and the edges around
    them: zero bytes, exactly one byte free, a request one byte too large, and full.

??? note "Exercise 2: a length-prefixed frame parser"
    Frame format: one length byte, then that many payload bytes, at most 16.

    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    #define PAYLOAD_MAX 16u

    enum parse_result { PARSE_OK, PARSE_TRUNCATED, PARSE_TOO_LONG, PARSE_EMPTY };

    struct frame {
        const uint8_t *payload;        /* points into the input buffer */
        size_t length;
    };

    enum parse_result parse_frame(const uint8_t *input, size_t available,
                                  struct frame *frame, size_t *consumed)
    {
        if (available == 0) {
            return PARSE_EMPTY;
        }
        size_t length = input[0];
        if (length > PAYLOAD_MAX) {
            return PARSE_TOO_LONG;           /* reject before reading the payload */
        }
        if (length > available - 1) {        /* available >= 1 here, so no underflow */
            return PARSE_TRUNCATED;
        }
        frame->payload = input + 1;
        frame->length = length;
        *consumed = 1 + length;
        return PARSE_OK;
    }

    int main(void)
    {
        struct frame frame;
        size_t consumed;
        const uint8_t good[] = { 3, 'a', 'b', 'c', 9 };
        const uint8_t truncated[] = { 4, 'a', 'b' };
        const uint8_t too_long[] = { 17, 0 };
        const uint8_t zero_length[] = { 0 };

        assert(parse_frame(good, sizeof good, &frame, &consumed) == PARSE_OK);
        assert(frame.length == 3 && frame.payload[0] == 'a' && consumed == 4);
        assert(parse_frame(truncated, sizeof truncated, &frame, &consumed) == PARSE_TRUNCATED);
        assert(parse_frame(too_long, sizeof too_long, &frame, &consumed) == PARSE_TOO_LONG);
        assert(parse_frame(zero_length, sizeof zero_length, &frame, &consumed) == PARSE_OK);
        assert(frame.length == 0 && consumed == 1);
        assert(parse_frame(good, 0, &frame, &consumed) == PARSE_EMPTY);
        return 0;
    }
    ```

    - The declared length is checked against the protocol maximum *and* against the bytes
      actually available, before any payload byte is read.
    - `TRUNCATED` and `TOO_LONG` are different results on purpose. A stream reader waits for
      more bytes after `TRUNCATED`, but must resynchronize or drop the connection after
      `TOO_LONG`, because waiting cannot fix it.
    - The payload is returned as a view into the input, so its lifetime is the caller's
      buffer. The function never copies or allocates.
    - Run the tests under ASan and UBSan. A fuzzer is the next step for any real parser.

??? note "Exercise 3: a two-dimensional sample buffer"
    ```c
    #include <assert.h>
    #include <stddef.h>
    #include <stdint.h>

    #define CHANNELS 3
    #define SAMPLES 4

    int main(void)
    {
        int16_t samples[CHANNELS][SAMPLES];

        for (size_t channel = 0; channel < CHANNELS; channel++) {
            for (size_t i = 0; i < SAMPLES; i++) {
                samples[channel][i] = (int16_t)(channel * 100 + i);
            }
        }

        /* Row-major: the last index varies fastest, and rows are contiguous.
           Byte offsets within the whole array object are always well defined. */
        const unsigned char *base = (const unsigned char *)samples;
        for (size_t channel = 0; channel < CHANNELS; channel++) {
            for (size_t i = 0; i < SAMPLES; i++) {
                size_t offset = (channel * SAMPLES + i) * sizeof(int16_t);
                assert((const unsigned char *)&samples[channel][i] == base + offset);
            }
        }
        assert(sizeof samples == CHANNELS * SAMPLES * sizeof(int16_t));
        assert(sizeof samples[0] == SAMPLES * sizeof(int16_t));   /* one row */
        return 0;
    }
    ```

    `samples[channel][i]` is element `channel * SAMPLES + i` of one contiguous block, and
    `samples[channel]` is a whole row of `SAMPLES` elements. Loop with the last index
    innermost, as here, so memory is accessed sequentially. Swapping the loops strides
    through memory by a whole row on every step.

    The check compares byte offsets through `unsigned char *`, because any object may be
    inspected as bytes. A shortcut such as `const int16_t *flat = &samples[0][0];` followed
    by `flat[channel * SAMPLES + i]` is how much code treats a 2-D array, and compilers
    accept it. Strictly, though, `flat` points into the first row, and the standard defines
    its arithmetic only within that row of `SAMPLES` elements.

??? note "Exercise 4: a terminating string copy that reports truncation"
    ```c
    #define _POSIX_C_SOURCE 200809L   /* for strnlen before C23 */
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <string.h>

    /* Copies src into dst (capacity bytes). Returns false if the result was truncated.
       dst is always terminated when capacity > 0. */
    bool copy_string(char *dst, size_t capacity, const char *src)
    {
        if (capacity == 0) {
            return src[0] == '\0';
        }
        size_t length = strnlen(src, capacity);   /* stops at capacity: src may be long */
        if (length == capacity) {
            memcpy(dst, src, capacity - 1);
            dst[capacity - 1] = '\0';
            return false;
        }
        memcpy(dst, src, length + 1);             /* includes the terminator */
        return true;
    }

    int main(void)
    {
        char name[6];

        assert(copy_string(name, sizeof name, "adc0") && strcmp(name, "adc0") == 0);
        assert(copy_string(name, sizeof name, "adc01") && strcmp(name, "adc01") == 0);  /* exactly fits */
        assert(!copy_string(name, sizeof name, "adc012") && strcmp(name, "adc01") == 0); /* one too long */
        assert(copy_string(name, sizeof name, "") && name[0] == '\0');
        assert(!copy_string(name, 0, "x"));
        return 0;
    }
    ```

    - `strncpy` is the wrong tool: it does not terminate when the source is too long, and it
      pads short copies with zeros.
    - `strnlen` keeps the function from reading past `capacity` bytes of a long or
      unterminated source. It is POSIX, so before C23 it needs the `_POSIX_C_SOURCE`
      feature-test macro under `-std=c17`; C23 adds it to the standard library.
    - The boundary tests are the ones that matter: a string that fits exactly (5 characters
      plus terminator in 6 bytes), and one that is one character too long.
    - Reporting truncation lets the caller decide. For a file path, truncation is an error.
      For a log message, it is usually acceptable.

??? note "Exercise 5: sizeof on a local array versus an array parameter"
    Inside the function that defines it, an array keeps its type, so `sizeof` gives the size
    of the whole array. A parameter declared as an array is adjusted to a pointer (C11
    6.7.6.3p7), so `sizeof` gives the size of a pointer, whatever number appears in the
    brackets. GCC warns: `'sizeof' on array function parameter will return size of
    'unsigned char *'`.

    The fix is to pass the length with the pointer and compute it where the array type is
    still visible:

    ```c
    #include <stddef.h>
    #include <stdint.h>

    #define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

    void clear(uint8_t *data, size_t length)
    {
        for (size_t i = 0; i < length; i++) {
            data[i] = 0u;
        }
    }

    void reset_frame(void)
    {
        uint8_t frame[32];

        clear(frame, ARRAY_SIZE(frame));
    }
    ```

    `ARRAY_SIZE` itself gives the wrong answer if it is applied to a pointer, which is why it
    belongs only where the array is declared. See also
    [test-out Q7](./test-out.md#q7-sizes-of-strings).

??? note "Exercise 6: a DMA-buffer contract"
    An example contract for a UART receive buffer filled by DMA, on a core with a data cache
    and 32-byte cache lines:

    | Item | Contract |
    | --- | --- |
    | Producer | the DMA controller writes received bytes |
    | Consumer | the UART driver's task-level code, after the transfer-complete interrupt |
    | Ownership | `CPU` → `DEVICE` when the driver starts a transfer; `DEVICE` → `CPU` only when the transfer-complete interrupt fires, or after the driver has stopped the channel and waited for it to go idle. The CPU must not read or write the buffer while the device owns it. |
    | Size | 256 bytes, a multiple of the cache-line size |
    | Alignment | 32 bytes, so no other object shares its cache lines |
    | Placement | the `.dma_buffers` section, in RAM that the DMA controller can reach |
    | Cache, before the transfer | *invalidate* the buffer's lines, so no dirty line is written back over DMA data later |
    | Cache, after the transfer | *invalidate* again before reading, so the CPU does not see stale cached bytes |
    | Cancellation | stop the channel, wait until it reports idle, then take ownership back |
    | Error path | a DMA error interrupt returns ownership to the CPU and marks the data invalid |

    ```c
    #include <stdalign.h>
    #include <stdint.h>

    #define CACHE_LINE 32u
    #define RX_DMA_SIZE 256u

    _Static_assert(RX_DMA_SIZE % CACHE_LINE == 0, "DMA buffer must be whole cache lines");

    void cache_invalidate(void *address, uint32_t length);
    void dma_start_rx(uint8_t *buffer, uint32_t length);

    /* Owned by the UART driver; see the contract table. */
    __attribute__((section(".dma_buffers")))
    static alignas(CACHE_LINE) uint8_t rx_dma_buffer[RX_DMA_SIZE];

    /* CPU -> DEVICE: discard cached lines, then hand the buffer to the DMA engine. */
    void uart_rx_start(void)
    {
        cache_invalidate(rx_dma_buffer, RX_DMA_SIZE);
        dma_start_rx(rx_dma_buffer, RX_DMA_SIZE);
    }
    ```

    The alignment and size rules exist because cache maintenance works on whole lines. If an
    unrelated variable shared the buffer's last line, invalidating that line would discard
    the variable's latest value. Writing the contract down also answers the review questions
    for every function that touches the buffer: which state must it be in, and who moves it to
    the next one? [DMA, Cache, And Memory Barriers](../embedded-c-and-hardware/dma-cache-and-memory-barriers.md)
    covers the mechanisms in detail.

## Common Mistakes

- Using sizeof(pointer) where the array size was intended.
- Treating an array parameter bound as runtime validation.
- Forgetting a null terminator or writing past capacity.
- Calling string functions on binary or unterminated input.
- Modifying a string literal.
- Passing overlapping objects to memcpy.
- Treating structure layout as a serialized packet format.
- Multiplying dimensions without checking size_t overflow.
- Reading while DMA or another task is writing.
- Assuming memset zero creates valid values for every object type.

## Debugging Checklist

1. Log pointer, length, capacity, and ownership at every boundary.
2. Assert or test length <= capacity before indexing.
3. Test empty, full, maximum, truncated, and unterminated input.
4. Use address and undefined-behavior sanitizers in host builds when possible.
5. Check whether an array has decayed before applying sizeof.
6. Check source/destination overlap for every copy.
7. Verify encoding and terminator policy.
8. On target, inspect DMA descriptors, cache state, alignment, and handoff.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Types, Values, And Objects](./types-values-and-objects.md)
- [Structures, Unions, And Enumerations](./structures-unions-and-enums.md)
- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [C string handling — cppreference](https://en.cppreference.com/w/c/string/byte)
- [CERT C array rules](https://wiki.sei.cmu.edu/confluence/display/c)
