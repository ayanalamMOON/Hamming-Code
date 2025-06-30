#include "ecc/performance_analyzer.hpp"
#include "ecc/hamming_code.hpp"
#include "ecc/reed_solomon.hpp"
#include "ecc/bch_code.hpp"
#include "../src/error_simulator.cpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <random>
#include <concepts>
#include <ranges>
#include <filesystem>
#include <execution>
#include <thread>
#include <future>
#include <memory_resource>

namespace ecc::benchmark
{

    // C++20 concepts for better type safety
    template <typename T>
    concept CodecType = requires(T code, typename T::DataWord data, typename T::CodeWord codeword) {
        typename T::DataWord;
        typename T::CodeWord;
        { code.encode(data) } -> std::convertible_to<typename T::CodeWord>;
        { code.decode(codeword) } -> std::convertible_to<typename T::DataWord>;
        { T::code_length } -> std::convertible_to<size_t>;
        { T::data_length } -> std::convertible_to<size_t>;
        requires std::is_default_constructible_v<T>;
    };

    template <typename T>
    concept DataWordType = requires(T data) {
        { data.size() } -> std::convertible_to<size_t>;
        { data[0] } -> std::convertible_to<bool>;
        requires std::ranges::range<T>;
    };

    template <typename T>
    concept CodeWordType = requires(T codeword) {
        { codeword.size() } -> std::convertible_to<size_t>;
        { codeword[0] } -> std::convertible_to<bool>;
        { codeword.flip(0) } -> std::same_as<T &>;
        requires std::ranges::range<T>;
    };

    /// BER Analysis Configuration
    struct BERAnalysisConfig
    {
        double snr_min_db = 0.0;
        double snr_max_db = 12.0;
        double snr_step_db = 1.0;
        size_t iterations_per_point = 10000;
        size_t min_errors = 100;         // Minimum errors for statistical significance
        size_t max_iterations = 1000000; // Maximum iterations per SNR point
        bool save_to_csv = true;
        std::string output_directory = "ber_results/";
    };

    /// BER Analysis Results
    struct BERResults
    {
        std::vector<double> snr_db_values;
        std::vector<double> ber_values;
        std::vector<double> bler_values;
        std::vector<double> throughput_values;
        std::vector<size_t> error_counts;
        std::vector<size_t> block_counts;
        std::string code_name;
    };

    /// Comprehensive BER Analysis Class
    class BERAnalyzer
    {
    private:
        BERAnalysisConfig config_;
        ErrorSimulator simulator_;
        PerformanceAnalyzer analyzer_;

    public:
        BERAnalyzer(const BERAnalysisConfig &config = {}) : config_(config), simulator_(42) {}

        /// Analyze BER curves for multiple codes
        void analyze_ber_curves()
        {
            std::cout << "=== Comprehensive BER Analysis ===\n\n";

            create_output_directory();

            // Analyze different error correction codes
            std::vector<BERResults> all_results;

            // Hamming codes
            std::cout << "Analyzing Hamming Codes...\n";
            all_results.push_back(analyze_hamming_7_4());
            all_results.push_back(analyze_hamming_15_11());

            // BCH codes (if available)
            std::cout << "Analyzing BCH Codes...\n";
            // all_results.push_back(analyze_bch_code());

            // Reed-Solomon codes (if available)
            std::cout << "Analyzing Reed-Solomon Codes...\n";
            // all_results.push_back(analyze_reed_solomon());

            // Generate comparison report
            generate_comparison_report(all_results);

            // Save individual results
            for (const auto &result : all_results)
            {
                save_ber_results(result);
            }

            std::cout << "\nBER Analysis Complete!\n";
            std::cout << "Results saved to: " << config_.output_directory << "\n";
        }

        /// Analyze specific error patterns
        void analyze_error_patterns()
        {
            std::cout << "\n=== Error Pattern Analysis ===\n";

            using Hamming74 = HammingCode<7, 4>;
            Hamming74 code;

            // Test different error patterns
            analyze_single_errors<Hamming74>("Hamming(7,4)");
            analyze_double_errors<Hamming74>("Hamming(7,4)");
            analyze_burst_errors<Hamming74>("Hamming(7,4)");
        }

