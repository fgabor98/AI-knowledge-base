# Tech Knowledge Base

This repository is a Markdown-first knowledge base for programming and embedded Linux know-how.

The project is intentionally structured like a small documentation codebase:

- `docs/` contains rendered knowledge pages.
- `docs/topic-map.md` owns the taxonomy before full pages are written.
- `docs/templates/` contains the standard page structure.
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

## Content Workflow

1. Extend `docs/topic-map.md`.
2. Generate or write outlines first.
3. Create pages from `docs/topic-page-template.md`.
4. Add runnable examples under `examples/` where useful.
5. Review pages using `checklists/page-review.md`.
6. Mark reviewed pages in YAML frontmatter only after checking them.

AI-generated content starts as a draft. Treat it as untrusted until reviewed.
