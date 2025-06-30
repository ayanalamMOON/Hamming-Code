#include "ecc/bch_code.hpp"
#include "ecc/galois_field.hpp"
#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>
#include <vector>

using namespace ecc;

void demonstrate_bch_encoding_decoding()
{
    std::cout << "=== BCH Code Demonstration ===" << std::endl;
    std::cout << std::endl;

    // Demonstrate BCH(15,7) code with t=1 error correction
    std::cout << "1. BCH(15,7) Code - Single Error Correction" << std::endl;
    std::cout << "   Code parameters: n=15, k=7, t=1" << std::endl;
    std::cout << "   Minimum distance: " << BCH_15_7_3::min_distance << std::endl;
    std::cout << std::endl;

    BCH_15_7_3 bch15;

    // Example data word
    BCH_15_7_3::DataWord data;
    data[0] = 1;
    data[1] = 0;
    data[2] = 1;
    data[3] = 1;
    data[4] = 0;
    data[5] = 1;
    data[6] = 0;

    std::cout << "   Original data:     " << data << std::endl;

    // Encode
    auto encoded = bch15.encode(data);
    std::cout << "   Encoded codeword:  " << encoded << std::endl;

    // Decode without errors
    auto result = bch15.decode(encoded);
    std::cout << "   Decoded data:      " << result.data << std::endl;
    std::cout << "   Decoding success:  " << (result.success ? "Yes" : "No") << std::endl;
    std::cout << "   Errors corrected:  " << result.errors_corrected << std::endl;
    std::cout << std::endl;

    // Introduce single error
    std::cout << "2. Error Correction Demonstration" << std::endl;
    auto corrupted = encoded;
    size_t error_position = 5;
    corrupted.flip(error_position);
    std::cout << "   Corrupted codeword: " << corrupted << " (error at position " << error_position << ")" << std::endl;

    result = bch15.decode(corrupted);
    std::cout << "   Corrected data:     " << result.data << std::endl;
    std::cout << "   Decoding success:   " << (result.success ? "Yes" : "No") << std::endl;
    std::cout << "   Errors corrected:   " << result.errors_corrected << std::endl;
    if (!result.error_positions.empty())
    {
        std::cout << "   Error positions:    ";
        for (size_t pos : result.error_positions)
            std::cout << pos << " ";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void demonstrate_multiple_bch_codes()
{
    std::cout << "3. Multiple BCH Code Configurations" << std::endl;
    std::cout << std::endl;

    // BCH(31,21) demonstration
    std::cout << "   BCH(31,21) Code - Single Error Correction" << std::endl;
    BCH_31_21_3 bch31;

    BCH_31_21_3::DataWord data31;
    std::mt19937 rng(42);
    for (size_t i = 0; i < BCH_31_21_3::data_length; ++i)
    {
        data31[i] = rng() % 2;
    }

    auto encoded31 = bch31.encode(data31);
    std::cout << "   Code length: " << BCH_31_21_3::code_length << std::endl;
    std::cout << "   Data length: " << BCH_31_21_3::data_length << std::endl;
    std::cout << "   Parity bits: " << BCH_31_21_3::parity_length << std::endl;
    std::cout << "   Error capacity: " << BCH_31_21_3::error_capacity << std::endl;

    // Test error correction
    auto corrupted31 = encoded31;
    corrupted31.flip(10);
    auto result31 = bch31.decode(corrupted31);
    std::cout << "   Single error correction: " << (result31.success ? "Success" : "Failed") << std::endl;
    std::cout << std::endl;

    // BCH(15,5) with t=2 demonstration
    std::cout << "   BCH(15,5) Code - Double Error Correction" << std::endl;
    BCH_15_5_3 bch15_2;

    BCH_15_5_3::DataWord data15_2;
    data15_2[0] = 1;
    data15_2[1] = 0;
    data15_2[2] = 1;
    data15_2[3] = 1;
    data15_2[4] = 0;

    auto encoded15_2 = bch15_2.encode(data15_2);
    std::cout << "   Code length: " << BCH_15_5_3::code_length << std::endl;
    std::cout << "   Data length: " << BCH_15_5_3::data_length << std::endl;
    std::cout << "   Error capacity: " << BCH_15_5_3::error_capacity << std::endl;

    // Test double error correction
    auto corrupted15_2 = encoded15_2;
    corrupted15_2.flip(2);
    corrupted15_2.flip(8);
    auto result15_2 = bch15_2.decode(corrupted15_2);
    std::cout << "   Double error correction: " << (result15_2.success ? "Success" : "Failed") << std::endl;
    std::cout << "   Errors corrected: " << result15_2.errors_corrected << std::endl;
    std::cout << std::endl;
}

void performance_analysis()
{
    std::cout << "4. Performance Analysis" << std::endl;
    std::cout << std::endl;

    const size_t num_tests = 10000;
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

    struct CodeStats
    {
        std::string name;
        size_t successful_corrections;
        size_t total_tests;
        double avg_encode_time_us;
        double avg_decode_time_us;
        size_t total_errors_corrected;
    };

    std::vector<CodeStats> stats;

    // Test BCH(15,7)
    {
        BCH_15_7_3 bch;
        CodeStats stat;
        stat.name = "BCH(15,7)";
        stat.successful_corrections = 0;
        stat.total_tests = num_tests;
        stat.total_errors_corrected = 0;

        auto start_time = std::chrono::high_resolution_clock::now();

        double total_encode_time = 0.0;
        double total_decode_time = 0.0;

        for (size_t test = 0; test < num_tests; ++test)
        {
            // Generate random data
            BCH_15_7_3::DataWord data;
            for (size_t i = 0; i < BCH_15_7_3::data_length; ++i)
            {
                data[i] = rng() % 2;
            }

            // Measure encoding time
            auto encode_start = std::chrono::high_resolution_clock::now();
            auto encoded = bch.encode(data);
            auto encode_end = std::chrono::high_resolution_clock::now();
            total_encode_time += std::chrono::duration<double, std::micro>(encode_end - encode_start).count();

            // Introduce random single error
            auto corrupted = encoded;
            if (rng() % 4 < 3) // 75% chance of error
            {
                size_t error_pos = rng() % BCH_15_7_3::code_length;
                corrupted.flip(error_pos);
            }

            // Measure decoding time
            auto decode_start = std::chrono::high_resolution_clock::now();
            auto result = bch.decode(corrupted);
            auto decode_end = std::chrono::high_resolution_clock::now();
            total_decode_time += std::chrono::duration<double, std::micro>(decode_end - decode_start).count();

            if (result.success && result.data == data)
            {
                stat.successful_corrections++;
                stat.total_errors_corrected += result.errors_corrected;
            }
        }

        stat.avg_encode_time_us = total_encode_time / num_tests;
        stat.avg_decode_time_us = total_decode_time / num_tests;
        stats.push_back(stat);
    }

    // Test BCH(31,21)
    {
        BCH_31_21_3 bch;
        CodeStats stat;
        stat.name = "BCH(31,21)";
        stat.successful_corrections = 0;
        stat.total_tests = num_tests / 10; // Fewer tests for larger code
        stat.total_errors_corrected = 0;

        double total_encode_time = 0.0;
        double total_decode_time = 0.0;

        for (size_t test = 0; test < stat.total_tests; ++test)
        {
            BCH_31_21_3::DataWord data;
            for (size_t i = 0; i < BCH_31_21_3::data_length; ++i)
            {
                data[i] = rng() % 2;
            }

            auto encode_start = std::chrono::high_resolution_clock::now();
            auto encoded = bch.encode(data);
            auto encode_end = std::chrono::high_resolution_clock::now();
            total_encode_time += std::chrono::duration<double, std::micro>(encode_end - encode_start).count();

            auto corrupted = encoded;
            if (rng() % 4 < 3)
            {
                size_t error_pos = rng() % BCH_31_21_3::code_length;
                corrupted.flip(error_pos);
            }

            auto decode_start = std::chrono::high_resolution_clock::now();
            auto result = bch.decode(corrupted);
            auto decode_end = std::chrono::high_resolution_clock::now();
            total_decode_time += std::chrono::duration<double, std::micro>(decode_end - decode_start).count();

            if (result.success && result.data == data)
            {
                stat.successful_corrections++;
                stat.total_errors_corrected += result.errors_corrected;
            }
        }

        stat.avg_encode_time_us = total_encode_time / stat.total_tests;
        stat.avg_decode_time_us = total_decode_time / stat.total_tests;
        stats.push_back(stat);
    }

    // Display results
    std::cout << "   Performance Results:" << std::endl;
    std::cout << "   " << std::setw(12) << "Code"
              << std::setw(10) << "Success%"
              << std::setw(12) << "Encode(μs)"
              << std::setw(12) << "Decode(μs)"
              << std::setw(12) << "Corrected" << std::endl;
    std::cout << "   " << std::string(58, '-') << std::endl;

    for (const auto &stat : stats)
    {
        double success_rate = 100.0 * stat.successful_corrections / stat.total_tests;
        std::cout << "   " << std::setw(12) << stat.name
                  << std::setw(9) << std::fixed << std::setprecision(1) << success_rate << "%"
                  << std::setw(12) << std::fixed << std::setprecision(2) << stat.avg_encode_time_us
                  << std::setw(12) << std::fixed << std::setprecision(2) << stat.avg_decode_time_us
                  << std::setw(12) << stat.total_errors_corrected << std::endl;
    }
    std::cout << std::endl;
}

void demonstrate_advanced_codes()
{
    std::cout << "5. Advanced Codes Demonstration" << std::endl;
    std::cout << std::endl;

    // LDPC Code demonstration
    std::cout << "   LDPC Code Example:" << std::endl;
    LDPCCode ldpc(15, 7);

    LDPCCode::DataWord ldpc_data = {1, 0, 1, 1, 0, 1, 0};
    auto ldpc_encoded = ldpc.encode(ldpc_data);

    std::cout << "   Code length: " << ldpc_encoded.size() << std::endl;
    std::cout << "   Data length: " << ldpc_data.size() << std::endl;
    std::cout << "   Code rate: " << std::fixed << std::setprecision(3)
              << static_cast<double>(ldpc_data.size()) / ldpc_encoded.size() << std::endl;

    auto ldpc_result = ldpc.decode(ldpc_encoded);
    std::cout << "   Decoding success: " << (ldpc_result.success ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // Turbo Code demonstration
    std::cout << "   Turbo Code Example:" << std::endl;
    TurboCode turbo(7);

    TurboCode::DataWord turbo_data = {1, 0, 1, 1, 0, 1, 0};
    auto turbo_encoded = turbo.encode(turbo_data);

    std::cout << "   Code length: " << turbo_encoded.size() << std::endl;
    std::cout << "   Data length: " << turbo_data.size() << std::endl;
    std::cout << "   Code rate: " << std::fixed << std::setprecision(3)
              << static_cast<double>(turbo_data.size()) / turbo_encoded.size() << std::endl;

    auto turbo_result = turbo.decode(turbo_encoded);
    std::cout << "   Decoding success: " << (turbo_result.success ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
}

int main()
{
    try
    {
        demonstrate_bch_encoding_decoding();
        demonstrate_multiple_bch_codes();
        performance_analysis();
        demonstrate_advanced_codes();

        std::cout << "🎉 BCH Code demonstration completed successfully!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