        /// Analyze channel model comparison
        void analyze_channel_comparison()
        {
            std::cout << "\n=== Channel Model Comparison ===\n";

            using Hamming74 = HammingCode<7, 4>;

            const size_t iterations = 50000;

            std::vector<std::pair<std::string, BERResults>> channel_results;

            // BSC Channel
            channel_results.push_back({"BSC", analyze_channel_model<Hamming74>(
                                                  ErrorType::RANDOM, 0.01, iterations, "BSC")});

            // Burst Channel
            channel_results.push_back({"Burst", analyze_channel_model<Hamming74>(
                                                    ErrorType::BURST, 0.05, iterations, "Burst")});

            // Clustered Channel
            channel_results.push_back({"Clustered", analyze_channel_model<Hamming74>(
                                                        ErrorType::CLUSTERED, 0.01, iterations, "Clustered")});

            // Print comparison table
            print_channel_comparison(channel_results);
        }

    private:
        /// Analyze Hamming(7,4) code
        BERResults analyze_hamming_7_4()
        {
            using Hamming74 = HammingCode<7, 4>;
            return analyze_code<Hamming74>("Hamming(7,4)");
        }

        /// Analyze Hamming(15,11) code
        BERResults analyze_hamming_15_11()
        {
            using Hamming1511 = HammingCode<15, 11>;
            return analyze_code<Hamming1511>("Hamming(15,11)");
        }

        /// Generic code analysis template
        template <typename CodeType>
            requires CodecType<CodeType>
        BERResults analyze_code(const std::string &code_name)
        {
            std::cout << "Analyzing " << code_name << "...\n";

            BERResults results;
            results.code_name = code_name;

            CodeType code;

            for (double snr_db = config_.snr_min_db;
                 snr_db <= config_.snr_max_db;
                 snr_db += config_.snr_step_db)
            {
                std::cout << "  SNR: " << std::fixed << std::setprecision(1)
                          << snr_db << " dB... " << std::flush;

                auto metrics = analyze_snr_point<CodeType>(snr_db);

                results.snr_db_values.push_back(snr_db);
                results.ber_values.push_back(metrics.bit_error_rate);
                results.bler_values.push_back(metrics.block_error_rate);
                results.throughput_values.push_back(metrics.throughput_mbps);
                results.error_counts.push_back(metrics.error_bits);
                results.block_counts.push_back(metrics.total_blocks);

                std::cout << "BER: " << std::scientific << std::setprecision(2)
                          << metrics.bit_error_rate << "\n";
            }

            return results;
        }

