#pragma once

#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <concepts>
#include <bitset>

namespace ecc
{

    /// Channel models for error simulation
    enum class ChannelType
    {
        BSC,  // Binary Symmetric Channel
        AWGN, // Additive White Gaussian Noise
        BEC,  // Binary Erasure Channel
        BURST // Burst Error Channel
    };

    /// Performance metrics for error correction codes
    struct PerformanceMetrics
    {
        double bit_error_rate;
        double block_error_rate;
        double throughput_mbps;
        double encoding_time_ms;
        double decoding_time_ms;
        size_t total_bits;
        size_t error_bits;
        size_t total_blocks;
        size_t error_blocks;
        size_t corrected_errors;
        size_t uncorrectable_errors;
    };

    /// Concept for error correction codes
    template <typename T>
    concept ErrorCorrectionCode = requires(T code, typename T::DataWord data, typename T::CodeWord codeword) {
        typename T::DataWord;
        typename T::CodeWord;
        { code.encode(data) } -> std::convertible_to<typename T::CodeWord>;
        { T::code_length } -> std::convertible_to<size_t>;
        { T::data_length } -> std::convertible_to<size_t>;
    };

    /// Performance analyzer for error correction codes
    class PerformanceAnalyzer
    {
    private:
        std::mt19937 rng;

    public:
        PerformanceAnalyzer() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

        /// Analyze BER performance over SNR range
        template <ErrorCorrectionCode CodeType>
        std::vector<PerformanceMetrics> analyze_ber_curve(
            double snr_db_min, double snr_db_max, double snr_db_step,
            size_t iterations_per_point)
        {

            std::vector<PerformanceMetrics> results;

            for (double snr_db = snr_db_min; snr_db <= snr_db_max; snr_db += snr_db_step)
            {
                auto metrics = analyze_performance<CodeType>(
                    ChannelType::AWGN, snr_db, iterations_per_point);
                results.push_back(metrics);

                std::cout << "SNR: " << std::fixed << std::setprecision(1) << snr_db
                          << " dB, BER: " << std::scientific << std::setprecision(2)
                          << metrics.bit_error_rate << std::endl;
            }

            return results;
        }

