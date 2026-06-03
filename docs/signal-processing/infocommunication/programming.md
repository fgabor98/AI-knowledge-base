---
status: draft
reviewed: false
domain: signal-processing/infocommunication/programming
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Infocommunication Programming

## Taxonomy

- Parent: [Infocommunication](index.md)
- Grandparent: [Signal Processing](../index.md)
- Page type: practical programming implementation
- Companion pages: [Theory](theory.md), [References](references.md)
- Implementation target: numerical simulations for coding, modulation, channels, receiver decisions, and system-level intuition

## Programming Learning Order

### 1. Communication Signal Setup

- bit arrays and symbol arrays
- sample rate and symbol rate
- time base and carrier frequency
- energy and power normalization
- AWGN helper

Deliverables:

- bitstream generator
- symbol mapper
- additive-noise channel helper

### 2. Quantization And Source Coding

- uniform quantization
- SQNR measurement
- codeword dictionaries
- average codeword length
- prefix-free-code validation

Deliverables:

- quantizer simulation
- simple prefix-code encoder/decoder

### 3. Error-Control Coding

- CRC generation and checking
- Hamming distance calculation
- nearest-codeword decoding
- interleaving and deinterleaving
- generator-matrix encoding
- parity-check matrix
- syndrome decoding
- convolutional encoder
- Viterbi decoder
- soft-decision metrics
- bit-error counting

Deliverables:

- small linear block code
- syndrome decoder
- CRC checker
- convolutional code with Viterbi decoder
- coding-gain experiment over a binary symmetric channel

### 4. Analog Modulation Simulations

- AM modulation and demodulation
- DSB-SC and coherent detection
- FM/PM signal generation
- spectrum plotting
- bandwidth inspection

Deliverables:

- AM waveform and spectrum demo
- coherent vs envelope demodulation comparison

### 5. Baseband Digital Transmission

- PCM and PAM
- pulse shaping
- matched filtering
- channel pulse response
- ISI simulation
- eye diagram generation
- raised-cosine and root-raised-cosine filters

Deliverables:

- PAM over low-pass channel
- eye diagram before and after pulse shaping
- ISI measurement at sampling instants

### 6. Carrier Digital Modulation

- ASK, PSK, FSK, and QAM mapping
- carrier modulation and demodulation
- constellation plotting
- decision regions
- phase and amplitude offset
- symbol and bit error rate

Deliverables:

- BPSK, QPSK, and 16-QAM simulations
- constellation before and after channel impairments
- BER vs SNR sweep for at least one modulation

### 7. Communication Synchronization And Equalization

- symbol timing offset simulation
- carrier frequency offset simulation
- carrier recovery
- symbol timing recovery
- phase-locked loop
- Costas loop for PSK
- pilot-based channel estimation
- adaptive equalization

Deliverables:

- simulate carrier frequency offset and recover it
- implement a simple Costas loop for BPSK or QPSK
- estimate a flat or frequency-selective channel from pilots
- compare unequalized and equalized constellations

### 8. OFDM And System Context

- subcarrier mapping
- IFFT/FFT modulation and demodulation
- cyclic-prefix intuition
- simple multipath channel
- frequency-domain equalization
- multi-access toy examples

Deliverables:

- minimal OFDM transmitter/receiver simulation
- subcarrier orthogonality check
- multipath/equalization demo