        /// Analyze single SNR point
        template <typename CodeType>
            requires CodecType<CodeType>
        PerformanceMetrics analyze_snr_point(double snr_db)
        {
            CodeType code;
            PerformanceMetrics metrics{};

            // Configure AWGN channel
            ErrorParameters params;
            params.type = ErrorType::RANDOM;
            params.probability = snr_db;                      // SNR in dB
            params.seed = static_cast<size_t>(snr_db * 1000); // Unique seed per SNR

            simulator_.create_channel(ErrorType::RANDOM, params);

            size_t iterations = 0;
            size_t total_errors = 0;
            size_t block_errors = 0;

            auto start_time = std::chrono::high_resolution_clock::now();

            // Run until we have enough statistics or hit max iterations
            while ((total_errors < config_.min_errors && iterations < config_.max_iterations) ||
                   iterations < config_.iterations_per_point)
            {
                // Generate random data
                typename CodeType::DataWord data;
                generate_random_data(data);

                // Encode
                auto encode_start = std::chrono::high_resolution_clock::now();
                auto codeword = code.encode(data);
                auto encode_end = std::chrono::high_resolution_clock::now();

                // Convert to vector for error simulation
                std::vector<uint8_t> codeword_vec(CodeType::code_length);
                for (size_t i = 0; i < CodeType::code_length; ++i)
                {
                    codeword_vec[i] = codeword[i] ? 1 : 0;
                }

                // Add channel errors
                auto received_vec = add_awgn_errors(codeword_vec, snr_db);

                // Convert back to bitset
                typename CodeType::CodeWord received;
                for (size_t i = 0; i < CodeType::code_length; ++i)
                {
                    received[i] = received_vec[i];
                }

                // Count bit errors before correction
                size_t bit_errors = 0;
                for (size_t i = 0; i < CodeType::code_length; ++i)
                {
                    if (codeword[i] != received[i])
                        bit_errors++;
                }
                total_errors += bit_errors;

                // Decode
                auto decode_start = std::chrono::high_resolution_clock::now();
                auto decoded = code.decode(received);
                auto decode_end = std::chrono::high_resolution_clock::now();

                // Check if block error occurred
                bool is_block_error = false;
                for (size_t i = 0; i < CodeType::data_length; ++i)
                {
                    if (data[i] != decoded[i])
                    {
                        is_block_error = true;
                        break;
                    }
                }
                if (is_block_error)
                    block_errors++;

                // Update timing metrics
                metrics.encoding_time_ms += std::chrono::duration<double, std::milli>(
                                                encode_end - encode_start)
                                                .count();
                metrics.decoding_time_ms += std::chrono::duration<double, std::milli>(
                                                decode_end - decode_start)
                                                .count();

                iterations++;
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto total_time = std::chrono::duration<double>(end_time - start_time).count();

            // Calculate final metrics
            metrics.total_bits = iterations * CodeType::code_length;
            metrics.error_bits = total_errors;
            metrics.total_blocks = iterations;
            metrics.error_blocks = block_errors;
            metrics.bit_error_rate = static_cast<double>(total_errors) / metrics.total_bits;
            metrics.block_error_rate = static_cast<double>(block_errors) / iterations;
            metrics.throughput_mbps = (iterations * CodeType::data_length) / (total_time * 1e6);
            metrics.encoding_time_ms /= iterations;
            metrics.decoding_time_ms /= iterations;

            return metrics;
        }

        /// Generate random data for testing
        template <typename DataWord>
        void generate_random_data(DataWord &data)
        {
            static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            static std::uniform_int_distribution<int> dist(0, 1);

            for (size_t i = 0; i < data.size(); ++i)
            {
                data[i] = dist(rng);
            }
        }

        /// Add AWGN errors to codeword
        std::vector<uint8_t> add_awgn_errors(const std::vector<uint8_t> &codeword, double snr_db)
        {
            static std::mt19937 rng(42);

            auto result = codeword;
            double snr_linear = std::pow(10.0, snr_db / 10.0);
            double noise_variance = 1.0 / (2.0 * snr_linear);
            std::normal_distribution<double> noise_dist(0.0, std::sqrt(noise_variance));

            for (auto &bit : result)
            {
                double signal = (bit == 0) ? -1.0 : 1.0;
                double received_signal = signal + noise_dist(rng);
                bit = (received_signal > 0.0) ? 1 : 0;
            }

            return result;
        }

        /// Analyze specific channel model
        template <typename CodeType>
            requires CodecType<CodeType>
        BERResults analyze_channel_model(ErrorType channel_type, double parameter,
                                         size_t iterations, const std::string &name)
        {
            BERResults results;
            results.code_name = name;

            ErrorParameters params;
            params.type = channel_type;
            params.probability = parameter;
            params.seed = 12345;

            if (channel_type == ErrorType::BURST)
                params.burst_length = 3;
            if (channel_type == ErrorType::CLUSTERED)
                params.cluster_size = 2;

            simulator_.create_channel(channel_type, params);

            CodeType code;
            size_t total_errors = 0;
            size_t block_errors = 0;

            for (size_t iter = 0; iter < iterations; ++iter)
            {
                // Generate and encode data
                typename CodeType::DataWord data;
                generate_random_data(data);
                auto codeword = code.encode(data);

                // Convert and add errors
                std::vector<uint8_t> codeword_vec(CodeType::code_length);
                for (size_t i = 0; i < CodeType::code_length; ++i)
                {
                    codeword_vec[i] = codeword[i] ? 1 : 0;
                }

                auto received_vec = simulator_.apply_errors(codeword_vec);

                // Convert back and decode
                typename CodeType::CodeWord received;
                for (size_t i = 0; i < CodeType::code_length; ++i)
                {
                    received[i] = received_vec[i];
                }

                auto decoded = code.decode(received);

                // Count errors
                size_t bit_errors = 0;
                for (size_t i = 0; i < CodeType::code_length; ++i)
                {
                    if (codeword[i] != received[i])
                        bit_errors++;
                }
                total_errors += bit_errors;

                bool is_block_error = false;
                for (size_t i = 0; i < CodeType::data_length; ++i)
                {
                    if (data[i] != decoded[i])
                    {
                        is_block_error = true;
                        break;
                    }
                }
                if (is_block_error)
                    block_errors++;
            }

            // Store results
            results.snr_db_values.push_back(0.0); // Placeholder
            results.ber_values.push_back(static_cast<double>(total_errors) /
                                         (iterations * CodeType::code_length));
            results.bler_values.push_back(static_cast<double>(block_errors) / iterations);
            results.error_counts.push_back(total_errors);
            results.block_counts.push_back(iterations);

            return results;
        }

        /// Analyze single error patterns
        template <typename CodeType>
            requires CodecType<CodeType>
        void analyze_single_errors(const std::string &code_name)
        {
            std::cout << "\nSingle Error Analysis for " << code_name << ":\n";
            std::cout << std::string(50, '-') << "\n";

            CodeType code;
            ErrorPatternGenerator pattern_gen(54321);

            size_t successful_corrections = 0;
            const size_t total_positions = CodeType::code_length;

            for (size_t pos = 0; pos < total_positions; ++pos)
            {
                // Generate test data
                typename CodeType::DataWord test_data;
                for (size_t i = 0; i < CodeType::data_length; ++i)
                {
                    test_data[i] = (i % 2); // Alternating pattern
                }

                auto codeword = code.encode(test_data);
                auto corrupted = codeword;
                corrupted.flip(pos); // Single error at position pos

                auto decoded = code.decode(corrupted);

                bool success = (test_data == decoded);
                if (success)
                    successful_corrections++;

                if (pos < 10) // Print first 10 for verification
                {
                    std::cout << "Error at pos " << pos << ": "
                              << (success ? "CORRECTED" : "FAILED") << "\n";
                }
            }

            double success_rate = static_cast<double>(successful_corrections) / total_positions;
            std::cout << "Single error correction rate: " << std::fixed
                      << std::setprecision(2) << (success_rate * 100) << "% ("
                      << successful_corrections << "/" << total_positions << ")\n";
        }

        /// Analyze double error patterns
        template <typename CodeType>
            requires CodecType<CodeType>
        void analyze_double_errors(const std::string &code_name)
        {
            std::cout << "\nDouble Error Analysis for " << code_name << ":\n";
            std::cout << std::string(50, '-') << "\n";

            CodeType code;
            size_t detections = 0;
            size_t total_tests = 50; // Test subset of double error patterns

            for (size_t test = 0; test < total_tests; ++test)
            {
                size_t pos1 = test % CodeType::code_length;
                size_t pos2 = (test * 3) % CodeType::code_length;
                if (pos1 == pos2)
                    continue;

                // Generate test data
                typename CodeType::DataWord test_data;
                for (size_t i = 0; i < CodeType::data_length; ++i)
                {
                    test_data[i] = (test + i) % 2;
                }

                auto codeword = code.encode(test_data);
                auto corrupted = codeword;
                corrupted.flip(pos1);
                corrupted.flip(pos2);

                auto decoded = code.decode(corrupted);
                bool detected = !(test_data == decoded); // Error should be detected
                if (detected)
                    detections++;
            }

            double detection_rate = static_cast<double>(detections) / total_tests;
            std::cout << "Double error detection rate: " << std::fixed
                      << std::setprecision(2) << (detection_rate * 100) << "% ("
                      << detections << "/" << total_tests << ")\n";
        }

        /// Analyze burst error patterns
        template <typename CodeType>
            requires CodecType<CodeType>
        void analyze_burst_errors(const std::string &code_name)
        {
            std::cout << "\nBurst Error Analysis for " << code_name << ":\n";
            std::cout << std::string(50, '-') << "\n";

            CodeType code;
            ErrorPatternGenerator pattern_gen(98765);

            std::vector<size_t> burst_lengths = {2, 3, 4, 5};

            for (size_t burst_len : burst_lengths)
            {
                size_t corrections = 0;
                size_t tests = 20;

                for (size_t test = 0; test < tests; ++test)
                {
                    typename CodeType::DataWord test_data;
                    for (size_t i = 0; i < CodeType::data_length; ++i)
                    {
                        test_data[i] = (test + i) % 2;
                    }

                    auto codeword = code.encode(test_data);

                    // Apply burst error
                    auto error_pattern = pattern_gen.generate_burst_error(
                        CodeType::code_length, test % (CodeType::code_length - burst_len), burst_len);

                    auto corrupted = codeword;
                    for (size_t i = 0; i < CodeType::code_length; ++i)
                    {
                        if (error_pattern[i])
                            corrupted.flip(i);
                    }

                    auto decoded = code.decode(corrupted);
                    if (test_data == decoded)
                        corrections++;
                }

                double correction_rate = static_cast<double>(corrections) / tests;
                std::cout << "Burst length " << burst_len << ": " << std::fixed
                          << std::setprecision(1) << (correction_rate * 100) << "% corrected\n";
            }
        }

        /// Create output directory
        void create_output_directory()
        {
// For Windows/cross-platform compatibility
#ifdef _WIN32
            system(("mkdir " + config_.output_directory + " 2>nul").c_str());
#else
            system(("mkdir -p " + config_.output_directory).c_str());
#endif
        }

        /// Save BER results to CSV
        void save_ber_results(const BERResults &results)
        {
            if (!config_.save_to_csv)
                return;

            std::string filename = config_.output_directory + results.code_name + "_ber_results.csv";

            // Replace parentheses in filename
            std::replace(filename.begin(), filename.end(), '(', '_');
            std::replace(filename.begin(), filename.end(), ')', '_');
            std::replace(filename.begin(), filename.end(), ',', '_');

            std::ofstream file(filename);
            if (!file.is_open())
            {
                std::cerr << "Warning: Could not open " << filename << " for writing\n";
                return;
            }

            // Write CSV header
            file << "SNR_dB,BER,BLER,Throughput_Mbps,Error_Count,Block_Count\n";

            // Write data
            for (size_t i = 0; i < results.snr_db_values.size(); ++i)
            {
                file << std::fixed << std::setprecision(2) << results.snr_db_values[i] << ","
                     << std::scientific << std::setprecision(6) << results.ber_values[i] << ","
                     << results.bler_values[i] << ","
                     << std::fixed << std::setprecision(2) << results.throughput_values[i] << ","
                     << results.error_counts[i] << ","
                     << results.block_counts[i] << "\n";
            }

            std::cout << "Results saved to: " << filename << "\n";
        }

        /// Generate comparison report
        void generate_comparison_report(const std::vector<BERResults> &all_results)
        {
            std::cout << "\n=== BER Comparison Report ===\n";
            std::cout << std::string(80, '=') << "\n";

            // Print header
            std::cout << std::left << std::setw(15) << "Code"
                      << std::setw(10) << "SNR(dB)"
                      << std::setw(15) << "BER"
                      << std::setw(15) << "BLER"
                      << std::setw(15) << "Throughput"
                      << "\n";
            std::cout << std::string(80, '-') << "\n";

            // Print data for each code at selected SNR points
            std::vector<double> test_snr_points = {3.0, 6.0, 9.0};

            for (const auto &result : all_results)
            {
                for (double test_snr : test_snr_points)
                {
                    // Find closest SNR point
                    size_t best_idx = 0;
                    double min_diff = std::abs(result.snr_db_values[0] - test_snr);

                    for (size_t i = 1; i < result.snr_db_values.size(); ++i)
                    {
                        double diff = std::abs(result.snr_db_values[i] - test_snr);
                        if (diff < min_diff)
                        {
                            min_diff = diff;
                            best_idx = i;
                        }
                    }

                    std::cout << std::left << std::setw(15) << result.code_name
                              << std::setw(10) << std::fixed << std::setprecision(1)
                              << result.snr_db_values[best_idx]
                              << std::setw(15) << std::scientific << std::setprecision(2)
                              << result.ber_values[best_idx]
                              << std::setw(15) << result.bler_values[best_idx]
                              << std::setw(15) << std::fixed << std::setprecision(1)
                              << result.throughput_values[best_idx] << " Mbps\n";
                }
                std::cout << std::string(80, '-') << "\n";
            }
        }

        /// Print channel comparison
        void print_channel_comparison(const std::vector<std::pair<std::string, BERResults>> &results)
        {
            std::cout << std::string(60, '-') << "\n";
            std::cout << std::left << std::setw(15) << "Channel"
                      << std::setw(15) << "BER"
                      << std::setw(15) << "BLER"
                      << std::setw(15) << "Errors"
                      << "\n";
            std::cout << std::string(60, '-') << "\n";

            for (const auto &[name, result] : results)
            {
                if (!result.ber_values.empty())
                {
                    std::cout << std::left << std::setw(15) << name
                              << std::setw(15) << std::scientific << std::setprecision(2)
                              << result.ber_values[0]
                              << std::setw(15) << result.bler_values[0]
                              << std::setw(15) << result.error_counts[0]
                              << "\n";
                }
            }
        }
    };

    /// Main analysis function
    void analyze_ber_curves()
    {
        BERAnalysisConfig config;
        config.snr_min_db = 0.0;
        config.snr_max_db = 10.0;
        config.snr_step_db = 1.0;
        config.iterations_per_point = 10000;
        config.min_errors = 50;
        config.save_to_csv = true;

        BERAnalyzer analyzer(config);

        // Run comprehensive analysis
        analyzer.analyze_ber_curves();
        analyzer.analyze_error_patterns();
        analyzer.analyze_channel_comparison();

        std::cout << "\n=== Analysis Summary ===\n";
        std::cout << "✓ BER curves generated for multiple codes\n";
        std::cout << "✓ Error pattern analysis completed\n";
        std::cout << "✓ Channel model comparison finished\n";
        std::cout << "✓ Results saved to CSV files\n";
        std::cout << "\nBER Analysis Suite Complete!\n";
    }

}
