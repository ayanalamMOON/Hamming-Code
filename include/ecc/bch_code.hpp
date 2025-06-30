#pragma once

#include "galois_field.hpp"
#include <vector>
#include <array>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <concepts>
#include <span>
#include <memory>

namespace ecc
{

    /// BCH Code implementation
    template <size_t m, size_t t>
    class BCHCode
    {
    public:
        static constexpr size_t field_order = m;
        static constexpr size_t error_capacity = t;
        static constexpr size_t code_length = (1u << m) - 1;
        static constexpr size_t parity_length = m * t;
        static constexpr size_t data_length = code_length - parity_length;
        static constexpr size_t min_distance = 2 * t + 1;

        using Element = typename GaloisField<m>::Element;
        using Field = GaloisField<m>;
        using Polynomial = GFPolynomial<m>;
        using CodeWord = std::bitset<code_length>;
        using DataWord = std::bitset<data_length>;

    private:
        std::unique_ptr<Field> field;
        Polynomial generator_poly;
        std::vector<Element> roots;

    public:
        /// Constructor with primitive polynomial
        explicit BCHCode(Element primitive_poly)
            : field(std::make_unique<Field>(primitive_poly))
        {
            generate_bch_polynomial();
        }

        /// Default constructor for common primitive polynomials
        BCHCode() : BCHCode(get_default_primitive_poly()) {}

