---
status: draft
reviewed: false
domain: signal-processing/digital-signal-processing/theory
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Digital Signal Processing Theory

## Taxonomy

- Parent: [Digital Signal Processing](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: theory
- Companion pages: [Programming](programming.md), [References](references.md)
- Foundation dependency: [Signals And Systems Theory](../signals-and-systems/theory.md)

## Theory Learning Order

### 1. DSP Signal Model

- sampled signal arrays
- sample index, sample period, and sample rate
- finite records and observation windows
- deterministic and stochastic signal models
- mean, variance, autocorrelation, autocovariance, and power spectral density
- stationarity and ergodicity

### 2. Sampling And Quantization

- analog-to-digital representation
- sampling theorem and aliasing from a DSP implementation viewpoint
- under- and oversampling
- quantizer step size and bit depth
- quantization error
- additive quantization-noise model
- quantization theorems
- Sheppard corrections
- dither
- white-noise-spectrum conditions

### 3. Averaging And Elementary Filters

- ideal averaging
- recursive and prediction-correction averaging
- moving-window average
- exponential average
- frequency response of averaging filters
- pole-zero view of moving-average and exponential-average filters

### 4. DFT And Spectral Estimation

- DFT as a computable finite-record transform
- DFT as a filter bank
- DFT filters and comb filters
- recursive DFT
- DFT as an observer
- FFT analyzer
- band-selective and zoom FFT
- spectrum shifting
- DFT interpolation and zero padding
- spectral leakage, picket-fence effect, and scalloping loss
- window functions and equivalent noise bandwidth
- periodogram
- periodogram variance
- Bartlett and Welch averaging
- autocorrelation and cross-correlation estimation

### 5. Digital Filters

- FIR and IIR systems
- direct forms and second-order sections
- pole-zero design interpretation
- numerical robustness in recursive filters
- IIR design from analog prototypes
- Butterworth, Chebyshev, and elliptic filters
- bilinear transform and frequency prewarping
- FIR design by windowing
- equiripple/Remez FIR design
- FIR vs IIR tradeoffs: phase, order, stability, runtime, and finite precision

### 6. Model Fitting And Adaptive Filtering

- regression and model fitting
- parameter estimation vs adaptation
- adaptive linear combiner
- mean-square error cost
- Wiener-Hopf equation
- steepest descent
- LMS
- NLMS
- LMS-Newton
- adaptive IIR filters
- equation-error and output-error adaptation
- filtered-x LMS
- adaptive line enhancement
- echo cancellation
- active noise control

### 7. Compensation And Sensor Fusion Algorithms

- measurement-chain transfer model
- inverse filtering as deconvolution
- ill-conditioned inverse problems
- Tikhonov regularization
- Wiener filtering
- Kalman filtering
- Bayesian sensor fusion
- Dempster-Shafer fusion
- reliability weighting and conflict handling

### 8. Real-Time And Validation Concepts

- sample-by-sample processing
- block processing
- latency and state
- overflow and finite precision
- processor workflow
- reproducible plotted artifacts
- golden-signal tests
- comparison against reference implementations
