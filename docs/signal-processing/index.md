---
status: draft
reviewed: false
domain: signal-processing
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Signal Processing

This is the umbrella topic for the university-note-driven signal processing curriculum.

The structure separates mathematical foundations, DSP algorithms, measurement practice, and communication systems so each topic can grow without forcing everything under Digital Signal Processing.

## Topic Structure

- [Signals And Systems](signals-and-systems/index.md): mathematical foundation for continuous-time and discrete-time signals, systems, transforms, state-space models, stability, and simulation.
- [Digital Signal Processing](digital-signal-processing/index.md): programmable DSP algorithms, spectral analysis, digital filters, adaptive filters, compensation, sensor fusion, and reproducible DSP tests.
- [Measurement And Instrumentation](measurement-and-instrumentation/index.md): sensors, ADC/DAC measurement chains, oscilloscope workflows, timing, synchronization, calibration, transfer measurement, and lab data processing.
- [Infocommunication](infocommunication/index.md): communication channels, coding, modulation, pulse shaping, ISI, OFDM, broadcast, mobile, and VoIP context.
- [Authoring Instructions](authoring-instructions.md): terminology, taxonomy, and page-consistency rules for AI-generated signal-processing pages.
- [Diagram Instructions](diagram-instructions.md): Markdown-renderable diagram rules for all signal-processing pages.

## Learning Order

1. Signals And Systems
2. Digital Signal Processing
3. Measurement And Instrumentation
4. Infocommunication

The order is not strict after the first step. Measurement and Infocommunication both reuse DSP, but they emphasize different applications.

## Source Batches

- Signals and Systems 1 and 2 notes provide the mathematical base.
- The Digital Signal Processing note and DSP lecture slides provide algorithmic DSP topics.
- Measurement lab guides provide practical instrumentation and real-time workflow topics.
- Infocommunication notes provide coding, modulation, channel, and system-level communications context.

## Page Split Rules

- Put transform theory, system equations, and stability concepts in Signals And Systems.
- Put finite-record algorithms, filters, adaptive methods, implementation, and tests in Digital Signal Processing.
- Put instrument workflows, sensors, timing, calibration, and measurement-chain correction in Measurement And Instrumentation.
- Put modulation, coding, channels, and communication system architecture in Infocommunication.
- If a concept appears in multiple branches, explain it once in its foundation page and link to it from application pages.

## Subtopic Page Pattern

Each subtopic uses the same four-page structure:

- `index.md`: taxonomy, scope, and learning order.
- `theory.md`: concepts, definitions, algorithms, equations, and design tradeoffs in learning order.
- `programming.md`: practical implementation path, exercises, deliverables, and verification.
- `references.md`: source PDFs, extraction confidence, and pending reference work.

## Terminology Rule

When introducing a new technical term in a signal-processing page, include the correct Hungarian term in square brackets on first appearance. See [Authoring Instructions](authoring-instructions.md).