        /// Analyze performance for specific channel and SNR
        template <ErrorCorrectionCode CodeType>
        PerformanceMetrics analyze_performance(
            ChannelType channel, double parameter, size_t iterations)
        {

            CodeType code;
            PerformanceMetrics metrics{};

            auto start_time = std::chrono::high_resolution_clock::now();

            for (size_t iter = 0; iter < iterations; ++iter)
            {
                // Generate random data
                auto data = generate_random_data<CodeType>();

                // Encode
                auto encode_start = std::chrono::high_resolution_clock::now();
                auto codeword = code.encode(data);
                auto encode_end = std::chrono::high_resolution_clock::now();

                metrics.encoding_time_ms += std::chrono::duration<double, std::milli>(
                                                encode_end - encode_start)
                                                .count();

                // Add channel errors
                auto received = add_channel_errors(codeword, channel, parameter);

                // Count bit errors before correction
                auto bit_errors = count_bit_errors(codeword, received);
                metrics.error_bits += bit_errors;

                // Decode
                auto decode_start = std::chrono::high_resolution_clock::now();
                auto decoded_data = code.decode(received);
                auto decode_end = std::chrono::high_resolution_clock::now();

                metrics.decoding_time_ms += std::chrono::duration<double, std::milli>(
                                                decode_end - decode_start)
                                                .count();

                // Count remaining errors
                bool block_error = false;
                for (size_t i = 0; i < CodeType::data_length; ++i)
                {
                    if constexpr (std::is_same_v<typename CodeType::DataWord, std::bitset<CodeType::data_length>>)
                    {
                        if (data[i] != decoded_data[i])
                        {
                            block_error = true;
                            break;
                        }
                    }
                    else
                    {
                        if (!std::equal(data.begin(), data.end(), decoded_data.begin()))
                        {
                            block_error = true;
                            break;
                        }
                    }
                }
                if (block_error)
                {
                    metrics.error_blocks++;
                }

                metrics.total_bits += CodeType::code_length;
                metrics.total_blocks++;
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto total_time = std::chrono::duration<double>(end_time - start_time).count();

            // Calculate final metrics
            metrics.bit_error_rate = static_cast<double>(metrics.error_bits) / metrics.total_bits;
            metrics.block_error_rate = static_cast<double>(metrics.error_blocks) / metrics.total_blocks;
            metrics.throughput_mbps = (metrics.total_bits * CodeType::data_length / CodeType::code_length) / (total_time * 1e6);
            metrics.encoding_time_ms /= iterations;
            metrics.decoding_time_ms /= iterations;

            return metrics;
        }

        /// Save results to CSV file
        void save_results(const std::vector<PerformanceMetrics> &results,
                          const std::string &filename) const
        {
            std::ofstream file(filename);
            if (!file.is_open())
            {
                throw std::runtime_error("Cannot open file: " + filename);
            }

            // Write header
            file << "BER,BLER,Throughput_Mbps,Encoding_Time_ms,Decoding_Time_ms,"
                 << "Total_Bits,Error_Bits,Total_Blocks,Error_Blocks\n";

            // Write data
            for (const auto &metrics : results)
            {
                file << std::scientific << std::setprecision(6)
                     << metrics.bit_error_rate << ","
                     << metrics.block_error_rate << ","
                     << std::fixed << std::setprecision(2)
                     << metrics.throughput_mbps << ","
                     << metrics.encoding_time_ms << ","
                     << metrics.decoding_time_ms << ","
                     << metrics.total_bits << ","
                     << metrics.error_bits << ","
                     << metrics.total_blocks << ","
                     << metrics.error_blocks << "\n";
            }
        }

        /// Compare multiple codes
        template <ErrorCorrectionCode... CodeTypes>
        void compare_codes(ChannelType channel, double parameter, size_t iterations)
        {
            std::cout << "\nCode Comparison Results:\n";
            std::cout << std::string(80, '=') << "\n";
            std::cout << std::left << std::setw(15) << "Code"
                      << std::setw(12) << "BER"
                      << std::setw(12) << "BLER"
                      << std::setw(15) << "Throughput"
                      << std::setw(12) << "Enc Time"
                      << std::setw(12) << "Dec Time" << "\n";
            std::cout << std::string(80, '-') << "\n";

            auto print_metrics = [this, channel, parameter, iterations](const std::string &name, auto &&code_type)
            {
                using CodeType = std::decay_t<decltype(code_type)>;
                auto metrics = this->analyze_performance<CodeType>(channel, parameter, iterations);

                std::cout << std::left << std::setw(15) << name
                          << std::scientific << std::setprecision(2) << std::setw(12) << metrics.bit_error_rate
                          << std::setw(12) << metrics.block_error_rate
                          << std::fixed << std::setprecision(1) << std::setw(15) << metrics.throughput_mbps
                          << std::setprecision(3) << std::setw(12) << metrics.encoding_time_ms
                          << std::setw(12) << metrics.decoding_time_ms << "\n";
            };

            (print_metrics(get_code_name<CodeTypes>(), CodeTypes{}), ...);
        }

    private:
        template <typename CodeType>
        typename CodeType::DataWord generate_random_data()
        {
            typename CodeType::DataWord data;
            std::uniform_int_distribution<int> dist(0, 1);

            if constexpr (std::is_same_v<typename CodeType::DataWord, std::bitset<CodeType::data_length>>)
            {
                for (size_t i = 0; i < CodeType::data_length; ++i)
                {
                    data[i] = dist(rng);
                }
            }
            else
            {
                for (auto &element : data)
                {
                    element = dist(rng);
                }
            }

            return data;
        }

        template <typename CodeWord>
        CodeWord add_channel_errors(const CodeWord &codeword, ChannelType channel, double parameter)
        {
            auto received = codeword;

            switch (channel)
            {
            case ChannelType::BSC:
                add_bsc_errors(received, parameter);
                break;
            case ChannelType::AWGN:
                add_awgn_errors(received, parameter);
                break;
            case ChannelType::BEC:
                add_bec_errors(received, parameter);
                break;
            case ChannelType::BURST:
                add_burst_errors(received, static_cast<size_t>(parameter));
                break;
            }

            return received;
        }

        template <typename CodeWord>
        void add_bsc_errors(CodeWord &codeword, double error_probability)
        {
            std::uniform_real_distribution<double> dist(0.0, 1.0);

            if constexpr (std::is_same_v<CodeWord, std::bitset<std::tuple_size_v<CodeWord>>>)
            {
                for (size_t i = 0; i < codeword.size(); ++i)
                {
                    if (dist(rng) < error_probability)
                    {
                        codeword.flip(i);
                    }
                }
            }
            else
            {
                for (auto &bit : codeword)
                {
                    if (dist(rng) < error_probability)
                    {
                        bit = (bit == 0) ? 1 : 0;
                    }
                }
            }
        }

        template <typename CodeWord>
        void add_awgn_errors(CodeWord &codeword, double snr_db)
        {
            double snr_linear = std::pow(10.0, snr_db / 10.0);
            double noise_variance = 1.0 / (2.0 * snr_linear);
            std::normal_distribution<double> noise_dist(0.0, std::sqrt(noise_variance));

            if constexpr (std::is_same_v<CodeWord, std::bitset<std::tuple_size_v<CodeWord>>>)
            {
                for (size_t i = 0; i < codeword.size(); ++i)
                {
                    double signal = codeword[i] ? 1.0 : -1.0;
                    double received_signal = signal + noise_dist(rng);
                    codeword[i] = (received_signal > 0.0);
                }
            }
            else
            {
                for (auto &bit : codeword)
                {
                    double signal = (bit == 0) ? -1.0 : 1.0;
                    double received_signal = signal + noise_dist(rng);
                    bit = (received_signal > 0.0) ? 1 : 0;
                }
            }
        }

        template <typename CodeWord>
        void add_bec_errors(CodeWord &codeword, double erasure_probability)
        {
            std::uniform_real_distribution<double> dist(0.0, 1.0);

            if constexpr (std::is_same_v<CodeWord, std::bitset<std::tuple_size_v<CodeWord>>>)
            {
                for (size_t i = 0; i < codeword.size(); ++i)
                {
                    if (dist(rng) < erasure_probability)
                    {
                        codeword[i] = 0; // Simple implementation
                    }
                }
            }
            else
            {
                for (auto &bit : codeword)
                {
                    if (dist(rng) < erasure_probability)
                    {
                        // Mark as erasure (could use special value)
                        bit = 0; // Simple implementation
                    }
                }
            }
        }

        template <typename CodeWord>
        void add_burst_errors(CodeWord &codeword, size_t burst_length)
        {
            if (codeword.size() < burst_length)
                return;

            std::uniform_int_distribution<size_t> pos_dist(0, codeword.size() - burst_length);
            size_t start_pos = pos_dist(rng);

            for (size_t i = start_pos; i < start_pos + burst_length; ++i)
            {
                codeword[i] = (codeword[i] == 0) ? 1 : 0;
            }
        }

        template <typename CodeWord>
        size_t count_bit_errors(const CodeWord &original, const CodeWord &received)
        {
            size_t errors = 0;
            for (size_t i = 0; i < original.size(); ++i)
            {
                if (original[i] != received[i])
                {
                    errors++;
                }
            }
            return errors;
        }

        template <typename CodeType>
        std::string get_code_name()
        {
            // Simple name extraction (could be improved with type traits)
            std::string name = typeid(CodeType).name();
            if (name.find("Hamming") != std::string::npos)
            {
                return "Hamming(" + std::to_string(CodeType::code_length) + "," + std::to_string(CodeType::data_length) + ")";
            }
            else if (name.find("ReedSolomon") != std::string::npos)
            {
                return "RS(" + std::to_string(CodeType::code_length) + "," + std::to_string(CodeType::data_length) + ")";
            }
            return "Unknown";
        }
    };

