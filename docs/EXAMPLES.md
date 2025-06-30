# Examples and Usage Guide

This directory contains comprehensive examples demonstrating the capabilities of the Advanced Error Correction Codes library.

## Basic Usage Examples

### Hamming Codes

```cpp
#include "ecc/hamming_code.hpp"

// Basic Hamming(7,4) encoding/decoding
ecc::Hamming_7_4 code;
std::bitset<4> data("1011");

auto codeword = code.encode(data);
auto decoded = code.decode(codeword);

// Error correction example
auto received = codeword;
received.flip(2);  // Introduce single-bit error

auto result = code.decode_with_detection(received);
if (result.error_detected) {
    std::cout << "Error corrected at position: " << result.error_position << std::endl;
}
```

### Reed-Solomon Codes

```cpp
#include "ecc/reed_solomon.hpp"

// RS(255,223) code for byte-oriented data
ecc::RS_255_223 rs_code;
std::array<uint8_t, 223> data;
std::iota(data.begin(), data.end(), 0);  // Fill with 0,1,2,...

auto codeword = rs_code.encode(data);
auto result = rs_code.decode(codeword);

if (result.success) {
    std::cout << "Decoding successful, " << result.errors_corrected
              << " errors corrected" << std::endl;
}
```

### Performance Analysis

```cpp
#include "ecc/performance_analyzer.hpp"

ecc::PerformanceAnalyzer analyzer;

// BER curve analysis
auto results = analyzer.analyze_ber_curve<ecc::Hamming_15_11>(
    0.0, 10.0, 0.5, 10000);

analyzer.save_results(results, "hamming_15_11_ber.csv");

// Code comparison
analyzer.compare_codes<ecc::Hamming_7_4, ecc::RS_255_223>(
    ecc::ChannelType::AWGN, 5.0, 10000);
```

## Advanced Examples

### Custom Error Patterns

```cpp
#include "ecc/performance_analyzer.hpp"

ecc::ErrorPatternAnalyzer pattern_analyzer;

// Analyze correction capability vs error patterns
pattern_analyzer.analyze_error_patterns<ecc::Hamming_15_11>(5, 1000);
```

### SECDED Implementation

```cpp
#include "ecc/hamming_code.hpp"

// Single Error Correction, Double Error Detection
ecc::SECDED_16_11 secded_code;
std::bitset<11> data("10110100101");

auto codeword = secded_code.encode(data);

// Simulate double error
auto received = codeword;
received.flip(0);
received.flip(5);

auto result = secded_code.decode_secded(received);
switch (result.status) {
    case ecc::SECDED_16_11::SECDEDResult::Status::NO_ERROR:
        std::cout << "No errors detected" << std::endl;
        break;
    case ecc::SECDED_16_11::SECDEDResult::Status::SINGLE_ERROR_CORRECTED:
        std::cout << "Single error corrected" << std::endl;
        break;
    case ecc::SECDED_16_11::SECDEDResult::Status::DOUBLE_ERROR_DETECTED:
        std::cout << "Double error detected (uncorrectable)" << std::endl;
        break;
}
```

## Integration Examples

### With File I/O

```cpp
#include "ecc/reed_solomon.hpp"
#include <fstream>

void encode_file(const std::string& input_file, const std::string& output_file) {
    ecc::RS_255_223 code;
    std::ifstream in(input_file, std::ios::binary);
    std::ofstream out(output_file, std::ios::binary);

    std::array<uint8_t, 223> buffer;
    while (in.read(reinterpret_cast<char*>(buffer.data()), 223)) {
        auto codeword = code.encode(buffer);
        out.write(reinterpret_cast<const char*>(codeword.data()), 255);
    }
}
```

### Multi-threaded Processing

```cpp
#include "ecc/hamming_code.hpp"
#include <thread>
#include <vector>

void parallel_encode(const std::vector<std::bitset<11>>& data) {
    ecc::Hamming_15_11 code;
    const size_t num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    auto worker = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            auto codeword = code.encode(data[i]);
            // Process codeword...
        }
    };

    size_t chunk_size = data.size() / num_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? data.size() : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto& t : threads) {
        t.join();
    }
}
```

## Optimization Tips

### Memory Management
```cpp
// Pre-allocate vectors for better performance
std::vector<ecc::Hamming_7_4::CodeWord> codewords;
codewords.reserve(1000000);  // Reserve space for large datasets

// Use span for zero-copy operations
std::span<const ecc::Hamming_7_4::DataWord> data_span(data_vector);
auto encoded = code.encode(data_span);
```

### SIMD Optimizations
```cpp
// The library automatically uses SIMD instructions when available
// Compile with -march=native for best performance
// Use aligned memory for better vectorization
alignas(32) std::array<uint8_t, 256> aligned_buffer;
```

## Benchmarking Your Application

```cpp
#include <chrono>

auto benchmark_encoding = [](auto& code, const auto& data_set, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        for (const auto& data : data_set) {
            auto codeword = code.encode(data);
            // Prevent optimization
            asm volatile("" : : "r,m"(codeword) : "memory");
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();

    double throughput = (data_set.size() * iterations * code.code_length)
                       / (duration * 1e6);  // Mbps

    std::cout << "Throughput: " << throughput << " Mbps" << std::endl;
};
```

## Error Simulation

```cpp
#include "ecc/performance_analyzer.hpp"

// Custom error injection
template<typename CodeWord>
void inject_burst_error(CodeWord& codeword, size_t start, size_t length) {
    for (size_t i = start; i < std::min(start + length, codeword.size()); ++i) {
        codeword[i] = !codeword[i];
    }
}

// Channel simulation
void simulate_awgn_channel(auto& codeword, double snr_db) {
    std::random_device rd;
    std::mt19937 gen(rd());

    double snr_linear = std::pow(10.0, snr_db / 10.0);
    double noise_variance = 1.0 / (2.0 * snr_linear);
    std::normal_distribution<> noise(0.0, std::sqrt(noise_variance));

    for (auto& bit : codeword) {
        double signal = bit ? 1.0 : -1.0;
        double received = signal + noise(gen);
        bit = (received > 0.0);
    }
}
```
