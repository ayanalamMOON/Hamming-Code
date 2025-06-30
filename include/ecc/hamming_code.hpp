#pragma once

#include <array>
#include <vector>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <concepts>
#include <span>

namespace ecc
{

    /// Concept for valid Hamming code parameters
    template <size_t n, size_t k>
    concept ValidHammingParams = (n > k) && (n == (1u << (n - k)) - 1) && (k == n - (n - k));

    /// Template class for Hamming codes with compile-time parameters
    template <size_t n, size_t k>
        requires ValidHammingParams<n, k>
    class HammingCode
    {
    public:
        static constexpr size_t code_length = n;
        static constexpr size_t data_length = k;
        static constexpr size_t parity_length = n - k;
        static constexpr size_t min_distance = 3;

        using CodeWord = std::bitset<n>;
        using DataWord = std::bitset<k>;
        using Syndrome = std::bitset<parity_length>;

    private:
        // Generator matrix G (k x n)
        std::array<std::bitset<n>, k> generator_matrix;

        // Parity check matrix H ((n-k) x n)
        std::array<std::bitset<n>, parity_length> parity_check_matrix;

        // Syndrome lookup table for error correction
        std::array<size_t, (1u << parity_length)> syndrome_table;

    public:
        /// Constructor - generates the matrices
        HammingCode()
        {
            generate_matrices();
            build_syndrome_table();
        }

        /// Encode a data word into a codeword
        [[nodiscard]] CodeWord encode(const DataWord &data) const noexcept
        {
            CodeWord codeword;

            // Systematic encoding: codeword = [data | parity]
            for (size_t i = 0; i < k; ++i)
            {
                codeword[i] = data[i];
            }

            // Calculate parity bits
            for (size_t i = 0; i < parity_length; ++i)
            {
                bool parity = false;
                for (size_t j = 0; j < k; ++j)
                {
                    parity ^= (data[j] && generator_matrix[j][k + i]);
                }
                codeword[k + i] = parity;
            }

            return codeword;
        }

        /// Encode vector of data words
        [[nodiscard]] std::vector<CodeWord> encode(std::span<const DataWord> data) const
        {
            std::vector<CodeWord> result;
            result.reserve(data.size());

            for (const auto &word : data)
            {
                result.push_back(encode(word));
            }

            return result;
        }

        /// Decode a received codeword with error correction
        [[nodiscard]] DataWord decode(const CodeWord &received) const
        {
            auto syndrome = calculate_syndrome(received);
            auto corrected = correct_errors(received, syndrome);

            // Extract data bits (systematic code)
            DataWord data;
            for (size_t i = 0; i < k; ++i)
            {
                data[i] = corrected[i];
            }

            return data;
        }

        /// Decode with error detection (returns error position if detected)
        struct DecodeResult
        {
            DataWord data;
            bool error_detected;
            size_t error_position; // 0-based, n if no error
        };

        [[nodiscard]] DecodeResult decode_with_detection(const CodeWord &received) const
        {
            auto syndrome = calculate_syndrome(received);
            size_t error_pos = syndrome_table[syndrome.to_ulong()];

            DecodeResult result;
            result.error_detected = (error_pos < n);
            result.error_position = error_pos;

            if (result.error_detected)
            {
                auto corrected = received;
                corrected.flip(error_pos);

                for (size_t i = 0; i < k; ++i)
                {
                    result.data[i] = corrected[i];
                }
            }
            else
            {
                for (size_t i = 0; i < k; ++i)
                {
                    result.data[i] = received[i];
                }
            }

            return result;
        }

        /// Calculate syndrome for received codeword
        [[nodiscard]] Syndrome calculate_syndrome(const CodeWord &received) const noexcept
        {
            Syndrome syndrome;

            for (size_t i = 0; i < parity_length; ++i)
            {
                bool bit = false;
                for (size_t j = 0; j < n; ++j)
                {
                    bit ^= (received[j] && parity_check_matrix[i][j]);
                }
                syndrome[i] = bit;
            }

            return syndrome;
        }

        /// Get minimum distance of the code
        [[nodiscard]] constexpr size_t get_min_distance() const noexcept
        {
            return min_distance;
        }

        /// Get error correction capability
        [[nodiscard]] constexpr size_t get_error_correction_capability() const noexcept
        {
            return (min_distance - 1) / 2;
        }