    /// Error pattern analyzer
    class ErrorPatternAnalyzer
    {
    private:
        std::mt19937 rng;

    public:
        ErrorPatternAnalyzer() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

        /// Analyze error correction patterns
        template <ErrorCorrectionCode CodeType>
        void analyze_error_patterns(size_t max_errors, size_t iterations_per_pattern)
        {
            CodeType code;

            std::cout << "\nError Pattern Analysis for " << get_code_name<CodeType>() << ":\n";
            std::cout << std::string(60, '=') << "\n";
            std::cout << std::left << std::setw(12) << "Errors"
                      << std::setw(15) << "Success Rate"
                      << std::setw(15) << "Avg Time (ms)"
                      << std::setw(15) << "Pattern Type" << "\n";
            std::cout << std::string(60, '-') << "\n";

            for (size_t num_errors = 1; num_errors <= max_errors; ++num_errors)
            {
                // Test random error patterns
                auto random_results = test_error_pattern<CodeType>(
                    code, num_errors, iterations_per_pattern, false);

                // Test burst error patterns
                auto burst_results = test_error_pattern<CodeType>(
                    code, num_errors, iterations_per_pattern, true);

                std::cout << std::left << std::setw(12) << num_errors
                          << std::fixed << std::setprecision(2) << std::setw(15)
                          << (random_results.first * 100.0) << "%"
                          << std::setw(15) << random_results.second
                          << std::setw(15) << "Random" << "\n";

                std::cout << std::left << std::setw(12) << ""
                          << std::fixed << std::setprecision(2) << std::setw(15)
                          << (burst_results.first * 100.0) << "%"
                          << std::setw(15) << burst_results.second
                          << std::setw(15) << "Burst" << "\n";
            }
        }

