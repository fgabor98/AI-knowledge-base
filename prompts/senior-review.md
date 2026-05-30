# Prompt: Senior Review

Use this prompt to review a drafted page before marking it as trusted.

```text
Review this Markdown page as a senior embedded Linux and systems engineer.

Focus on:
- factual inaccuracies
- missing edge cases
- dangerous oversimplifications
- misleading examples
- missing prerequisites
- weak debugging advice
- places where primary references are needed
- claims that should be tested with executable examples

Output:
1. Blocking correctness issues.
2. Important missing material.
3. Suggested concrete edits.
4. Example or test coverage to add.
5. Whether this page is ready to mark reviewed.

Do not rewrite the full page unless asked.
```
