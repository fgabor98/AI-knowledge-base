---
status: draft
reviewed: false
domain: signal-processing/measurement-and-instrumentation/references
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Measurement And Instrumentation References

## Taxonomy

- Parent: [Measurement And Instrumentation](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: references
- Companion pages: [Theory](theory.md), [Programming](programming.md)
- Reference scope: sources that support lab, instrument, measurement-chain, and synchronization topics

## Primary Sources

- `06_digitalis_kompenzalas.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: measurement-chain compensation, inverse filtering, regularization, equivalent-time sampling, bandwidth extension, and Kalman-filter-based fusion.
- `9meres.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: distributed measurement systems, wireless sensor networks, sampling/processing timing, synchronization, and interpolation-based correction.
- `inflab-5m.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: timestamp transformation, resonator-observer data compression, and active noise control in a wireless/DSP lab setup.
- `meres_04.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: frequency-domain signal analysis, DFT/FFT scaling, line spectra, windowing, leakage, and transfer-characteristic measurement.
- `meres_05.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: time-domain signal analysis, pulse metrics, phase measurement, transfer measurement, time-domain reflectometry, and averaging.

## Secondary Sources

- `02_vimm4084-digit-jelfeldolg.pdf`
  - Use for theoretical backing of averaging, DFT filters, spectral estimation, and model fitting.
- `inflab-3m.pdf`
  - Use for DSP-card workflow and adaptive noise-control lab topics.

## Low-Confidence Source

- `inflab-4m.pdf`
  - Text extraction is damaged by PDF font encoding.
  - Tentative coverage: neural-network spectral classification.

## Pending Reference Work

- Add page references for lab workflows and figures.
- Manually confirm `inflab-4m.pdf`.
- Decide where measured-data examples should live under `examples/`.
