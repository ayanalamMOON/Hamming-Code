#include "ecc/bch_code.hpp"
#include "ecc/galois_field.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <cassert>
#include <set>
#include <algorithm>
#include <iomanip>
#include <cmath>

namespace ecc
{

    // LDPC Code implementation
    class LDPCCode
    {
    private:
        size_t n, k;
        std::vector<std::vector<size_t>> parity_check_matrix;
        std::vector<std::vector<size_t>> generator_matrix;
        size_t max_iterations;

    public:
        using CodeWord = std::vector<uint8_t>;
        using DataWord = std::vector<uint8_t>;

        static constexpr size_t code_length = 0; // Dynamic
        static constexpr size_t data_length = 0; // Dynamic

        LDPCCode(size_t code_length, size_t data_length, size_t max_iter = 50)
            : n(code_length), k(data_length), max_iterations(max_iter)
        {
            generate_ldpc_matrices();
        }

        /// Encode data using LDPC code
        [[nodiscard]] CodeWord encode(const DataWord &data) const
        {
            if (data.size() != k)
                throw std::invalid_argument("Invalid data length");

            CodeWord codeword(n, 0);

            // Systematic encoding: copy data bits
            for (size_t i = 0; i < k; ++i)
            {
                codeword[i] = data[i];
            }

            // Calculate parity bits using generator matrix
            for (size_t i = k; i < n; ++i)
            {
                uint8_t parity = 0;
                for (size_t j = 0; j < k; ++j)
                {
                    if (generator_matrix[i - k][j])
                        parity ^= data[j];
                }
                codeword[i] = parity;
            }

            return codeword;
        }

        /// Decode using belief propagation
        struct DecodeResult
        {
            DataWord data;
            bool success;
            size_t iterations_used;
        };

        [[nodiscard]] DecodeResult decode(const CodeWord &received) const
        {
            if (received.size() != n)
                throw std::invalid_argument("Invalid codeword length");

            // Initialize log-likelihood ratios
            std::vector<double> llr(n);
            for (size_t i = 0; i < n; ++i)
            {
                llr[i] = received[i] ? -1.0 : 1.0; // Simple hard decision LLR
            }

            // Belief propagation decoding
            std::vector<uint8_t> decoded = belief_propagation(llr);

            // Check if decoding was successful
            bool success = check_parity(decoded);

            DataWord data(k);
            for (size_t i = 0; i < k; ++i)
            {
                data[i] = decoded[i];
            }

            return {data, success, max_iterations};
        }

    private:
        void generate_ldpc_matrices()
        {
            // Simple regular LDPC code construction
            size_t parity_bits = n - k;
            parity_check_matrix.resize(parity_bits, std::vector<size_t>());

            std::mt19937 rng(42); // Fixed seed for reproducibility
            std::uniform_int_distribution<size_t> dist(0, n - 1);

            // Create sparse parity check matrix
            size_t ones_per_row = 3; // Regular LDPC with degree 3
            for (size_t i = 0; i < parity_bits; ++i)
            {
                std::set<size_t> positions;
                while (positions.size() < ones_per_row)
                {
                    positions.insert(dist(rng));
                }
                parity_check_matrix[i] = std::vector<size_t>(positions.begin(), positions.end());
            }

            // Derive generator matrix (simplified)
            generator_matrix.resize(parity_bits, std::vector<size_t>(k, 0));
            for (size_t i = 0; i < parity_bits; ++i)
            {
                for (size_t j = 0; j < k && j < parity_check_matrix[i].size(); ++j)
                {
                    if (parity_check_matrix[i][j] < k)
                        generator_matrix[i][parity_check_matrix[i][j]] = 1;
                }
            }
        }

