#pragma once

#include "galois_field.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <span>

namespace ecc
{

    /// Reed-Solomon code implementation with configurable parameters
    template <size_t n, size_t k, size_t m = 8>
        requires(n <= (1u << m) - 1) && (k <= n) && (n - k <= n)
    class ReedSolomonCode
    {
    public:
        static constexpr size_t code_length = n;
        static constexpr size_t data_length = k;
        static constexpr size_t parity_length = n - k;
        static constexpr size_t symbol_size = m;
        static constexpr size_t min_distance = parity_length + 1;
        static constexpr size_t error_correction_capability = parity_length / 2;

        using Symbol = uint32_t;
        using CodeWord = std::array<Symbol, n>;
        using DataWord = std::array<Symbol, k>;
        using Field = GaloisField<m>;
        using Polynomial = GFPolynomial<m>;

    private:
        std::unique_ptr<Field> field;
        Polynomial generator_poly;
        Symbol primitive_element;

    public:
        /// Constructor with default primitive polynomial
        ReedSolomonCode() : ReedSolomonCode(get_default_primitive_poly()) {}

        /// Constructor with custom primitive polynomial
        explicit ReedSolomonCode(Symbol primitive_polynomial)
            : field(std::make_unique<Field>(primitive_polynomial)),
              generator_poly(*field),
              primitive_element(field->get_primitive())
        {

            generate_polynomial();
        }

        /// Encode data symbols into codeword
        [[nodiscard]] CodeWord encode(const DataWord &data) const
        {
            CodeWord codeword{};

            // Copy data to systematic positions
            std::copy(data.begin(), data.end(), codeword.begin());

            // Calculate parity symbols
            auto data_poly = Polynomial(*field,
                                        std::vector<Symbol>(data.begin(), data.end()));

            // Multiply by x^(n-k) to shift data
            std::vector<Symbol> shifted_coeffs(parity_length, 0);
            shifted_coeffs.insert(shifted_coeffs.end(), data.begin(), data.end());
            auto shifted_poly = Polynomial(*field, std::move(shifted_coeffs));

            // Calculate remainder
            auto remainder = polynomial_mod(shifted_poly, generator_poly);

            // Add parity symbols
            for (size_t i = 0; i < parity_length; ++i)
            {
                codeword[k + i] = remainder[i];
            }

            return codeword;
        }

        /// Encode vector of data symbols
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

        /// Decode received codeword with error correction
        struct DecodeResult
        {
            DataWord data;
            bool success;
            size_t errors_corrected;
            std::vector<size_t> error_positions;
        };

        [[nodiscard]] DecodeResult decode(const CodeWord &received) const
        {
            DecodeResult result{};

            // Calculate syndrome
            auto syndromes = calculate_syndromes(received);

            // Check if there are errors
            bool has_errors = std::any_of(syndromes.begin(), syndromes.end(),
                                          [](Symbol s)
                                          { return s != 0; });

            if (!has_errors)
            {
                // No errors detected
                std::copy(received.begin(), received.begin() + k, result.data.begin());
                result.success = true;
                result.errors_corrected = 0;
                return result;
            }

            // Find error locator polynomial using Berlekamp-Massey algorithm
            auto error_locator = berlekamp_massey(syndromes);

            // Find error positions using Chien search
            auto error_positions = chien_search(error_locator);

            if (error_positions.size() > error_correction_capability)
            {
                // Too many errors to correct
                result.success = false;
                return result;
            }

            // Calculate error values using Forney algorithm
            auto error_values = forney_algorithm(syndromes, error_locator, error_positions);

            // Correct errors
            auto corrected = received;
            for (size_t i = 0; i < error_positions.size(); ++i)
            {
                corrected[error_positions[i]] = field->add(
                    corrected[error_positions[i]], error_values[i]);
            }

            // Extract data
            std::copy(corrected.begin(), corrected.begin() + k, result.data.begin());
            result.success = true;
            result.errors_corrected = error_positions.size();
            result.error_positions = std::move(error_positions);

            return result;
        }

        /// Calculate syndrome vector
        [[nodiscard]] std::vector<Symbol> calculate_syndromes(const CodeWord &received) const
        {
            std::vector<Symbol> syndromes(parity_length);

            for (size_t i = 0; i < parity_length; ++i)
            {
                Symbol syndrome = 0;
                Symbol alpha_power = field->power(primitive_element, i + 1);
                Symbol current_power = 1;

                for (size_t j = 0; j < n; ++j)
                {
                    syndrome = field->add(syndrome,
                                          field->multiply(received[j], current_power));
                    current_power = field->multiply(current_power, alpha_power);
                }

                syndromes[i] = syndrome;
            }

            return syndromes;
        }

        /// Get minimum distance
        [[nodiscard]] constexpr size_t get_min_distance() const noexcept
        {
            return min_distance;
        }

        /// Get error correction capability
        [[nodiscard]] constexpr size_t get_error_correction_capability() const noexcept
        {
            return error_correction_capability;
        }

        /// Get code rate
        [[nodiscard]] constexpr double get_code_rate() const noexcept
        {
            return static_cast<double>(k) / n;
        }

