---
status: draft
reviewed: false
domain: signal-processing/digital-signal-processing/programming
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Digital Signal Processing Programming

## Taxonomy

- Parent: [Digital Signal Processing](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: practical programming implementation
- Companion pages: [Theory](theory.md), [References](references.md)
- Implementation target: Python first; later pages may add embedded C, fixed-point, or DSP-card workflows

## Programming Learning Order

### 1. DSP Project Setup

- `numpy`, `scipy.signal`, `matplotlib`
- signal-generation helpers
- plotting helpers for time, spectrum, phase, pole-zero, Bode, and Nyquist views
- deterministic test signals
- numerical tolerance conventions

Deliverables:

- `signals.py`
- `plotting.py`
- golden signals for impulse, step, sinusoid, exponential, and chirp

### 2. Sampled Signal Utilities

- sample grids
- finite records
- windows
- energy and power estimates
- autocorrelation and RMS helpers
- quantizer helper functions

Deliverables:

- sample a callable `x(t)` into `x[k]`
- compute energy, power, mean, variance, and RMS
- quantize a signal and compute quantization error

### 3. Frequency Response And Transform Tools

- DFT and inverse DFT usage
- DTFT grid evaluation
- `freqz`-style response from `b, a`
- phase wrapping and unwrapping
- Parseval verification

Deliverables:

- `freq_response(b, a, nu_grid)`
- DFT-bin-to-frequency mapping
- phase-delay and group-delay plotting helpers

### 4. Practical FFT Analyzer

- DFT normalization
- coherent vs noncoherent sampling experiments
- windowed FFT
- amplitude scaling for sine peaks
- zero padding and interpolation
- periodogram and averaged spectral estimates
- recursive DFT bin
- zoom FFT

Deliverables:

- window comparison notebook or script
- periodogram variance demo
- sliding recursive DFT demo
- narrowband zoom FFT demo

### 5. Digital Filter Implementation

- direct-form FIR
- direct-form I/II IIR
- transposed structures
- second-order sections
- filter state and initial conditions
- coefficient normalization
- numerical robustness checks

Deliverables:

- FIR implementation tested against direct convolution
- IIR implementation tested against `scipy.signal.lfilter`
- SOS implementation compared with direct form

### 6. Digital Filter Design

- low-pass, high-pass, band-pass, and band-stop specifications
- IIR design with Butterworth, Chebyshev, and elliptic families
- bilinear transform and prewarping
- FIR window-method design
- equiripple/Remez FIR design
- FIR/IIR comparison

Deliverables:

- design one specification as FIR and IIR
- compare order, phase, runtime, and numerical behavior
- commit plots generated from scripts

### 7. Sampling, Quantization, And Reconstruction

- aliasing simulation
- anti-alias filtering
- zero-order hold
- sinc reconstruction
- continuous-to-discrete system mappings
- SQNR vs bit depth
- dither experiment

Deliverables:

- aliasing visualization
- ZOH vs sinc reconstruction comparison
- SQNR sweep
- dithered vs non-dithered error spectrum

### 8. Model Fitting And Adaptive Filters

- least-squares FIR identification
- LMS
- NLMS
- LMS-Newton or recursive correlation estimate
- adaptive IIR approximation
- filtered-x LMS
- line enhancement
- echo cancellation
- active noise control

Deliverables:

- FIR identification from noisy input/output pairs
- LMS vs NLMS convergence comparison
- filtered-x LMS secondary-path simulation
- active noise control toy example

### 9. Compensation And Fusion

- inverse filtering with noise amplification
- Tikhonov regularization sweep
- Wiener-filter example
- scalar Kalman filter
- multisensor Kalman fusion
- Bayesian update example
- Dempster-Shafer mass-combination example

Deliverables:

- low-pass measurement-chain compensation demo
- regularization tradeoff plot
- two-sensor fusion demo with different noise variances

### 10. Test And Review Harness

- property tests for linearity and time invariance
- causality checks
- stability assertions
- pole-radius checks
- golden-signal regression tests
- comparison with `scipy.signal`
- reproducible plot generation

Deliverables:

- test suite for FIR, IIR, DFT, and adaptive-filter examples
- script-based generation for any committed numeric plots