    private:
        template <ErrorCorrectionCode CodeType>
        std::pair<double, double> test_error_pattern(
            const CodeType &code, size_t num_errors, size_t iterations, bool burst_pattern)
        {

            size_t successes = 0;
            double total_time = 0.0;

            for (size_t iter = 0; iter < iterations; ++iter)
            {
                // Generate random data
                typename CodeType::DataWord data;
                std::uniform_int_distribution<int> bit_dist(0, 1);
                for (auto &bit : data)
                {
                    bit = bit_dist(rng);
                }

                // Encode
                auto codeword = code.encode(data);
                auto received = codeword;

                // Add errors
                if (burst_pattern)
                {
                    add_burst_error_pattern(received, num_errors);
                }
                else
                {
                    add_random_error_pattern(received, num_errors);
                }

                // Decode and measure time
                auto start = std::chrono::high_resolution_clock::now();
                auto decoded = code.decode(received);
                auto end = std::chrono::high_resolution_clock::now();

                total_time += std::chrono::duration<double, std::milli>(end - start).count();

                // Check if decoding was successful
                if (std::equal(data.begin(), data.end(), decoded.begin()))
                {
                    successes++;
                }
            }

            double success_rate = static_cast<double>(successes) / iterations;
            double avg_time = total_time / iterations;

            return {success_rate, avg_time};
        }

        template <typename CodeWord>
        void add_random_error_pattern(CodeWord &codeword, size_t num_errors)
        {
            std::vector<size_t> positions(codeword.size());
            std::iota(positions.begin(), positions.end(), 0);
            std::shuffle(positions.begin(), positions.end(), rng);

            for (size_t i = 0; i < std::min(num_errors, positions.size()); ++i)
            {
                size_t pos = positions[i];
                codeword[pos] = (codeword[pos] == 0) ? 1 : 0;
            }
        }

        template <typename CodeWord>
        void add_burst_error_pattern(CodeWord &codeword, size_t burst_length)
        {
            if (codeword.size() < burst_length)
                return;

            std::uniform_int_distribution<size_t> pos_dist(0, codeword.size() - burst_length);
            size_t start_pos = pos_dist(rng);

            for (size_t i = start_pos; i < start_pos + burst_length; ++i)
            {
                codeword[i] = (codeword[i] == 0) ? 1 : 0;
            }
        }

        template <typename CodeType>
        std::string get_code_name()
        {
            return "Code(" + std::to_string(CodeType::code_length) + "," + std::to_string(CodeType::data_length) + ")";
        }
    };

} // namespace ecc
