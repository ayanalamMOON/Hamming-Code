#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <bitset>
#include <cmath>
#include <cassert>
#include <memory>
#include <set>

// Include our error simulator directly
namespace ecc
{
    // Copy the necessary enums and classes from error_simulator.cpp
    enum class ErrorType
    {
        RANDOM,    // Random bit errors
        BURST,     // Burst errors
        PERIODIC,  // Periodic errors
        CLUSTERED, // Clustered errors
        ERASURE,   // Erasure errors
        FADING     // Fading channel errors
    };

    struct ErrorParameters
    {
        ErrorType type = ErrorType::RANDOM;
        double probability = 0.01;     // Error probability or SNR
        size_t burst_length = 5;       // For burst errors
        size_t cluster_size = 3;       // For clustered errors
        size_t period = 7;             // For periodic errors
        double fading_amplitude = 0.5; // For fading channels
        size_t seed = 42;              // Random seed for reproducibility
    };

    class ChannelModel
    {
    public:
        virtual ~ChannelModel() = default;
        virtual std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) = 0;
        virtual void set_parameters(const ErrorParameters &params) = 0;
        virtual std::string get_name() const = 0;
    };

    class BSCChannel : public ChannelModel
    {
    private:
        ErrorParameters params_;
        mutable std::mt19937 rng_;
        mutable std::uniform_real_distribution<double> dist_;

    public:
        BSCChannel(const ErrorParameters &params = {})
            : params_(params), rng_(params.seed), dist_(0.0, 1.0) {}

        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) override
        {
            auto result = codeword;
            for (auto &bit : result)
            {
                if (dist_(rng_) < params_.probability)
                {
                    bit = (bit == 0) ? 1 : 0;
                }
            }
            return result;
        }

        void set_parameters(const ErrorParameters &params) override
        {
            params_ = params;
            rng_.seed(params.seed);
        }

        std::string get_name() const override
        {
            return "BSC(p=" + std::to_string(params_.probability) + ")";
        }
    };

    class BurstErrorChannel : public ChannelModel
    {
    private:
        ErrorParameters params_;
        mutable std::mt19937 rng_;

    public:
        BurstErrorChannel(const ErrorParameters &params = {})
            : params_(params), rng_(params.seed) {}

        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) override
        {
            auto result = codeword;

            if (result.size() < params_.burst_length)
                return result;

            std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
            std::uniform_int_distribution<size_t> pos_dist(0, result.size() - params_.burst_length);

            // Decide if a burst occurs
            if (prob_dist(rng_) < params_.probability)
            {
                size_t start_pos = pos_dist(rng_);
                for (size_t i = start_pos; i < start_pos + params_.burst_length; ++i)
                {
                    result[i] = (result[i] == 0) ? 1 : 0;
                }
            }

            return result;
        }

        void set_parameters(const ErrorParameters &params) override
        {
            params_ = params;
            rng_.seed(params.seed);
        }

        std::string get_name() const override
        {
            return "Burst(p=" + std::to_string(params_.probability) +
                   ",len=" + std::to_string(params_.burst_length) + ")";
        }
    };

    class ErrorPatternGenerator
    {
    private:
        std::mt19937 rng_;

    public:
        ErrorPatternGenerator(size_t seed = 42) : rng_(seed) {}

        std::vector<uint8_t> generate_single_error(size_t codeword_length, size_t error_position)
        {
            std::vector<uint8_t> errors(codeword_length, 0);
            if (error_position < codeword_length)
            {
                errors[error_position] = 1;
            }
            return errors;
        }

        std::vector<uint8_t> generate_double_error(size_t codeword_length,
                                                   size_t pos1, size_t pos2)
        {
            std::vector<uint8_t> errors(codeword_length, 0);
            if (pos1 < codeword_length)
                errors[pos1] = 1;
            if (pos2 < codeword_length && pos2 != pos1)
                errors[pos2] = 1;
            return errors;
        }

        std::vector<uint8_t> generate_burst_error(size_t codeword_length,
                                                  size_t start_pos, size_t burst_length)
        {
            std::vector<uint8_t> errors(codeword_length, 0);
            for (size_t i = start_pos; i < std::min(start_pos + burst_length, codeword_length); ++i)
            {
                errors[i] = 1;
            }
            return errors;
        }

        std::vector<uint8_t> generate_random_errors(size_t codeword_length, size_t num_errors)
        {
            std::vector<uint8_t> errors(codeword_length, 0);
            std::vector<size_t> positions(codeword_length);
            std::iota(positions.begin(), positions.end(), 0);
            std::shuffle(positions.begin(), positions.end(), rng_);

            for (size_t i = 0; i < std::min(num_errors, codeword_length); ++i)
            {
                errors[positions[i]] = 1;
            }
            return errors;
        }
    };

    class ErrorSimulator
    {
    private:
        std::unique_ptr<ChannelModel> channel_;
        ErrorPatternGenerator pattern_gen_;
        std::mt19937 rng_;

    public:
        ErrorSimulator(size_t seed = 42) : pattern_gen_(seed), rng_(seed) {}

        void create_channel(ErrorType type, const ErrorParameters &params = {})
        {
            ErrorParameters local_params = params;
            local_params.type = type;

            switch (type)
            {
            case ErrorType::RANDOM:
                channel_ = std::make_unique<BSCChannel>(local_params);
                break;
            case ErrorType::BURST:
                channel_ = std::make_unique<BurstErrorChannel>(local_params);
                break;
            default:
                channel_ = std::make_unique<BSCChannel>(local_params);
                break;
            }
        }

        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword)
        {
            if (!channel_)
            {
                throw std::runtime_error("No channel model set");
            }
            return channel_->apply_errors(codeword);
        }

        std::vector<uint8_t> apply_error_pattern(const std::vector<uint8_t> &codeword,
                                                 const std::vector<uint8_t> &error_pattern)
        {
            if (codeword.size() != error_pattern.size())
            {
                throw std::invalid_argument("Codeword and error pattern size mismatch");
            }

            std::vector<uint8_t> result = codeword;
            for (size_t i = 0; i < result.size(); ++i)
            {
                if (error_pattern[i])
                {
                    result[i] = (result[i] == 0) ? 1 : 0;
                }
            }
            return result;
        }

        struct ErrorStatistics
        {
            size_t total_bits;
            size_t error_bits;
            size_t error_blocks;
            double bit_error_rate;
            double block_error_rate;
            std::vector<size_t> error_positions;
        };

        ErrorStatistics analyze_errors(const std::vector<uint8_t> &original,
                                       const std::vector<uint8_t> &received)
        {
            ErrorStatistics stats{};
            stats.total_bits = original.size();

            for (size_t i = 0; i < original.size(); ++i)
            {
                if (original[i] != received[i])
                {
                    stats.error_bits++;
                    stats.error_positions.push_back(i);
                }
            }

            stats.bit_error_rate = static_cast<double>(stats.error_bits) / stats.total_bits;
            stats.error_blocks = (stats.error_bits > 0) ? 1 : 0;
            stats.block_error_rate = static_cast<double>(stats.error_blocks);

            return stats;
        }

        ErrorPatternGenerator &get_pattern_generator() { return pattern_gen_; }

        std::string get_channel_name() const
        {
            return channel_ ? channel_->get_name() : "No Channel";
        }
    };
}

