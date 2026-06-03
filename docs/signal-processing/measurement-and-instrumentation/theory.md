---
status: draft
reviewed: false
domain: signal-processing/measurement-and-instrumentation/theory
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Measurement And Instrumentation Theory

## Taxonomy

- Parent: [Measurement And Instrumentation](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: theory
- Companion pages: [Programming](programming.md), [References](references.md)
- Foundation dependency: [Signals And Systems Theory](../signals-and-systems/theory.md) and [DSP Theory](../digital-signal-processing/theory.md)

## Theory Learning Order

### 1. Measurement Chain

- physical process
- sensor
- analog signal conditioning
- sample-and-hold
- ADC
- digital processor
- actuator or output path
- measurement range and saturation
- dynamic range

### 2. Error Sources

- offset error
- gain error
- differential nonlinearity
- integral nonlinearity
- finite bandwidth
- harmonic distortion
- intermodulation distortion
- noise
- sampling jitter
- transient settling and overload behavior

### 3. Time-Domain Measurement

- oscilloscope traces
- rise time and fall time
- overshoot and undershoot
- pulse width
- settling time
- time delay
- phase delay
- time-domain reflectometry
- averaging as noise suppression

### 4. Frequency-Domain Measurement

- instrument FFT scaling
- Fourier-series measurement from sampled data
- line spectrum interpretation
- coherent and noncoherent sampling
- leakage and window choice
- amplitude-characteristic measurement
- phase-characteristic measurement
- stepped-sine measurement
- transfer-characteristic measurement

### 5. Measurement Uncertainty

- measurement uncertainty
- uncertainty budgets
- resolution vs accuracy vs precision
- error propagation
- confidence intervals
- calibration traceability
- sampling-clock uncertainty
- uncertainty propagation in compensated measurements

### 6. Compensation And Calibration

- measurement-chain transfer model
- convolution matrix and Toeplitz model
- inverse filtering
- ill-conditioned inversion
- regularization
- Tikhonov compensation
- Wiener filtering
- Kalman filtering
- equivalent-time sampling
- bandwidth extension
- oscilloscope calibration by inverse filtering
- frequency-response-function estimation
- coherence-based measurement validation

### 7. Distributed Measurement Systems

- wireless sensor nodes
- local clocks
- packet timing
- clock offset
- clock drift
- synchronization points
- timestamp transformation
- interpolation-based synchronization correction
- resonator-observer data compression
- active noise control over a measurement loop

### 8. Lab Workflow Concepts

- MATLAB measurement scripts
- DSP-card experiments
- hardware-in-the-loop validation
- recorded-data reproducibility
- low-confidence neural-network spectral classification sidebar
