---
status: draft
reviewed: false
domain: signal-processing/signals-and-systems/programming
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Signals And Systems Programming

## Taxonomy

- Parent: [Signals And Systems](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: practical programming implementation
- Companion pages: [Theory](theory.md), [References](references.md)
- Implementation target: Python with `numpy`, `scipy.signal`, and `matplotlib` unless a later page states otherwise

## Programming Learning Order

### 1. Numerical Setup

- arrays, vectors, matrices, and shape conventions
- complex numbers and phasors
- time vectors and sample grids
- sample period `T`, sample rate `fs`, angular frequency `omega`
- reproducible plots for time-domain, frequency-domain, Bode, Nyquist, and pole-zero views

Implementation outcomes:

- generate step, impulse, rectangular pulse, exponential, sinusoid, damped sinusoid, and complex exponential signals
- plot real part, imaginary part, magnitude, phase, and unwrapped phase

### 2. Signals As Code

- continuous-time signal callables `x(t)`
- sampled signals as arrays `x[k] = x(kT)`
- finite-duration and infinite-duration approximations
- unit step and impulse helpers
- shifting, scaling, and windowing helpers
- energy and power estimates

Implementation outcomes:

- implement `unit_step(k)`, `unit_impulse(k)`, `rect_window(k, k1, k2)`, and `shift(x, k0)`
- approximate energy and average power for finite records

### 3. LTI Simulation

- numerical convolution
- impulse response and step response simulation
- ODE-based simulation for first-order and second-order systems
- response comparison between convolution and ODE solving

Implementation outcomes:

- simulate an RC/RL first-order system
- compute and compare impulse, step, and sinusoidal responses

### 4. State-Space Simulation

- state vector storage
- state update stepping
- matrix exponential solution
- eigenvalue stability checks
- state-space to transfer-function conversion

Implementation outcomes:

- implement a small `StateSpaceSystem`
- simulate stable and unstable systems
- convert a difference equation into state-space form

### 5. Transform Utilities

- numerical Fourier-series coefficients
- DFT and DTFT evaluation on chosen grids
- Laplace-domain transfer evaluation
- pole-zero plotting
- Parseval checks

Implementation outcomes:

- compute Fourier-series coefficients for a square wave
- evaluate `H(jw)` by substituting `s = jw`
- plot how pole locations affect time-domain response

### 6. Nonlinear Solvers

- bisection
- Newton-Raphson
- residual diagnostics
- numerical Jacobians
- explicit and implicit Euler stepping
- small-signal linearization around an operating point

Implementation outcomes:

- solve a diode-resistor operating point
- compare explicit and implicit Euler on a stiff first-order example
- linearize a nonlinear element and measure local approximation error

### 7. Sampling And Reconstruction

- sample continuous functions
- demonstrate aliasing
- reconstruct with zero-order hold and sinc interpolation
- discretize simple continuous systems
- compare continuous and discrete frequency responses

Implementation outcomes:

- sample band-limited and non-band-limited signals
- discretize a first-order analog transfer function using impulse-response sampling and bilinear transform
- verify stability preservation or distortion for each mapping

### 8. Verification

- tests for linearity and time invariance
- causality checks
- stability checks
- impulse-response regression tests
- comparison against `scipy.signal`
- numerical tolerance by signal scale