int main()
{
    using namespace ecc;

    std::cout << "Error Simulator Test Program\n";
    std::cout << std::string(50, '=') << "\n\n";

    // Create a simple test codeword
    std::vector<uint8_t> test_codeword = {1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1};

    std::cout << "Original codeword: ";
    for (auto bit : test_codeword)
    {
        std::cout << static_cast<int>(bit);
    }
    std::cout << "\n\n";

    // Test different error types
    ErrorSimulator simulator;

    // 1. Test BSC Channel
    std::cout << "1. Binary Symmetric Channel (BSC) Test\n";
    std::cout << std::string(40, '-') << "\n";

    ErrorParameters bsc_params;
    bsc_params.type = ErrorType::RANDOM;
    bsc_params.probability = 0.1; // 10% error rate
    bsc_params.seed = 123;

    simulator.create_channel(ErrorType::RANDOM, bsc_params);

    for (int i = 0; i < 5; ++i)
    {
        auto corrupted = simulator.apply_errors(test_codeword);
        auto stats = simulator.analyze_errors(test_codeword, corrupted);

        std::cout << "Trial " << (i + 1) << ": ";
        for (auto bit : corrupted)
        {
            std::cout << static_cast<int>(bit);
        }
        std::cout << " (BER: " << std::fixed << std::setprecision(3)
                  << stats.bit_error_rate << ")\n";
    }

    // 2. Test Burst Error Channel
    std::cout << "\n2. Burst Error Channel Test\n";
    std::cout << std::string(40, '-') << "\n";

    ErrorParameters burst_params;
    burst_params.type = ErrorType::BURST;
    burst_params.probability = 0.8; // 80% chance of burst
    burst_params.burst_length = 3;
    burst_params.seed = 456;

    simulator.create_channel(ErrorType::BURST, burst_params);

    for (int i = 0; i < 5; ++i)
    {
        auto corrupted = simulator.apply_errors(test_codeword);
        auto stats = simulator.analyze_errors(test_codeword, corrupted);

        std::cout << "Trial " << (i + 1) << ": ";
        for (auto bit : corrupted)
        {
            std::cout << static_cast<int>(bit);
        }
        std::cout << " (Errors: " << stats.error_bits << ")\n";
    }

    // 3. Test Error Pattern Generator
    std::cout << "\n3. Error Pattern Generator Test\n";
    std::cout << std::string(40, '-') << "\n";

    auto &pattern_gen = simulator.get_pattern_generator();

    // Single error
    auto single_error = pattern_gen.generate_single_error(test_codeword.size(), 5);
    auto single_corrupted = simulator.apply_error_pattern(test_codeword, single_error);
    std::cout << "Single error at position 5: ";
    for (auto bit : single_corrupted)
    {
        std::cout << static_cast<int>(bit);
    }
    std::cout << "\n";

    // Double error
    auto double_error = pattern_gen.generate_double_error(test_codeword.size(), 2, 8);
    auto double_corrupted = simulator.apply_error_pattern(test_codeword, double_error);
    std::cout << "Double error at positions 2,8: ";
    for (auto bit : double_corrupted)
    {
        std::cout << static_cast<int>(bit);
    }
    std::cout << "\n";

    // Burst error
    auto burst_error = pattern_gen.generate_burst_error(test_codeword.size(), 6, 4);
    auto burst_corrupted = simulator.apply_error_pattern(test_codeword, burst_error);
    std::cout << "Burst error from position 6 (length 4): ";
    for (auto bit : burst_corrupted)
    {
        std::cout << static_cast<int>(bit);
    }
    std::cout << "\n";

    // 4. Test Error Statistics
    std::cout << "\n4. Error Statistics Test\n";
    std::cout << std::string(40, '-') << "\n";

    auto random_errors = pattern_gen.generate_random_errors(test_codeword.size(), 4);
    auto random_corrupted = simulator.apply_error_pattern(test_codeword, random_errors);
    auto detailed_stats = simulator.analyze_errors(test_codeword, random_corrupted);

    std::cout << "Random 4 errors: ";
    for (auto bit : random_corrupted)
    {
        std::cout << static_cast<int>(bit);
    }
    std::cout << "\n";
    std::cout << "Total bits: " << detailed_stats.total_bits << "\n";
    std::cout << "Error bits: " << detailed_stats.error_bits << "\n";
    std::cout << "Bit error rate: " << std::fixed << std::setprecision(4)
              << detailed_stats.bit_error_rate << "\n";
    std::cout << "Error positions: ";
    for (auto pos : detailed_stats.error_positions)
    {
        std::cout << pos << " ";
    }
    std::cout << "\n";

    // 5. Test Channel Performance
    std::cout << "\n5. Channel Performance Comparison\n";
    std::cout << std::string(40, '-') << "\n";

    const size_t num_trials = 1000;

    // BSC Channel performance
    ErrorParameters perf_bsc;
    perf_bsc.probability = 0.05;
    perf_bsc.seed = 1000;
    simulator.create_channel(ErrorType::RANDOM, perf_bsc);

    size_t total_bsc_errors = 0;
    for (size_t trial = 0; trial < num_trials; ++trial)
    {
        auto corrupted = simulator.apply_errors(test_codeword);
        auto stats = simulator.analyze_errors(test_codeword, corrupted);
        total_bsc_errors += stats.error_bits;
    }

    double avg_bsc_ber = static_cast<double>(total_bsc_errors) / (num_trials * test_codeword.size());

    // Burst Channel performance
    ErrorParameters perf_burst;
    perf_burst.probability = 0.1;
    perf_burst.burst_length = 3;
    perf_burst.seed = 2000;
    simulator.create_channel(ErrorType::BURST, perf_burst);

    size_t total_burst_errors = 0;
    for (size_t trial = 0; trial < num_trials; ++trial)
    {
        auto corrupted = simulator.apply_errors(test_codeword);
        auto stats = simulator.analyze_errors(test_codeword, corrupted);
        total_burst_errors += stats.error_bits;
    }

    double avg_burst_ber = static_cast<double>(total_burst_errors) / (num_trials * test_codeword.size());

    std::cout << "BSC Channel (p=0.05): Average BER = " << std::scientific
              << std::setprecision(4) << avg_bsc_ber << "\n";
    std::cout << "Burst Channel (p=0.1, len=3): Average BER = " << std::scientific
              << std::setprecision(4) << avg_burst_ber << "\n";

    std::cout << "\nError Simulator Test Completed Successfully!\n";

    return 0;
}