    private:
        void generate_polynomial()
        {
            // Generator polynomial g(x) = (x - α)(x - α²)...(x - α^(n-k))
            std::vector<Symbol> coeffs = {1};
            auto gen_poly = Polynomial(*field, coeffs);

            for (size_t i = 1; i <= parity_length; ++i)
            {
                Symbol root = field->power(primitive_element, i);

                // Multiply by (x - root)
                std::vector<Symbol> factor_coeffs = {root, 1};
                auto factor = Polynomial(*field, factor_coeffs);
                gen_poly = gen_poly * factor;
            }

            generator_poly = std::move(gen_poly);
        }

        [[nodiscard]] Polynomial polynomial_mod(const Polynomial &dividend,
                                                const Polynomial &divisor) const
        {
            auto quotient = dividend;
            size_t divisor_degree = divisor.degree();

            while (quotient.degree() >= divisor_degree && !quotient.is_zero())
            {
                Symbol lead_coeff = quotient[quotient.degree()];
                Symbol divisor_lead = divisor[divisor_degree];
                Symbol factor = field->divide(lead_coeff, divisor_lead);

                size_t shift = quotient.degree() - divisor_degree;

                // Subtract factor * x^shift * divisor
                for (size_t i = 0; i <= divisor_degree; ++i)
                {
                    Symbol term = field->multiply(factor, divisor[i]);
                    Symbol current = quotient[i + shift];
                    quotient.set_coefficient(i + shift, field->add(current, term));
                }
            }

            return quotient;
        }

        [[nodiscard]] Polynomial berlekamp_massey(const std::vector<Symbol> &syndromes) const
        {
            size_t L = 0;   // Current length
            size_t pos = 1; // Position of last length change

            std::vector<Symbol> C(parity_length + 1, 0); // Connection polynomial
            std::vector<Symbol> B(parity_length + 1, 0); // Previous connection polynomial
            std::vector<Symbol> T(parity_length + 1, 0); // Temporary

            C[0] = 1;
            B[0] = 1;

            for (size_t i = 0; i < parity_length; ++i)
            {
                // Calculate discrepancy
                Symbol d = syndromes[i];
                for (size_t j = 1; j <= L; ++j)
                {
                    d = field->add(d, field->multiply(C[j], syndromes[i - j]));
                }

                if (d == 0)
                {
                    pos++;
                }
                else
                {
                    std::copy(C.begin(), C.end(), T.begin());

                    Symbol d_inv = field->inverse(d);
                    for (size_t idx = 0; idx < parity_length + 1 - pos && idx + pos < parity_length + 1; ++idx)
                    {
                        C[idx + pos] = field->add(C[idx + pos],
                                                  field->multiply(d_inv, B[idx]));
                    }

                    if (2 * L <= i)
                    {
                        L = i + 1 - L;
                        std::copy(T.begin(), T.end(), B.begin());
                        pos = 1;
                    }
                    else
                    {
                        pos++;
                    }
                }
            }

            return Polynomial(*field, std::vector<Symbol>(C.begin(), C.begin() + L + 1));
        }

        [[nodiscard]] std::vector<size_t> chien_search(const Polynomial &error_locator) const
        {
            std::vector<size_t> positions;

            for (size_t i = 0; i < n; ++i)
            {
                Symbol alpha_inv = field->power(primitive_element, n - 1 - i);
                if (error_locator.evaluate(alpha_inv) == 0)
                {
                    positions.push_back(i);
                }
            }

            return positions;
        }

        [[nodiscard]] std::vector<Symbol> forney_algorithm(
            const std::vector<Symbol> &syndromes,
            const Polynomial &error_locator,
            const std::vector<size_t> &error_positions) const
        {

            std::vector<Symbol> error_values(error_positions.size());

            // Calculate error evaluator polynomial
            std::vector<Symbol> syndrome_coeffs(syndromes.rbegin(), syndromes.rend());
            auto syndrome_poly = Polynomial(*field, syndrome_coeffs);
            auto error_evaluator = syndrome_poly * error_locator;

            // Calculate derivative of error locator
            std::vector<Symbol> derivative_coeffs;
            for (size_t i = 1; i <= error_locator.degree(); i += 2)
            {
                if (i < error_locator.degree() + 1)
                {
                    derivative_coeffs.push_back(error_locator[i]);
                }
            }
            auto error_locator_derivative = Polynomial(*field, derivative_coeffs);

            // Calculate error values
            for (size_t i = 0; i < error_positions.size(); ++i)
            {
                Symbol alpha_inv = field->power(primitive_element, n - 1 - error_positions[i]);
                Symbol numerator = error_evaluator.evaluate(alpha_inv);
                Symbol denominator = error_locator_derivative.evaluate(alpha_inv);

                if (denominator != 0)
                {
                    error_values[i] = field->divide(numerator, denominator);
                }
            }

            return error_values;
        }

        [[nodiscard]] static constexpr Symbol get_default_primitive_poly() noexcept
        {
            if constexpr (m == 8)
                return primitive_poly_8;
            else if constexpr (m == 10)
                return primitive_poly_10;
            else if constexpr (m == 12)
                return primitive_poly_12;
            else
                return 0x3; // x + 1 for small fields
        }
    };

    /// Convenience type aliases for common Reed-Solomon codes
    using RS_255_223 = ReedSolomonCode<255, 223, 8>;      // Standard RS code
    using RS_255_239 = ReedSolomonCode<255, 239, 8>;      // High-rate RS code
    using RS_255_191 = ReedSolomonCode<255, 191, 8>;      // High-redundancy RS code
    using RS_1023_1007 = ReedSolomonCode<1023, 1007, 10>; // Extended RS code

} // namespace ecc