        /// Get error detection capability
        [[nodiscard]] constexpr size_t get_error_detection_capability() const noexcept
        {
            return min_distance - 1;
        }

        /// Get code rate
        [[nodiscard]] constexpr double get_code_rate() const noexcept
        {
            return static_cast<double>(k) / n;
        }

    private:
        void generate_matrices()
        {
            // Generate systematic generator matrix G = [I_k | P]
            for (size_t i = 0; i < k; ++i)
            {
                // Identity part
                generator_matrix[i][i] = 1;

                // Parity part - use position-based encoding
                for (size_t j = 0; j < parity_length; ++j)
                {
                    size_t col = k + j;
                    size_t power = 1u << j;
                    generator_matrix[i][col] = ((i + 1) & power) != 0;
                }
            }

            // Generate parity check matrix H = [P^T | I_(n-k)]
            for (size_t i = 0; i < parity_length; ++i)
            {
                // Parity part (transpose of P)
                for (size_t j = 0; j < k; ++j)
                {
                    parity_check_matrix[i][j] = generator_matrix[j][k + i];
                }

                // Identity part
                parity_check_matrix[i][k + i] = 1;
            }
        }

        void build_syndrome_table()
        {
            // Initialize all entries to "no error"
            std::fill(syndrome_table.begin(), syndrome_table.end(), n);

            // Single-bit error patterns
            for (size_t pos = 0; pos < n; ++pos)
            {
                CodeWord error_pattern;
                error_pattern[pos] = 1;

                auto syndrome = calculate_syndrome(error_pattern);
                syndrome_table[syndrome.to_ulong()] = pos;
            }
        }

        [[nodiscard]] CodeWord correct_errors(const CodeWord &received, const Syndrome &syndrome) const
        {
            size_t error_pos = syndrome_table[syndrome.to_ulong()];

            if (error_pos < n)
            {
                auto corrected = received;
                corrected.flip(error_pos);
                return corrected;
            }

            return received; // No correctable error detected
        }
    };

    /// SECDED (Single Error Correction, Double Error Detection) Hamming Code
    template <size_t n, size_t k>
        requires ValidHammingParams<n, k>
    class SECDEDHammingCode : public HammingCode<n + 1, k>
    {
    public:
        using Base = HammingCode<n + 1, k>;
        using typename Base::CodeWord;
        using typename Base::DataWord;

        struct SECDEDResult
        {
            DataWord data;
            enum class Status
            {
                NO_ERROR,
                SINGLE_ERROR_CORRECTED,
                DOUBLE_ERROR_DETECTED,
                UNCORRECTABLE_ERROR
            } status;
            size_t error_position;
        };

        /// Decode with SECDED capability
        [[nodiscard]] SECDEDResult decode_secded(const CodeWord &received) const
        {
            // Calculate syndrome and overall parity
            auto syndrome = this->calculate_syndrome(received);
            bool overall_parity = received.count() % 2;

            SECDEDResult result;

            if (syndrome.none())
            {
                if (!overall_parity)
                {
                    result.status = SECDEDResult::Status::NO_ERROR;
                }
                else
                {
                    result.status = SECDEDResult::Status::SINGLE_ERROR_CORRECTED;
                    result.error_position = n; // Error in overall parity bit
                }
            }
            else
            {
                if (overall_parity)
                {
                    // Single error - correct it
                    result.status = SECDEDResult::Status::SINGLE_ERROR_CORRECTED;
                    auto decode_result = this->decode_with_detection(received);
                    result.data = decode_result.data;
                    result.error_position = decode_result.error_position;
                }
                else
                {
                    // Double error detected
                    result.status = SECDEDResult::Status::DOUBLE_ERROR_DETECTED;
                }
            }

            return result;
        }
    };

    /// Convenience type aliases for common Hamming codes
    using Hamming_7_4 = HammingCode<7, 4>;
    using Hamming_15_11 = HammingCode<15, 11>;
    using Hamming_31_26 = HammingCode<31, 26>;
    using Hamming_63_57 = HammingCode<63, 57>;

    using SECDED_8_4 = SECDEDHammingCode<7, 4>;
    using SECDED_16_11 = SECDEDHammingCode<15, 11>;
    using SECDED_32_26 = SECDEDHammingCode<31, 26>;
    using SECDED_64_57 = SECDEDHammingCode<63, 57>;

} // namespace ecc
