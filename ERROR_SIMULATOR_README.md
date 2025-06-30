# Error Simulator Documentation

## Overview

The Error Simulator is a comprehensive tool for simulating various types of communication channel errors in error correction code testing. It provides multiple channel models and error pattern generators to thoroughly test the performance of error correction codes under different conditions.

## Features

### Channel Models

1. **Binary Symmetric Channel (BSC)**
   - Random bit errors with configurable probability
   - Independent error events
   - Ideal for testing random error correction capability

2. **Burst Error Channel**
   - Simulates burst errors common in real communication systems
   - Configurable burst length and occurrence probability
   - Models scenarios like fading channels or interference

3. **Additive White Gaussian Noise (AWGN) Channel**
   - Models continuous channel with Gaussian noise
   - SNR-based error simulation
   - Realistic for wireless communication systems

4. **Clustered Error Channel**
   - Errors occur in clusters rather than uniformly
   - Configurable cluster size and density
   - Models correlated error patterns

5. **Erasure Channel**
   - Some bits are completely lost (marked as erasures)
   - Different from bit errors (flipped bits)
   - Useful for testing erasure correction codes

6. **Fading Channel**
   - Combines multiplicative fading with additive noise
   - Models time-varying channel conditions
   - Realistic for mobile communication scenarios

### Error Pattern Generator

The Error Pattern Generator provides deterministic error patterns for systematic testing:

- **Single Error**: One bit error at specified position
- **Double Error**: Two bit errors at specified positions
- **Triple Error**: Three bit errors at specified positions
- **Burst Error**: Consecutive errors of specified length
- **Random Error**: Specified number of randomly placed errors
- **Weight Pattern**: Errors with specific Hamming weight

## Usage Examples

### Basic Usage

```cpp
#include "error_simulator.cpp"
using namespace ecc;

// Create simulator
ErrorSimulator simulator;

// Configure BSC channel with 5% error rate
ErrorParameters params;
params.probability = 0.05;
params.seed = 42;
simulator.create_channel(ErrorType::RANDOM, params);

// Apply errors to codeword
std::vector<uint8_t> codeword = {1, 0, 1, 1, 0, 0, 1};
auto corrupted = simulator.apply_errors(codeword);

// Analyze error statistics
auto stats = simulator.analyze_errors(codeword, corrupted);
std::cout << "BER: " << stats.bit_error_rate << std::endl;
```

### Error Pattern Testing

```cpp
ErrorPatternGenerator pattern_gen;

// Test single error correction
auto single_error = pattern_gen.generate_single_error(codeword.size(), 3);
auto corrupted = simulator.apply_error_pattern(codeword, single_error);

// Test burst error correction
auto burst_error = pattern_gen.generate_burst_error(codeword.size(), 2, 3);
auto burst_corrupted = simulator.apply_error_pattern(codeword, burst_error);
```

### Performance Analysis

```cpp
// Compare different channel models
const size_t num_trials = 1000;

// Test BSC performance
ErrorParameters bsc_params{ErrorType::RANDOM, 0.01, 0, 0, 0, 0.0, 1234};
simulator.create_channel(ErrorType::RANDOM, bsc_params);

size_t total_errors = 0;
for (size_t i = 0; i < num_trials; ++i) {
    auto corrupted = simulator.apply_errors(codeword);
    auto stats = simulator.analyze_errors(codeword, corrupted);
    total_errors += stats.error_bits;
}

double avg_ber = static_cast<double>(total_errors) / (num_trials * codeword.size());
```

## Configuration Parameters

### ErrorParameters Structure

```cpp
struct ErrorParameters {
    ErrorType type = ErrorType::RANDOM;
    double probability = 0.01;     // Error probability or SNR (dB)
    size_t burst_length = 5;       // Length of burst errors
    size_t cluster_size = 3;       // Size of error clusters
    size_t period = 7;             // Period for periodic errors
    double fading_amplitude = 0.5; // Fading channel amplitude variation
    size_t seed = 42;              // Random seed for reproducibility
};
```

### Channel-Specific Parameters

- **BSC**: `probability` is the bit error probability (0.0 to 1.0)
- **AWGN**: `probability` is the SNR in dB
- **Burst**: `probability` is burst occurrence probability, `burst_length` is burst size
- **Clustered**: `probability` affects cluster density, `cluster_size` sets cluster size
- **Erasure**: `probability` is the erasure probability
- **Fading**: `probability` is SNR in dB, `fading_amplitude` is fading variation

## Integration with ECC Testing

The Error Simulator integrates seamlessly with the existing ECC framework:

```cpp
template<typename CodeType>
void test_code_performance() {
    CodeType code;
    ErrorSimulator simulator;

    // Test under different conditions
    std::vector<double> snr_values = {0, 3, 6, 9, 12, 15};

    for (double snr : snr_values) {
        ErrorParameters params{ErrorType::RANDOM, snr, 0, 0, 0, 0.0, 42};
        simulator.create_channel(ErrorType::RANDOM, params);

        // Perform BER analysis
        // ... testing code here ...
    }
}
```

## Error Statistics

The simulator provides detailed error analysis:

```cpp
struct ErrorStatistics {
    size_t total_bits;              // Total number of bits
    size_t error_bits;              // Number of bit errors
    size_t error_blocks;            // Number of block errors
    double bit_error_rate;          // BER = error_bits / total_bits
    double block_error_rate;        // BLER = error_blocks / total_blocks
    std::vector<size_t> error_positions; // Positions of errors
};
```

## Best Practices

1. **Reproducible Testing**: Always set a seed value for consistent results across runs
2. **Appropriate Parameters**: Choose error probabilities that match your target scenario
3. **Statistical Significance**: Use sufficient number of trials for reliable statistics
4. **Channel Modeling**: Select channel models that match your application domain
5. **Pattern Testing**: Use deterministic patterns to verify specific error correction capabilities

## Advanced Features

### Custom Channel Models

The simulator supports custom channel models by extending the `ChannelModel` interface:

```cpp
class CustomChannel : public ChannelModel {
public:
    std::vector<uint8_t> apply_errors(const std::vector<uint8_t>& codeword) override {
        // Custom error injection logic
    }

    void set_parameters(const ErrorParameters& params) override {
        // Parameter handling
    }

    std::string get_name() const override {
        return "Custom Channel";
    }
};
```

### Batch Processing

For large-scale simulations, use batch processing:

```cpp
void batch_simulation(const std::vector<std::vector<uint8_t>>& codewords) {
    ErrorSimulator simulator;
    // Configure channel...

    for (const auto& codeword : codewords) {
        auto corrupted = simulator.apply_errors(codeword);
        // Process results...
    }
}
```

## Performance Considerations

- Use appropriate random seeds for parallel simulations
- Consider memory usage for large-scale batch processing
- Profile performance for time-critical applications
- Cache channel objects when testing multiple codewords with same parameters

## Validation

The Error Simulator has been validated against:
- Theoretical BER calculations for BSC and AWGN channels
- Known error correction capabilities of standard codes
- Consistency across multiple simulation runs
- Integration with existing ECC test framework