        /// Encode data into BCH codeword
        [[nodiscard]] CodeWord encode(const DataWord &data) const
        {
            // Convert data to polynomial
            std::vector<Element> data_coeffs;
            for (size_t i = 0; i < data_length; ++i)
            {
                data_coeffs.push_back(data[i] ? 1 : 0);
            }

            Polynomial data_poly(field.get(), std::move(data_coeffs));

            // Multiply by x^(parity_length) for systematic encoding
            std::vector<Element> shifted_coeffs(data_length + parity_length, 0);
            for (size_t i = 0; i < data_length; ++i)
            {
                shifted_coeffs[i + parity_length] = data_poly.coefficient(i);
            }
            Polynomial shifted_data(field.get(), std::move(shifted_coeffs));

            // Calculate remainder
            auto [quotient, remainder] = shifted_data.divmod(generator_poly);

            // Systematic codeword: [parity | data]
            CodeWord codeword;

            // Set data bits
            for (size_t i = 0; i < data_length; ++i)
            {
                codeword[i + parity_length] = data[i];
            }

            // Set parity bits
            for (size_t i = 0; i < parity_length; ++i)
            {
                codeword[i] = remainder.coefficient(i) != 0;
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

        /// Decode with error correction using Berlekamp-Massey algorithm
        struct DecodeResult
        {
            DataWord data;
            bool success;
            size_t errors_corrected;
            std::vector<size_t> error_positions;
        };

        [[nodiscard]] DecodeResult decode(const CodeWord &received) const
        {
            // Calculate syndrome
            std::vector<Element> syndromes = calculate_syndromes(received);

            // Check if no errors
            bool no_errors = std::all_of(syndromes.begin(), syndromes.end(),
                                         [](Element s)
                                         { return s == 0; });

            if (no_errors)
            {
                DataWord data;
                for (size_t i = 0; i < data_length; ++i)
                {
                    data[i] = received[i + parity_length];
                }
                return {data, true, 0, {}};
            }

            // Find error locator polynomial using Berlekamp-Massey
            auto error_locator = berlekamp_massey(syndromes);

            // Find error positions using Chien search
            auto error_positions = chien_search(error_locator);

            // Verify we can correct the errors
            if (error_positions.size() > t)
            {
                DataWord data;
                for (size_t i = 0; i < data_length; ++i)
                {
                    data[i] = received[i + parity_length];
                }
                return {data, false, 0, {}};
            }

            // Correct errors
            CodeWord corrected = received;
            for (size_t pos : error_positions)
            {
                corrected.flip(pos);
            }

            // Extract data
            DataWord data;
            for (size_t i = 0; i < data_length; ++i)
            {
                data[i] = corrected[i + parity_length];
            }

            return {data, true, error_positions.size(), error_positions};
        }

        /// Get generator polynomial
        [[nodiscard]] const Polynomial &get_generator_polynomial() const noexcept
        {
            return generator_poly;
        }

        /// Get minimum distance
        [[nodiscard]] constexpr size_t get_min_distance() const noexcept
        {
            return min_distance;
        }

        /// Get error correction capability
        [[nodiscard]] constexpr size_t get_error_capacity() const noexcept
        {
            return error_capacity;
        }

    private:
        /// Generate BCH generator polynomial
        void generate_bch_polynomial()
        {
            // Find consecutive roots: α, α², ..., α^(2t)
            Element alpha = field->get_primitive();
            roots.clear();

            for (size_t i = 1; i <= 2 * t; ++i)
            {
                roots.push_back(field->power(alpha, i));
            }

            // Remove duplicate roots (since we're in GF(2^m))
            std::sort(roots.begin(), roots.end());
            roots.erase(std::unique(roots.begin(), roots.end()), roots.end());

            // Build generator polynomial as product of minimal polynomials
            generator_poly = Polynomial(field.get(), {1}); // Start with 1

            for (Element root : roots)
            {
                // Minimal polynomial (x - root)
                std::vector<Element> min_poly_coeffs = {root, 1};
                Polynomial min_poly(field.get(), std::move(min_poly_coeffs));
                generator_poly = generator_poly * min_poly;
            }
        }

        /// Calculate syndromes
        [[nodiscard]] std::vector<Element> calculate_syndromes(const CodeWord &received) const
        {
            std::vector<Element> syndromes(2 * t);

            // Convert received word to polynomial
            std::vector<Element> coeffs;
            for (size_t i = 0; i < code_length; ++i)
            {
                coeffs.push_back(received[i] ? 1 : 0);
            }
            Polynomial received_poly(field.get(), std::move(coeffs));

            // Evaluate at roots
            Element alpha = field->get_primitive();
            for (size_t i = 0; i < 2 * t; ++i)
            {
                Element root = field->power(alpha, i + 1);
                syndromes[i] = received_poly.evaluate(root);
            }

            return syndromes;
        }

        /// Berlekamp-Massey algorithm for finding error locator polynomial
        [[nodiscard]] Polynomial berlekamp_massey(const std::vector<Element> &syndromes) const
        {
            Polynomial C(field.get(), {1}); // Error locator polynomial
            Polynomial B(field.get(), {1}); // Previous error locator polynomial
            int L = 0;                      // Current length
            int pos = 1;                    // Current position
            Element b = 1;                  // Previous discrepancy

            for (size_t n = 0; n < syndromes.size(); ++n)
            {
                // Calculate discrepancy
                Element d = syndromes[n];
                for (int i = 1; i <= L; ++i)
                {
                    d = field->add(d, field->multiply(C.coefficient(i), syndromes[n - i]));
                }

                if (d == 0)
                {
                    pos++;
                }
                else
                {
                    // Update C
                    Polynomial T = C;

                    // C(x) = C(x) - (d/b) * x^pos * B(x)
                    Element coeff = field->divide(d, b);
                    std::vector<Element> term_coeffs(pos + B.degree() + 1, 0);
                    for (int i = 0; i <= B.degree(); ++i)
                    {
                        term_coeffs[i + pos] = field->multiply(coeff, B.coefficient(i));
                    }
                    Polynomial correction(field.get(), std::move(term_coeffs));
                    C = C + correction;

                    if (2 * L <= static_cast<int>(n))
                    {
                        L = static_cast<int>(n) + 1 - L;
                        B = T;
                        b = d;
                        pos = 1;
                    }
                    else
                    {
                        pos++;
                    }
                }
            }

            return C;
        }

        /// Chien search for finding roots of error locator polynomial
        [[nodiscard]] std::vector<size_t> chien_search(const Polynomial &error_locator) const
        {
            std::vector<size_t> error_positions;

            Element alpha = field->get_primitive();
            for (size_t i = 0; i < code_length; ++i)
            {
                Element alpha_inv = field->power(alpha, code_length - 1 - i);
                if (error_locator.evaluate(alpha_inv) == 0)
                {
                    error_positions.push_back(i);
                }
            }

            return error_positions;
        }

        /// Get default primitive polynomial for common field orders
        [[nodiscard]] static constexpr Element get_default_primitive_poly() noexcept
        {
            if constexpr (m == 3)
                return 0x0B; // x³ + x + 1
            else if constexpr (m == 4)
                return 0x13; // x⁴ + x + 1
            else if constexpr (m == 5)
                return 0x25; // x⁵ + x² + 1
            else if constexpr (m == 6)
                return 0x43; // x⁶ + x + 1
            else if constexpr (m == 7)
                return 0x89; // x⁷ + x³ + 1
            else if constexpr (m == 8)
                return 0x11D; // x⁸ + x⁴ + x³ + x² + 1
            else
                static_assert(m <= 8, "Default primitive polynomial not available for this field order");
        }
    };

    // Common BCH code configurations
    using BCH_15_7_3 = BCHCode<4, 1>;    // (15,7) BCH code, t=1
    using BCH_15_5_3 = BCHCode<4, 2>;    // (15,5) BCH code, t=2
    using BCH_31_21_3 = BCHCode<5, 1>;   // (31,21) BCH code, t=1
    using BCH_31_16_3 = BCHCode<5, 2>;   // (31,16) BCH code, t=2
    using BCH_63_51_3 = BCHCode<6, 1>;   // (63,51) BCH code, t=1
    using BCH_127_120_3 = BCHCode<7, 1>; // (127,120) BCH code, t=1

} // namespace ecc
