# Advanced Error Correction Codes Library

A comprehensive C++20 library implementing various error correction codes with modern design patterns, SIMD optimizations, and extensive testing.

## Features

### Error Correction Codes
- **Hamming Codes**: Classic and SECDED (Single Error Correction, Double Error Detection)
- **BCH Codes**: Binary Bose-Chaudhuri-Hocquenghem codes with configurable error correction capability
- **Reed-Solomon Codes**: Powerful non-binary codes for burst error correction
- **LDPC Codes**: Low-Density Parity-Check codes with belief propagation decoding
- **Turbo Codes**: Advanced iterative decoding with configurable interleavers

### Advanced Features
- **SIMD Optimizations**: AVX2/SSE instructions for high-performance matrix operations
- **Parallel Processing**: OpenMP support for multi-threaded encoding/decoding
- **Galois Field Arithmetic**: Optimized GF(2^m) operations with lookup tables
- **Performance Analysis**: BER curves, throughput measurements, error pattern analysis
- **Error Simulation**: Comprehensive channel models and error pattern generators
  - Binary Symmetric Channel (BSC)
  - Additive White Gaussian Noise (AWGN)
  - Burst Error Channel
  - Clustered Error Channel
  - Erasure Channel
  - Fading Channel
  - Deterministic error pattern generation

### Modern C++ Design
- Header-only template library for optimal performance
- RAII and smart pointers for memory management
- Concepts and constraints for type safety
- Exception-safe operations
- Comprehensive unit testing

## Documentation

📖 **Comprehensive documentation is available in the [`docs/`](docs/) directory:**

- **[Build Instructions](docs/BUILD_INSTRUCTIONS.md)** - Complete setup and compilation guide
- **[API Reference](docs/API_REFERENCE.md)** - Detailed API documentation for all classes and functions
- **[Examples Guide](docs/EXAMPLES.md)** - Usage examples and tutorials
- **[Contributing Guide](docs/CONTRIBUTING.md)** - How to contribute to the project
- **[Implementation Summary](docs/IMPLEMENTATION_SUMMARY.md)** - Technical implementation details
- **[Error Simulator Guide](docs/ERROR_SIMULATOR_README.md)** - Channel modeling and error simulation
- **[C++20 Configuration](docs/C++20_CONFIGURATION.md)** - C++20 setup and requirements
- **[Directory Organization](docs/DIRECTORY_ORGANIZATION.md)** - Project structure overview

## Quick Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

📋 **For detailed build instructions, see [Build Instructions](docs/BUILD_INSTRUCTIONS.md)**

### Requirements
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.16+
- Optional: OpenMP for parallel processing

## Quick Start

🚀 **For complete usage examples, see [Examples Guide](docs/EXAMPLES.md)**

```cpp
#include "hamming_code.hpp"
#include "reed_solomon.hpp"

// Hamming(7,4) code
auto hamming = ecc::HammingCode<7, 4>();
auto encoded = hamming.encode({1, 0, 1, 1});
auto decoded = hamming.decode(encoded);

// Reed-Solomon(255,223) code
auto rs = ecc::ReedSolomonCode<255, 223>();
std::vector<uint8_t> data(223, 42);
auto codeword = rs.encode(data);
auto recovered = rs.decode(codeword);
```

## Examples

🔧 **Complete examples and tutorials available in [Examples Guide](docs/EXAMPLES.md)**

### Command Line Interface
```bash
# Encode data with Hamming(15,11) code
./ecc_demo --code hamming --n 15 --k 11 --encode "10110100101"

# Analyze BER performance
./ecc_benchmark --code all --snr-range 0:10:0.5 --iterations 10000

# Run comprehensive tests
./ecc_test
```

### Performance Analysis
```cpp
#include "performance_analyzer.hpp"

auto analyzer = ecc::PerformanceAnalyzer();
analyzer.analyze_ber_curve<ecc::HammingCode<15, 11>>(
    0.0, 10.0, 0.5, 10000
);
analyzer.save_results("hamming_15_11_ber.csv");
```

## Architecture

📁 **Project structure details in [Directory Organization](docs/DIRECTORY_ORGANIZATION.md)**

```
include/
├── ecc/
│   ├── codes/           # Error correction code implementations
│   ├── galois/          # Galois field arithmetic
│   ├── matrix/          # Matrix operations and linear algebra
│   ├── performance/     # Performance analysis tools
│   └── utils/           # Utilities and helpers
src/                     # Implementation files
test/                    # Unit and integration tests
benchmarks/              # Performance benchmarks
examples/                # Usage examples
docs/                    # Documentation
output/                  # Compiled executables
```

## Performance

### Throughput (AMD Ryzen 9 5900X)
| Code | Encoding (Mbps) | Decoding (Mbps) |
|------|----------------|----------------|
| Hamming(15,11) | 2,847 | 1,923 |
| BCH(255,239,2) | 1,456 | 892 |
| RS(255,223) | 734 | 445 |
| LDPC(1024,512) | 234 | 156 |

### Error Correction Capability
| Code | Min Distance | Error Correction | Error Detection |
|------|-------------|-----------------|----------------|
| Hamming(15,11) | 3 | 1 bit | 2 bits |
| BCH(255,239,2) | 5 | 2 bits | 4 bits |
| RS(255,223) | 33 | 16 symbols | 32 symbols |

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is licensed under the MIT License - see LICENSE file for details.

## References

- Lin, S., & Costello, D. J. (2004). Error control coding.
- MacWilliams, F. J., & Sloane, N. J. A. (1977). The theory of error-correcting codes.
- Richardson, T., & Urbanke, R. (2008). Modern coding theory.
