---
status: draft
reviewed: false
domain: signal-processing/authoring-instructions
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Signal Processing Authoring Instructions

## Taxonomy

- Parent: [Signal Processing](index.md)
- Page type: authoring/reference support
- Applies to: Signals And Systems, Digital Signal Processing, Measurement And Instrumentation, and Infocommunication
- Purpose: keep AI-generated signal-processing pages consistent, traceable, and bilingual where useful

## Hungarian Terminology Rule

When introducing a new technical term, write the correct Hungarian term in square brackets immediately after the English term on its first appearance on that page.

Examples:

- sampling theorem [mintavételi tétel]
- impulse response [impulzusválasz]
- finite impulse response filter [véges impulzusválaszú szűrő]
- inter-symbol interference [szimbólumközi áthallás]
- measurement uncertainty [mérési bizonytalanság]

Apply the rule once per page for each term. After the first definition, use the English term normally unless the Hungarian term is needed for clarity.

For acronyms, introduce the full term, Hungarian term, and acronym together:

- discrete Fourier transform [diszkrét Fourier-transzformáció, DFT]
- phase-locked loop [fáziszárt hurok, PLL]

Do not guess Hungarian terminology. If the correct Hungarian term is not known from the provided course material, verify it before publishing the page. If verification is still pending in a draft, mark it explicitly as `[Hungarian term pending]` and add the term to the page's reference-review tasks.

Do not translate variable names or standard mathematical symbols. Translate the concept name, not the notation.

## Page Consistency Rules

- Every subtopic page must state its taxonomy near the top.
- Theory pages must present topics in learning order.
- Programming pages must present implementation topics in learning order and include concrete deliverables.
- References pages must state source confidence and extraction limits.
- If a term belongs primarily to another subtopic, link to that subtopic instead of redefining it deeply.
- Prefer terminology used in the Hungarian university notes when it is available.
