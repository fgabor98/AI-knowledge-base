---
status: draft
reviewed: false
domain: signal-processing/signals-and-systems/theory
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Signals And Systems Theory

## Taxonomy

- Parent: [Signals And Systems](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: theory
- Companion pages: [Programming](programming.md), [References](references.md)
- Downstream users: DSP filters and transforms, measurement-chain modeling, communication channels and modulation

## Theory Learning Order

### 1. Signal Vocabulary

- continuous-time signals `x(t)`
- discrete-time signals `x[k]`
- deterministic vs stochastic signals
- periodic, quasiperiodic, aperiodic, and transient signals
- unit step, Dirac impulse, Kronecker impulse, and rectangular windows
- shifted, scaled, delayed, and modulated signals
- even and odd decomposition
- signal energy, average power, mean, variance, RMS, and bandwidth

### 2. Continuous-Time LTI Systems

- linearity, time invariance, causality, and stability
- impulse response and step response
- convolution integral
- first-order and second-order systems
- natural response and forced response
- BIBO stability and asymptotic stability
- transfer characteristic `H(jw)` and transfer function `H(s)`

### 3. State-Space Models

- state variables and state vector
- continuous-time state equation
- discrete-time state equation
- matrix exponential
- eigenvalues and modes
- transfer-function/state-space relationship
- impulse and step response from state-space
- stability from eigenvalues

### 4. Transform Foundations

- Fourier series
- Fourier transform
- Laplace transform
- DFT
- DTFT
- z-transform
- inverse transforms
- Parseval relations
- convolution, delay, modulation, and frequency-shift theorems
- relation between `H(s)`, `H(jw)`, `H(z)`, and `H(e^jnu)`

### 5. Frequency-Domain System Analysis

- sinusoidal steady state
- phasors and complex amplitudes
- impedance and admittance
- poles and zeros
- Bode diagrams
- Nyquist diagrams
- all-pass systems
- minimum-phase systems
- distortion-free transmission
- bandwidth and filter tolerance schemes

### 6. Discrete-Time Systems

- discrete-time signal-flow networks
- gain, delay, adder, splitter, source, and sink blocks
- difference equations
- ARMA-style system equations
- impulse response and discrete convolution
- direct-form and state-space descriptions
- discrete-time sinusoidal steady state
- stability by roots and eigenvalues inside the unit circle

### 7. Nonlinear Systems

- nonlinear resistive and dynamic components
- canonical nonlinear algebraic equations
- operating point
- dynamic resistance, capacitance, and inductance
- small-signal linearization
- explicit Euler and implicit Euler
- bisection and Newton-Raphson
- convergence and local model validity

### 8. Sampling And Reconstruction

- sampling theorem
- aliasing
- time-domain and frequency-domain sampling views
- stochastic signal sampling
- band-limited assumptions
- zero-order hold reconstruction
- ideal sinc reconstruction
- continuous-to-discrete mappings
- impulse-response sampling, `z = exp(sT)`, Euler mapping, and bilinear transform