        std::vector<uint8_t> belief_propagation(const std::vector<double> &channel_llr) const
        {
            std::vector<double> var_to_check(n * (n - k), 0.0);
            std::vector<double> check_to_var(n * (n - k), 0.0);
            std::vector<double> posterior_llr = channel_llr;

            for (size_t iter = 0; iter < max_iterations; ++iter)
            {
                // Variable to check messages
                for (size_t i = 0; i < n - k; ++i)
                {
                    for (size_t pos : parity_check_matrix[i])
                    {
                        if (pos < n)
                        {
                            double sum = channel_llr[pos];
                            for (size_t j = 0; j < n - k; ++j)
                            {
                                if (j != i)
                                {
                                    for (size_t other_pos : parity_check_matrix[j])
                                    {
                                        if (other_pos == pos)
                                            sum += check_to_var[j * n + pos];
                                    }
                                }
                            }
                            var_to_check[i * n + pos] = sum;
                        }
                    }
                }

                // Check to variable messages
                for (size_t i = 0; i < n - k; ++i)
                {
                    for (size_t pos : parity_check_matrix[i])
                    {
                        if (pos < n)
                        {
                            double product = 1.0;
                            for (size_t other_pos : parity_check_matrix[i])
                            {
                                if (other_pos != pos && other_pos < n)
                                {
                                    product *= std::tanh(var_to_check[i * n + other_pos] / 2.0);
                                }
                            }
                            check_to_var[i * n + pos] = 2.0 * std::atanh(std::max(-0.999, std::min(0.999, product)));
                        }
                    }
                }

                // Update posterior LLR
                for (size_t i = 0; i < n; ++i)
                {
                    posterior_llr[i] = channel_llr[i];
                    for (size_t j = 0; j < n - k; ++j)
                    {
                        for (size_t pos : parity_check_matrix[j])
                        {
                            if (pos == i)
                                posterior_llr[i] += check_to_var[j * n + pos];
                        }
                    }
                }
            }

            // Hard decision
            std::vector<uint8_t> decoded(n);
            for (size_t i = 0; i < n; ++i)
            {
                decoded[i] = posterior_llr[i] < 0 ? 1 : 0;
            }

            return decoded;
        }

        bool check_parity(const std::vector<uint8_t> &codeword) const
        {
            for (size_t i = 0; i < n - k; ++i)
            {
                uint8_t parity = 0;
                for (size_t pos : parity_check_matrix[i])
                {
                    if (pos < n)
                        parity ^= codeword[pos];
                }
                if (parity != 0)
                    return false;
            }
            return true;
        }
    };

    // Turbo Code implementation
    class TurboCode
    {
    private:
        size_t k; // Information length
        size_t n; // Codeword length (3*k for rate 1/3)
        std::vector<size_t> interleaver;
        size_t max_iterations;

        // Constituent encoder polynomials (octal)
        static constexpr uint8_t g1 = 0x7; // 111 (1 + D² + D³)
        static constexpr uint8_t g2 = 0x5; // 101 (1 + D³)

    public:
        using CodeWord = std::vector<uint8_t>;
        using DataWord = std::vector<uint8_t>;

        static constexpr size_t code_length = 0; // Dynamic
        static constexpr size_t data_length = 0; // Dynamic

        TurboCode(size_t info_length, size_t max_iter = 8)
            : k(info_length), n(3 * info_length), max_iterations(max_iter)
        {
            generate_interleaver();
        }

        /// Encode data using turbo code
        [[nodiscard]] CodeWord encode(const DataWord &data) const
        {
            if (data.size() != k)
                throw std::invalid_argument("Invalid data length");

            CodeWord codeword(n);

            // Systematic bits
            for (size_t i = 0; i < k; ++i)
            {
                codeword[3 * i] = data[i];
            }

            // First parity sequence
            auto parity1 = rsc_encode(data);
            for (size_t i = 0; i < k; ++i)
            {
                codeword[3 * i + 1] = parity1[i];
            }

            // Interleave data
            DataWord interleaved_data(k);
            for (size_t i = 0; i < k; ++i)
            {
                interleaved_data[i] = data[interleaver[i]];
            }

            // Second parity sequence
            auto parity2 = rsc_encode(interleaved_data);
            for (size_t i = 0; i < k; ++i)
            {
                codeword[3 * i + 2] = parity2[i];
            }

            return codeword;
        }

        /// Decode using iterative turbo decoding
        struct DecodeResult
        {
            DataWord data;
            bool success;
            size_t iterations_used;
        };

        [[nodiscard]] DecodeResult decode(const CodeWord &received) const
        {
            if (received.size() != n)
                throw std::invalid_argument("Invalid codeword length");

            // Extract systematic, parity1, and parity2 bits
            std::vector<double> systematic_llr(k);
            std::vector<double> parity1_llr(k);
            std::vector<double> parity2_llr(k);

            for (size_t i = 0; i < k; ++i)
            {
                systematic_llr[i] = received[3 * i] ? -1.0 : 1.0;
                parity1_llr[i] = received[3 * i + 1] ? -1.0 : 1.0;
                parity2_llr[i] = received[3 * i + 2] ? -1.0 : 1.0;
            }

            // Iterative decoding
            std::vector<double> extrinsic1(k, 0.0);
            std::vector<double> extrinsic2(k, 0.0);

            for (size_t iter = 0; iter < max_iterations; ++iter)
            {
                // Decode first constituent code
                auto decoder1_out = bcjr_decode(systematic_llr, parity1_llr, extrinsic2);

                // Interleave extrinsic information
                std::vector<double> interleaved_extrinsic(k);
                for (size_t i = 0; i < k; ++i)
                {
                    interleaved_extrinsic[i] = decoder1_out[interleaver[i]];
                }

                // Interleave systematic information
                std::vector<double> interleaved_systematic(k);
                for (size_t i = 0; i < k; ++i)
                {
                    interleaved_systematic[i] = systematic_llr[interleaver[i]];
                }

                // Decode second constituent code
                auto decoder2_out = bcjr_decode(interleaved_systematic, parity2_llr, interleaved_extrinsic);

                // Deinterleave extrinsic information
                for (size_t i = 0; i < k; ++i)
                {
                    extrinsic2[interleaver[i]] = decoder2_out[i];
                }
            }

            // Final decision
            DataWord decoded(k);
            for (size_t i = 0; i < k; ++i)
            {
                double total_llr = systematic_llr[i] + extrinsic1[i] + extrinsic2[i];
                decoded[i] = total_llr < 0 ? 1 : 0;
            }

            return {decoded, true, max_iterations};
        }

