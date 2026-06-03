---
status: draft
reviewed: false
domain: signal-processing/digital-signal-processing/references
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Digital Signal Processing References

## Taxonomy

- Parent: [Digital Signal Processing](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: references
- Companion pages: [Theory](theory.md), [Programming](programming.md)
- Reference scope: sources that directly support DSP algorithms and implementation topics

## Primary Sources

- `02_vimm4084-digit-jelfeldolg.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: sampling, quantization, averaging, recursive DFT, observer interpretation, digital filter design, windowing, spectral estimation, regression, model fitting, LMS, and adaptive IIR systems.
- `03_Sigproc_intro.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: signal classification, distortion sources, sampling, quantization, finite records, DFT/FFT/FA/AFA, inverse filtering, and sensor fusion.
- `05_FA_AFA.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: resonator-based DFT observers, non-DFT frequency grids, leakage/picket-fence mitigation, and frequency adaptation.
- `06_digitalis_kompenzalas.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: measurement-chain compensation, inverse filtering, regularization, equivalent-time sampling, bandwidth extension, and Kalman-filter-based fusion.
- `07_SensorFusion.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: probabilistic fusion, Bayes updates, Kalman filtering, Dempster-Shafer fusion, reliability weighting, and conflict handling.
- `inflab-3m.pdf`
  - Confidence: high, text extraction succeeded.
  - Coverage: LMS, NLMS, FxLMS, adaptive line enhancement, echo cancellation, active noise control, MATLAB exercises, and DSP-card workflow.

## Cross-Topic Sources

- `Régi, de jó jegyzet.pdf`
  - Use for DFT, DTFT, z-transform, sampling, reconstruction, and continuous-to-discrete simulation foundations.
- `meres_04.pdf`
  - Use for FFT scaling, leakage, windowing, and practical spectral-analysis examples.
- `meres_05.pdf`
  - Use for time-domain metrics and averaging examples.
- `inflab-5m.pdf` and `9meres.pdf`
  - Use for distributed synchronization and resonator-observer streaming examples.

## Low-Confidence Source

- `inflab-4m.pdf`
  - Text extraction is damaged by PDF font encoding.
  - Only use as a tentative source for neural-network spectral classification until manually checked.

## Pending Reference Work

- Add page references for major formulas and algorithms.
- Add executable examples under `examples/dsp/`.
- Review numerical formulas against course material before treating them as canonical.
- Add or verify sources for multirate DSP, fast convolution, fixed-point arithmetic, and time-frequency analysis if the current university notes do not cover them deeply enough.
- Add a dedicated source for system identification if chirp, MLS, PRBS, coherence, and frequency-response-function estimation become full lessons.
