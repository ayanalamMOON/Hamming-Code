#include <iostream>
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
#include <iomanip>

namespace ecc
{

    /// Error types for different simulation scenarios
    enum class ErrorType
    {
        RANDOM,    // Random bit errors
        BURST,     // Burst errors
        PERIODIC,  // Periodic errors
        CLUSTERED, // Clustered errors
        ERASURE,   // Erasure errors
        FADING     // Fading channel errors
    };

    /// Error injection parameters
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

    /// Channel model interface
    class ChannelModel
    {
    public:
        virtual ~ChannelModel() = default;
        virtual std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) = 0;
        virtual void set_parameters(const ErrorParameters &params) = 0;
        virtual std::string get_name() const = 0;
    };

    /// Binary Symmetric Channel (BSC)
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

    /// Additive White Gaussian Noise (AWGN) Channel
    class AWGNChannel : public ChannelModel
    {
    private:
        ErrorParameters params_;
        mutable std::mt19937 rng_;
        mutable std::normal_distribution<double> noise_dist_;

    public:
        AWGNChannel(const ErrorParameters &params = {})
            : params_(params), rng_(params.seed), noise_dist_(0.0, 1.0) {}

        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) override
        {
            auto result = codeword;
            double snr_linear = std::pow(10.0, params_.probability / 10.0); // SNR in dB
            double noise_variance = 1.0 / (2.0 * snr_linear);

            noise_dist_ = std::normal_distribution<double>(0.0, std::sqrt(noise_variance));

            for (auto &bit : result)
            {
                double signal = (bit == 0) ? -1.0 : 1.0;
                double received_signal = signal + noise_dist_(rng_);
                bit = (received_signal > 0.0) ? 1 : 0;
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
            return "AWGN(SNR=" + std::to_string(params_.probability) + "dB)";
        }
    };

    /// Burst Error Channel
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

    /// Clustered Error Channel
    class ClusteredErrorChannel : public ChannelModel
    {
    private:
        ErrorParameters params_;
        mutable std::mt19937 rng_;

    public:
        ClusteredErrorChannel(const ErrorParameters &params = {})
            : params_(params), rng_(params.seed) {}

        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) override
        {
            auto result = codeword;
            std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
            std::uniform_int_distribution<size_t> pos_dist(0, result.size() - 1);

            // Create error clusters
            size_t num_clusters = static_cast<size_t>(result.size() * params_.probability / params_.cluster_size);

            for (size_t cluster = 0; cluster < num_clusters; ++cluster)
            {
                size_t center = pos_dist(rng_);
                size_t half_cluster = params_.cluster_size / 2;

                size_t start = (center >= half_cluster) ? center - half_cluster : 0;
                size_t end = std::min(center + half_cluster + 1, result.size());

                for (size_t i = start; i < end; ++i)
                {
                    if (prob_dist(rng_) < 0.8) // High probability within cluster
                    {
                        result[i] = (result[i] == 0) ? 1 : 0;
                    }
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
            return "Clustered(p=" + std::to_string(params_.probability) +
                   ",size=" + std::to_string(params_.cluster_size) + ")";
        }
    };

    /// Erasure Channel
    class ErasureChannel : public ChannelModel
    {
    private:
        ErrorParameters params_;
        mutable std::mt19937 rng_;
        mutable std::uniform_real_distribution<double> dist_;

    public:
        ErasureChannel(const ErrorParameters &params = {})
            : params_(params), rng_(params.seed), dist_(0.0, 1.0) {}

        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) override
        {
            auto result = codeword;
            for (auto &bit : result)
            {
                if (dist_(rng_) < params_.probability)
                {
                    bit = 2; // Use value 2 to indicate erasure (if supported)
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
            return "Erasure(p=" + std::to_string(params_.probability) + ")";
        }
    };

    /// Fading Channel
    class FadingChannel : public ChannelModel
    {
    private:
        ErrorParameters params_;
        mutable std::mt19937 rng_;
        mutable std::normal_distribution<double> fading_dist_;
        mutable std::normal_distribution<double> noise_dist_;

    public:
        FadingChannel(const ErrorParameters &params = {})
            : params_(params), rng_(params.seed),
              fading_dist_(0.0, params.fading_amplitude),
              noise_dist_(0.0, 1.0) {}

        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword) override
        {
            auto result = codeword;
            double snr_linear = std::pow(10.0, params_.probability / 10.0);
            double noise_variance = 1.0 / (2.0 * snr_linear);

            noise_dist_ = std::normal_distribution<double>(0.0, std::sqrt(noise_variance));

            for (auto &bit : result)
            {
                double signal = (bit == 0) ? -1.0 : 1.0;
                double fading_coeff = 1.0 + fading_dist_(rng_);
                double received_signal = fading_coeff * signal + noise_dist_(rng_);
                bit = (received_signal > 0.0) ? 1 : 0;
            }
            return result;
        }

        void set_parameters(const ErrorParameters &params) override
        {
            params_ = params;
            fading_dist_ = std::normal_distribution<double>(0.0, params.fading_amplitude);
            rng_.seed(params.seed);
        }

        std::string get_name() const override
        {
            return "Fading(SNR=" + std::to_string(params_.probability) +
                   "dB,fade=" + std::to_string(params_.fading_amplitude) + ")";
        }
    };

    /// Error Pattern Generator
    class ErrorPatternGenerator
    {
    private:
        std::mt19937 rng_;

    public:
        ErrorPatternGenerator(size_t seed = 42) : rng_(seed) {}

        /// Generate specific error patterns for testing
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

        std::vector<uint8_t> generate_triple_error(size_t codeword_length,
                                                   size_t pos1, size_t pos2, size_t pos3)
        {
            std::vector<uint8_t> errors(codeword_length, 0);
            std::set<size_t> positions = {pos1, pos2, pos3};
            for (size_t pos : positions)
            {
                if (pos < codeword_length)
                    errors[pos] = 1;
            }
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

        std::vector<uint8_t> generate_weight_pattern(size_t codeword_length, size_t weight)
        {
            return generate_random_errors(codeword_length, weight);
        }
    };

    /// Main Error Simulator class
    class ErrorSimulator
    {
    private:
        std::unique_ptr<ChannelModel> channel_;
        ErrorPatternGenerator pattern_gen_;
        std::mt19937 rng_;

    public:
        ErrorSimulator(size_t seed = 42) : pattern_gen_(seed), rng_(seed) {}

        /// Set channel model
        void set_channel(std::unique_ptr<ChannelModel> channel)
        {
            channel_ = std::move(channel);
        }

        /// Create channel by type
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
            case ErrorType::CLUSTERED:
                channel_ = std::make_unique<ClusteredErrorChannel>(local_params);
                break;
            case ErrorType::ERASURE:
                channel_ = std::make_unique<ErasureChannel>(local_params);
                break;
            case ErrorType::FADING:
                channel_ = std::make_unique<FadingChannel>(local_params);
                break;
            default:
                channel_ = std::make_unique<AWGNChannel>(local_params);
                break;
            }
        }

        /// Apply errors to codeword
        std::vector<uint8_t> apply_errors(const std::vector<uint8_t> &codeword)
        {
            if (!channel_)
            {
                throw std::runtime_error("No channel model set");
            }
            return channel_->apply_errors(codeword);
        }

        /// Apply specific error pattern
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

        /// Generate error statistics
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

        /// Test error correction capability
        template <typename CodeType>
        void test_error_correction_capability(size_t max_errors = 10, size_t iterations = 1000)
        {
            CodeType code;
            std::cout << "\nError Correction Capability Test\n";
            std::cout << std::string(50, '=') << "\n";
            std::cout << "Errors\tSuccess Rate\tAvg Corrections\n";
            std::cout << std::string(50, '-') << "\n";

            for (size_t num_errors = 1; num_errors <= max_errors; ++num_errors)
            {
                size_t successes = 0;
                size_t total_corrections = 0;

                for (size_t iter = 0; iter < iterations; ++iter)
                {
                    // Generate random data
                    typename CodeType::DataWord data;
                    std::uniform_int_distribution<int> bit_dist(0, 1);
                    for (auto &bit : data)
                    {
                        bit = bit_dist(rng_);
                    }

                    // Encode
                    auto codeword = code.encode(data);

                    // Add errors
                    auto error_pattern = pattern_gen_.generate_random_errors(
                        codeword.size(), num_errors);
                    auto received = apply_error_pattern(codeword, error_pattern);

                    // Decode
                    auto decoded = code.decode(received);

                    // Check success
                    if (std::equal(data.begin(), data.end(), decoded.begin()))
                    {
                        successes++;
                    }

                    // Count corrections made
                    auto original_errors = analyze_errors(codeword, received);
                    auto remaining_errors = analyze_errors(codeword,
                                                           std::vector<uint8_t>(decoded.begin(), decoded.end()));
                    total_corrections += (original_errors.error_bits - remaining_errors.error_bits);
                }

                double success_rate = static_cast<double>(successes) / iterations;
                double avg_corrections = static_cast<double>(total_corrections) / iterations;

                std::cout << num_errors << "\t"
                          << std::fixed << std::setprecision(3) << success_rate << "\t\t"
                          << std::setprecision(1) << avg_corrections << "\n";
            }
        }

        /// Get channel name
        std::string get_channel_name() const
        {
            return channel_ ? channel_->get_name() : "No Channel";
        }
    };

} // namespace ecc
