# Tech Knowledge Base

This repository is a Markdown-first knowledge base for programming and embedded Linux know-how.

The project is intentionally structured like a small documentation codebase:

- `docs/` contains rendered knowledge pages.
- `docs/topic-map.md` owns the taxonomy before full pages are written.
- `docs/topic-page-template.md` contains the standard page structure.
- `examples/` contains executable examples referenced by documentation pages.
- `prompts/` contains reusable AI prompts for controlled content generation.
- `checklists/` contains review criteria for turning drafts into trusted notes.
- `sources/` contains source references, reading notes, and links.
- `scripts/` contains local validation helpers.

## Local Preview

Install dependencies:

```sh
python3 -m pip install -r requirements.txt
```

Serve the site locally:

```sh
mkdocs serve
```

Build the static site:

```sh
mkdocs build
```

Run the Bash documentation and example checks:

```sh
scripts/validate-bash.sh
```

This check expects `shellcheck` to be installed.

Run the C documentation and example checks:

```sh
scripts/validate-c.sh
```

This builds and runs the C examples (`make -C examples/c check sanitize tsan`),
compiles every C code block under `docs/c` (`scripts/check-c-snippets.py`),
checks the test-out answer keys against the compiler (`examples/c/diagnostics/check.py`),
validates the failure case studies (`scripts/validate-c-case-studies.sh`), reruns
the tool sessions quoted in the docs (`examples/c/tool-sessions/*/run.sh`; each checks
that the output the docs quote still appears), and builds the documentation. It expects GCC with AddressSanitizer,
UndefinedBehaviorSanitizer, and ThreadSanitizer support, plus `shellcheck`.

C code blocks in `docs/c` must compile on their own with
`-std=c17 -Wall -Wextra -Wformat=2 -Werror`. A block whose first line is a
file-name comment such as `/* sensor.h */` can be included by later blocks on the
same page. Blocks that are not meant to compile carry a marker comment on the line
above the fence:

```markdown
<!-- c-check: skip -- statement fragment -->
<!-- c-check: expect-warning -->
<!-- c-check: expect-error -->
```

Use `skip` only for fragments, and always give a reason. Use `expect-warning` for
deliberately bad examples; the check then fails if the example stops warning.

## Content Workflow

1. Extend `docs/topic-map.md`.
2. Generate or write outlines first.
3. Create pages from `docs/topic-page-template.md`.
4. Add runnable examples under `examples/` where useful.
5. Review pages using `checklists/page-review.md`.
6. Mark reviewed pages in YAML frontmatter only after checking them.

AI-generated content starts as a draft. Treat it as untrusted until reviewed.
