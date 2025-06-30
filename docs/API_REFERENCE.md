# API Reference

## Core Classes

### HammingCode<n, k>
Template class for Hamming error correction codes.

#### Template Parameters
- `n`: Total code length (e.g., 7, 15, 31)
- `k`: Information bits (e.g., 4, 11, 26)

#### Methods
- `encode(data)`: Encode data bits into codeword
- `decode(received)`: Decode received bits and correct errors
- `get_syndrome(received)`: Calculate syndrome for error detection

### BCHCode<n, k, t>
Template class for BCH (Bose-Chaudhuri-Hocquenghem) codes.

#### Template Parameters
- `n`: Code length
- `k`: Information length
- `t`: Error correction capability

#### Methods
- `encode(data)`: Encode message using BCH algorithm
- `decode(received)`: Decode and correct up to t errors

### ReedSolomonCode<n, k, m>
Template class for Reed-Solomon codes.

#### Template Parameters
- `n`: Code length (typically 2^m - 1)
- `k`: Information length
- `m`: Symbol size in bits

#### Methods
- `encode(message)`: Encode message symbols
- `decode(received)`: Decode with error and erasure correction

### GaloisField<m>
Galois Field operations for finite field arithmetic.

#### Template Parameters
- `m`: Field extension degree (field size = 2^m)

#### Methods
- `add(a, b)`: Addition in GF(2^m)
- `multiply(a, b)`: Multiplication in GF(2^m)
- `inverse(a)`: Multiplicative inverse
- `power(base, exp)`: Exponentiation

## Performance Analysis

### PerformanceAnalyzer
Class for analyzing error correction performance.

#### Methods
- `analyze_performance<CodeType>(channel, parameter, iterations)`: Analyze BER performance
- `analyze_ber_curve<CodeType>(snr_min, snr_max, step, iterations)`: Generate BER curves
- `compare_codes<CodeTypes...>(channel, parameter, iterations)`: Compare multiple codes

## Error Simulation

### ErrorSimulator
Class for simulating channel errors.

#### Channel Types
- `AWGN`: Additive White Gaussian Noise
- `BSC`: Binary Symmetric Channel
- `BEC`: Binary Erasure Channel

#### Methods
- `add_errors(data, channel_type, parameter)`: Add channel errors to data
- `set_random_seed(seed)`: Set random number generator seed

## Utility Functions

### Matrix Operations
- `matrix_multiply(A, B)`: Matrix multiplication
- `matrix_transpose(A)`: Matrix transpose
- `matrix_inverse(A)`: Matrix inverse (if exists)

### Polynomial Operations
- `polynomial_add(p1, p2)`: Polynomial addition in GF(2)
- `polynomial_multiply(p1, p2)`: Polynomial multiplication
- `polynomial_divide(dividend, divisor)`: Polynomial division with remainder