    private:
        void generate_interleaver()
        {
            interleaver.resize(k);
            std::iota(interleaver.begin(), interleaver.end(), 0);

            // Simple pseudorandom interleaver
            std::mt19937 rng(42);
            std::shuffle(interleaver.begin(), interleaver.end(), rng);
        }

        std::vector<uint8_t> rsc_encode(const DataWord &data) const
        {
            std::vector<uint8_t> parity(k);
            uint8_t state = 0;

            for (size_t i = 0; i < k; ++i)
            {
                uint8_t input = data[i];

                // Calculate parity output
                uint8_t feedback = input ^ ((state >> 2) & 1);
                parity[i] = feedback ^ (state & 1) ^ ((state >> 1) & 1);

                // Update state
                state = (state >> 1) | (feedback << 2);
            }

            return parity;
        }

        std::vector<double> bcjr_decode(const std::vector<double> &systematic,
                                        const std::vector<double> &parity,
                                        const std::vector<double> &a_priori) const
        {
            // Simplified BCJR algorithm
            std::vector<double> extrinsic(k);

            // Forward recursion
            std::vector<std::vector<double>> alpha(k + 1, std::vector<double>(8, -1000.0));
            alpha[0][0] = 0.0;

            for (size_t i = 0; i < k; ++i)
            {
                for (uint8_t state = 0; state < 8; ++state)
                {
                    if (alpha[i][state] > -999.0)
                    {
                        for (uint8_t input = 0; input < 2; ++input)
                        {
                            uint8_t next_state = ((state >> 1) | (input << 2)) & 7;
                            uint8_t output = calculate_output(state, input);

                            double gamma = (input ? -systematic[i] : systematic[i]) +
                                           (input ? -a_priori[i] : a_priori[i]) +
                                           (output ? -parity[i] : parity[i]);

                            alpha[i + 1][next_state] = std::max(alpha[i + 1][next_state],
                                                                alpha[i][state] + gamma);
                        }
                    }
                }
            }

            // Backward recursion and LLR calculation
            std::vector<std::vector<double>> beta(k + 1, std::vector<double>(8, -1000.0));
            beta[k][0] = 0.0;

            for (int i = k - 1; i >= 0; --i)
            {
                for (uint8_t state = 0; state < 8; ++state)
                {
                    for (uint8_t input = 0; input < 2; ++input)
                    {
                        uint8_t next_state = ((state >> 1) | (input << 2)) & 7;
                        uint8_t output = calculate_output(state, input);

                        double gamma = (input ? -systematic[i] : systematic[i]) +
                                       (input ? -a_priori[i] : a_priori[i]) +
                                       (output ? -parity[i] : parity[i]);

                        beta[i][state] = std::max(beta[i][state],
                                                  beta[i + 1][next_state] + gamma);
                    }
                }

                // Calculate extrinsic LLR
                double llr_0 = -1000.0, llr_1 = -1000.0;

                for (uint8_t state = 0; state < 8; ++state)
                {
                    for (uint8_t input = 0; input < 2; ++input)
                    {
                        uint8_t next_state = ((state >> 1) | (input << 2)) & 7;
                        uint8_t output = calculate_output(state, input);

                        double gamma = (output ? -parity[i] : parity[i]);
                        double metric = alpha[i][state] + gamma + beta[i + 1][next_state];

                        if (input == 0)
                            llr_0 = std::max(llr_0, metric);
                        else
                            llr_1 = std::max(llr_1, metric);
                    }
                }

                extrinsic[i] = llr_0 - llr_1 - systematic[i] - a_priori[i];
            }

            return extrinsic;
        }

        uint8_t calculate_output(uint8_t state, uint8_t input) const
        {
            uint8_t feedback = input ^ ((state >> 2) & 1);
            return feedback ^ (state & 1) ^ ((state >> 1) & 1);
        }
    };

}
