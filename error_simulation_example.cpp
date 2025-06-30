// Example: Error Simulation and Code Performance Analysis
// This example demonstrates how to use the Error Simulator with error correction codes

#include <iostream>
#include <iomanip>
#include <vector>

// Include the error simulator (in a real project, this would be a proper header)
#include "src/error_simulator.cpp"

int main()
{
    using namespace ecc;

    std::cout << "Error Correction Code Performance Analysis\n";
    std::cout << std::string(60, '=') << "\n\n";

    // Simulate a simple (15,11) linear code (for demonstration)
    std::vector<uint8_t> test_data = {1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0};
    std::vector<uint8_t> test_codeword = {1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1}; // Add 4 parity bits

    std::cout << "Original data:     ";
    for (auto bit : test_data)
        std::cout << static_cast<int>(bit);
    std::cout << "\nEncoded codeword:  ";
    for (auto bit : test_codeword)
        std::cout << static_cast<int>(bit);
    std::cout << "\n\n";

    ErrorSimulator simulator;

    // 1. Error Correction Capability Analysis
    std::cout << "1. Error Correction Capability Test\n";
    std::cout << std::string(50, '-') << "\n";

    ErrorPatternGenerator pattern_gen(12345);

    // Test single error correction
    std::cout << "Testing single error correction:\n";
    for (size_t pos = 0; pos < test_codeword.size(); ++pos)
    {
        auto error_pattern = pattern_gen.generate_single_error(test_codeword.size(), pos);
        auto corrupted = simulator.apply_error_pattern(test_codeword, error_pattern);
        auto stats = simulator.analyze_errors(test_codeword, corrupted);

        std::cout << "Error at position " << std::setw(2) << pos << ": ";
        for (auto bit : corrupted)
            std::cout << static_cast<int>(bit);
        std::cout << " (" << stats.error_bits << " errors)\n";
    }

    // Test double error detection
    std::cout << "\nTesting double error patterns:\n";
    std::vector<std::pair<size_t, size_t>> double_error_positions = {
        {0, 7}, {2, 9}, {5, 12}, {1, 14}, {3, 8}};

    for (auto [pos1, pos2] : double_error_positions)
    {
        auto error_pattern = pattern_gen.generate_double_error(test_codeword.size(), pos1, pos2);
        auto corrupted = simulator.apply_error_pattern(test_codeword, error_pattern);
        auto stats = simulator.analyze_errors(test_codeword, corrupted);

        std::cout << "Errors at positions " << pos1 << "," << pos2 << ": ";
        for (auto bit : corrupted)
            std::cout << static_cast<int>(bit);
        std::cout << " (" << stats.error_bits << " errors)\n";
    }

    // 2. Channel Performance Analysis
    std::cout << "\n2. Channel Performance Analysis\n";
    std::cout << std::string(50, '-') << "\n";

    const size_t num_trials = 10000;
    std::vector<double> error_rates = {0.001, 0.005, 0.01, 0.02, 0.05, 0.1};

    std::cout << "BSC Error Rate | Avg BER    | Block Errors | Error Rate\n";
    std::cout << std::string(50, '-') << "\n";

    for (double rate : error_rates)
    {
        ErrorParameters params;
        params.probability = rate;
        params.seed = 54321;
        simulator.create_channel(ErrorType::RANDOM, params);

        size_t total_bit_errors = 0;
        size_t total_block_errors = 0;

        for (size_t trial = 0; trial < num_trials; ++trial)
        {
            auto corrupted = simulator.apply_errors(test_codeword);
            auto stats = simulator.analyze_errors(test_codeword, corrupted);
            total_bit_errors += stats.error_bits;
            if (stats.error_bits > 0)
                total_block_errors++;
        }

        double avg_ber = static_cast<double>(total_bit_errors) / (num_trials * test_codeword.size());
        double block_error_rate = static_cast<double>(total_block_errors) / num_trials;

        std::cout << std::scientific << std::setprecision(3) << rate << "      | "
                  << avg_ber << " | " << std::setw(11) << total_block_errors << " | "
                  << std::fixed << std::setprecision(3) << block_error_rate << "\n";
    }

    // 3. Burst Error Analysis
    std::cout << "\n3. Burst Error Analysis\n";
    std::cout << std::string(50, '-') << "\n";

    std::vector<size_t> burst_lengths = {1, 2, 3, 4, 5};

    std::cout << "Burst Length | Burst Prob | Avg Errors | Block Error Rate\n";
    std::cout << std::string(50, '-') << "\n";

    for (size_t burst_len : burst_lengths)
    {
        ErrorParameters burst_params;
        burst_params.type = ErrorType::BURST;
        burst_params.probability = 0.1; // 10% chance of burst
        burst_params.burst_length = burst_len;
        burst_params.seed = 98765;
        simulator.create_channel(ErrorType::BURST, burst_params);

        size_t total_bit_errors = 0;
        size_t total_block_errors = 0;

        for (size_t trial = 0; trial < num_trials; ++trial)
        {
            auto corrupted = simulator.apply_errors(test_codeword);
            auto stats = simulator.analyze_errors(test_codeword, corrupted);
            total_bit_errors += stats.error_bits;
            if (stats.error_bits > 0)
                total_block_errors++;
        }

        double avg_errors = static_cast<double>(total_bit_errors) / num_trials;
        double block_error_rate = static_cast<double>(total_block_errors) / num_trials;

        std::cout << std::setw(12) << burst_len << " | " << std::fixed << std::setprecision(3)
                  << burst_params.probability << "      | " << std::setprecision(2) << avg_errors
                  << "       | " << std::setprecision(3) << block_error_rate << "\n";
    }

    // 4. Error Distribution Analysis
    std::cout << "\n4. Error Distribution Analysis\n";
    std::cout << std::string(50, '-') << "\n";

    ErrorParameters dist_params;
    dist_params.probability = 0.02;
    dist_params.seed = 13579;
    simulator.create_channel(ErrorType::RANDOM, dist_params);

    std::vector<size_t> error_position_counts(test_codeword.size(), 0);

    for (size_t trial = 0; trial < num_trials; ++trial)
    {
        auto corrupted = simulator.apply_errors(test_codeword);
        auto stats = simulator.analyze_errors(test_codeword, corrupted);

        for (size_t pos : stats.error_positions)
        {
            error_position_counts[pos]++;
        }
    }

    std::cout << "Position | Error Count | Error Rate\n";
    std::cout << std::string(35, '-') << "\n";

    for (size_t pos = 0; pos < test_codeword.size(); ++pos)
    {
        double pos_error_rate = static_cast<double>(error_position_counts[pos]) / num_trials;
        std::cout << std::setw(8) << pos << " | " << std::setw(11) << error_position_counts[pos]
                  << " | " << std::fixed << std::setprecision(4) << pos_error_rate << "\n";
    }

    // 5. Comparison of Channel Models
    std::cout << "\n5. Channel Model Comparison\n";
    std::cout << std::string(50, '-') << "\n";

    struct ChannelTestResult
    {
        std::string name;
        double avg_ber;
        double block_error_rate;
    };

    std::vector<ChannelTestResult> results;

    // Test BSC
    {
        ErrorParameters bsc_params{ErrorType::RANDOM, 0.02, 0, 0, 0, 0.0, 11111};
        simulator.create_channel(ErrorType::RANDOM, bsc_params);

        size_t total_errors = 0, block_errors = 0;
        for (size_t trial = 0; trial < 5000; ++trial)
        {
            auto corrupted = simulator.apply_errors(test_codeword);
            auto stats = simulator.analyze_errors(test_codeword, corrupted);
            total_errors += stats.error_bits;
            if (stats.error_bits > 0)
                block_errors++;
        }

        results.push_back({"BSC (p=0.02)",
                           static_cast<double>(total_errors) / (5000 * test_codeword.size()),
                           static_cast<double>(block_errors) / 5000});
    }

    // Test Burst
    {
        ErrorParameters burst_params{ErrorType::BURST, 0.05, 4, 0, 0, 0.0, 22222};
        simulator.create_channel(ErrorType::BURST, burst_params);

        size_t total_errors = 0, block_errors = 0;
        for (size_t trial = 0; trial < 5000; ++trial)
        {
            auto corrupted = simulator.apply_errors(test_codeword);
            auto stats = simulator.analyze_errors(test_codeword, corrupted);
            total_errors += stats.error_bits;
            if (stats.error_bits > 0)
                block_errors++;
        }

        results.push_back({"Burst (p=0.05, len=4)",
                           static_cast<double>(total_errors) / (5000 * test_codeword.size()),
                           static_cast<double>(block_errors) / 5000});
    }

    // Test Clustered
    {
        ErrorParameters cluster_params{ErrorType::CLUSTERED, 0.015, 0, 3, 0, 0.0, 33333};
        simulator.create_channel(ErrorType::CLUSTERED, cluster_params);

        size_t total_errors = 0, block_errors = 0;
        for (size_t trial = 0; trial < 5000; ++trial)
        {
            auto corrupted = simulator.apply_errors(test_codeword);
            auto stats = simulator.analyze_errors(test_codeword, corrupted);
            total_errors += stats.error_bits;
            if (stats.error_bits > 0)
                block_errors++;
        }

        results.push_back({"Clustered (p=0.015, size=3)",
                           static_cast<double>(total_errors) / (5000 * test_codeword.size()),
                           static_cast<double>(block_errors) / 5000});
    }

    std::cout << "Channel Model           | Avg BER    | Block Error Rate\n";
    std::cout << std::string(50, '-') << "\n";

    for (const auto &result : results)
    {
        std::cout << std::left << std::setw(23) << result.name << " | "
                  << std::scientific << std::setprecision(3) << result.avg_ber << " | "
                  << std::fixed << std::setprecision(4) << result.block_error_rate << "\n";
    }

    std::cout << "\nAnalysis Complete!\n";
    std::cout << "The Error Simulator provides comprehensive channel modeling\n";
    std::cout << "for thorough testing of error correction code performance.\n";

    return 0;
}
