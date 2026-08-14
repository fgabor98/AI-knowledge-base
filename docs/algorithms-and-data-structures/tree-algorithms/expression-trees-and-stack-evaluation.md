---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Expression Trees And Stack Evaluation

An expression tree represents operands as leaves and operators as internal nodes. Traversal turns the same tree into different notations, while a stack can evaluate a postfix token sequence without building a tree.

Parsing, representation, and evaluation are separate problems. Keeping them separate makes malformed input, arithmetic failure, and resource limits easier to handle.

## Expression Forms

For the expression `(2 + 3) * 4`:

- infix: `(2 + 3) * 4`
- prefix: `* + 2 3 4`
- postfix: `2 3 + 4 *`

Infix is convenient for people but requires precedence and parenthesis rules. Prefix and postfix encode evaluation order directly. Postfix evaluation needs one stack: push operands, and when an operator appears, pop its operands, compute, and push the result.

## Expression-Tree Model

A binary expression node contains:

- an operator, or an operand value
- a left child for binary operators
- a right child for binary operators

The tree invariant is:

- operand nodes have no children
- operator nodes have exactly the number of children required by the operator
- every child is reachable once from the expression root
- evaluation does not mutate the expression structure

Postorder traversal emits postfix notation because children are emitted before their parent operator.

## Infix-To-Postfix Thinking

A shunting-yard style converter uses:

- an output sequence for operands and completed operators
- an operator stack
- precedence and associativity rules

For each token:

1. Emit operands immediately.
2. Push an opening parenthesis.
3. For an operator, pop operators with higher precedence, or equal precedence when associativity requires it, then push the new operator.
4. On a closing parenthesis, pop until the matching opening parenthesis.
5. At end of input, reject unmatched parentheses and emit remaining operators.

The operator stack needs a documented maximum depth. A parser should not continue after the stack or output buffer is full.

## Stack Evaluation Algorithm

For each postfix token:

1. If it is an operand, push it.
2. If it is an operator, verify that enough operands exist.
3. Pop the right operand, then the left operand.
4. Check division by zero and arithmetic overflow.
5. Push the result.

At the end, exactly one value must remain. Zero values or multiple values mean the token stream was malformed.

## Programming Examples

### C: Bounded Postfix Evaluation

The token representation assumes parsing has already identified numbers and the four binary operators. The evaluator performs no allocation and distinguishes syntax, stack, and arithmetic failures.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    EXPRESSION_MAX_TOKENS = 32,
    EXPRESSION_MAX_STACK = 32
};

enum expression_token_kind {
    EXPRESSION_NUMBER = 0,
    EXPRESSION_OPERATOR
};

struct expression_token {
    enum expression_token_kind kind;
    int32_t number;
    char operator;
};

enum expression_status {
    EXPRESSION_OK = 0,
    EXPRESSION_ERR_NULL,
    EXPRESSION_ERR_TOKEN,
    EXPRESSION_ERR_STACK,
    EXPRESSION_ERR_DIV_ZERO,
    EXPRESSION_ERR_OVERFLOW
};

static int checked_binary(int32_t left,
                          int32_t right,
                          char operator,
                          int32_t *out_result)
{
    int64_t result;

    if (operator == '/' && right == 0)
        return 1;
    if (operator == '/' && left == INT32_MIN && right == -1)
        return 2;

    switch (operator) {
    case '+':
        result = (int64_t)left + right;
        break;
    case '-':
        result = (int64_t)left - right;
        break;
    case '*':
        result = (int64_t)left * right;
        break;
    case '/':
        result = left / right;
        break;
    default:
        return 3;
    }

    if (result < INT32_MIN || result > INT32_MAX)
        return 2;
    *out_result = (int32_t)result;
    return 0;
}

enum expression_status evaluate_postfix(
    const struct expression_token *tokens,
    size_t token_count,
    int32_t *out_value)
{
    int32_t stack[EXPRESSION_MAX_STACK];
    size_t stack_count = 0;

    if (tokens == NULL || out_value == NULL)
        return EXPRESSION_ERR_NULL;
    if (token_count > EXPRESSION_MAX_TOKENS)
        return EXPRESSION_ERR_TOKEN;

    for (size_t i = 0; i < token_count; i++) {
        const struct expression_token *token = &tokens[i];

        if (token->kind == EXPRESSION_NUMBER) {
            if (stack_count == EXPRESSION_MAX_STACK)
                return EXPRESSION_ERR_STACK;
            stack[stack_count++] = token->number;
            continue;
        }

        if (token->kind != EXPRESSION_OPERATOR || stack_count < 2)
            return EXPRESSION_ERR_TOKEN;

        {
            int32_t right = stack[--stack_count];
            int32_t left = stack[--stack_count];
            int32_t result;
            int arithmetic_status = checked_binary(left,
                                                    right,
                                                    token->operator,
                                                    &result);

            if (arithmetic_status == 1)
                return EXPRESSION_ERR_DIV_ZERO;
            if (arithmetic_status != 0)
                return EXPRESSION_ERR_OVERFLOW;
            stack[stack_count++] = result;
        }
    }

    if (stack_count != 1)
        return EXPRESSION_ERR_TOKEN;
    *out_value = stack[0];
    return EXPRESSION_OK;
}
```

The evaluator uses `int64_t` for addition, subtraction, and multiplication intermediates. Division has a special `INT32_MIN / -1` case because the mathematical result is outside the signed 32-bit range.

### C: Postfix Emission From A Simple Tree

```c
enum expression_emit_status {
    EXPRESSION_EMIT_OK = 0,
    EXPRESSION_EMIT_ERR_NULL,
    EXPRESSION_EMIT_ERR_DEPTH,
    EXPRESSION_EMIT_ERR_OUTPUT
};

struct expression_node {
    int is_operator;
    int32_t number;
    char operator;
    size_t left;
    size_t right;
};

static enum expression_emit_status emit_postfix_node(
    const struct expression_node *nodes,
    size_t node_count,
    size_t index,
    struct expression_token *out_tokens,
    size_t out_capacity,
    size_t *out_count)
{
    enum expression_emit_status status;

    if (nodes == NULL || out_tokens == NULL || out_count == NULL)
        return EXPRESSION_EMIT_ERR_NULL;
    if (index >= node_count)
        return EXPRESSION_EMIT_ERR_DEPTH;
    if (!nodes[index].is_operator) {
        if (*out_count == out_capacity)
            return EXPRESSION_EMIT_ERR_OUTPUT;
        out_tokens[(*out_count)++] = (struct expression_token){
            .kind = EXPRESSION_NUMBER,
            .number = nodes[index].number
        };
        return EXPRESSION_EMIT_OK;
    }

    status = emit_postfix_node(nodes,
                               node_count,
                               nodes[index].left,
                               out_tokens,
                               out_capacity,
                               out_count);
    if (status != EXPRESSION_EMIT_OK)
        return status;
    status = emit_postfix_node(nodes,
                               node_count,
                               nodes[index].right,
                               out_tokens,
                               out_capacity,
                               out_count);
    if (status != EXPRESSION_EMIT_OK)
        return status;
    if (*out_count == out_capacity)
        return EXPRESSION_EMIT_ERR_OUTPUT;
    out_tokens[(*out_count)++] = (struct expression_token){
        .kind = EXPRESSION_OPERATOR,
        .operator = nodes[index].operator
    };
    return EXPRESSION_EMIT_OK;
}
```

This helper assumes the node array has already been validated as a tree and that operator nodes have valid children. A complete parser must validate those conditions before emission.

### Python: Reference Postfix Evaluator

```python
def evaluate_postfix(tokens):
    stack = []
    for token in tokens:
        if isinstance(token, int):
            stack.append(token)
            continue
        if token not in {"+", "-", "*", "/"} or len(stack) < 2:
            raise ValueError("malformed postfix expression")

        right = stack.pop()
        left = stack.pop()
        if token == "+":
            result = left + right
        elif token == "-":
            result = left - right
        elif token == "*":
            result = left * right
        else:
            if right == 0:
                raise ZeroDivisionError
            result = int(left / right)
        stack.append(result)

    if len(stack) != 1:
        raise ValueError("malformed postfix expression")
    return stack[0]
```

Python integers do not overflow, so tests for the C evaluator should include explicit values near the 32-bit limits.

## Parsing And Evaluation Boundaries

A parser should report:

- invalid characters or tokens
- missing operands or operators
- unmatched parentheses
- excessive nesting depth
- token-count overflow

An evaluator should report:

- stack underflow or final stack size other than one
- division by zero
- numeric overflow or unsupported operator

Do not let a parser silently produce a malformed token stream for the evaluator to guess about.

## Common Mistakes

- Popping operands in the wrong order for subtraction or division.
- Treating postfix token count as sufficient stack capacity without considering nested depth.
- Using C integer arithmetic without checking overflow.
- Calling postorder output a complete parser without validating syntax.
- Recursing through attacker-controlled expression depth.
- Returning a partial result after an arithmetic or token error.

## Embedded And Systems Angle

- validate expression depth and token count before evaluation
- bound the operand stack and distinguish overflow from malformed syntax
- separate parsing errors from evaluation errors
- use fixed node pools when expression trees must be retained
- decide whether division rounding, integer width, and overflow policy are part of the external contract

## Related Topics

- [Tree Algorithms](index.md)
- [Tree Traversals](tree-traversals.md)
- [Linked Lists Stacks And Queues](../data-structures-for-algorithms/linked-lists-stacks-and-queues.md)
- [Recursion And Stack-Depth Policy](../embedded-linux-algorithmic-constraints/recursion-and-stack-depth-policy.md)
