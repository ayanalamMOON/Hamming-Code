#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <set>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace ecc
{

    /// Galois Field GF(2^m) implementation with optimized arithmetic
    template <size_t m>
    class GaloisField
    {
    public:
        static constexpr size_t field_size = 1u << m;
        static constexpr size_t primitive_element = 2;

        using Element = uint32_t;

    private:
        std::array<Element, field_size> exp_table;
        std::array<Element, field_size> log_table;
        Element primitive_poly;

    public:
        explicit GaloisField(Element prim_poly) : primitive_poly(prim_poly)
        {
            build_tables();
        }

        /// Addition in GF(2^m) (XOR)
        [[nodiscard]] constexpr Element add(Element a, Element b) const noexcept
        {
            return a ^ b;
        }

        /// Multiplication in GF(2^m) using lookup tables
        [[nodiscard]] Element multiply(Element a, Element b) const noexcept
        {
            if (a == 0 || b == 0)
                return 0;

            size_t log_sum = (log_table[a] + log_table[b]) % (field_size - 1);
            return exp_table[log_sum];
        }

        /// Division in GF(2^m)
        [[nodiscard]] Element divide(Element a, Element b) const
        {
            if (b == 0)
                throw std::invalid_argument("Division by zero in Galois field");
            if (a == 0)
                return 0;

            size_t log_diff = (log_table[a] - log_table[b] + field_size - 1) % (field_size - 1);
            return exp_table[log_diff];
        }

        /// Power operation
        [[nodiscard]] Element power(Element base, size_t exponent) const noexcept
        {
            if (base == 0)
                return (exponent == 0) ? 1 : 0;

            size_t log_result = (log_table[base] * exponent) % (field_size - 1);
            return exp_table[log_result];
        }

        /// Multiplicative inverse
        [[nodiscard]] Element inverse(Element a) const
        {
            if (a == 0)
                throw std::invalid_argument("Zero has no multiplicative inverse");

            size_t inv_log = (field_size - 1 - log_table[a]) % (field_size - 1);
            return exp_table[inv_log];
        }

        /// Get primitive element
        [[nodiscard]] constexpr Element get_primitive() const noexcept
        {
            return primitive_element;
        }

        /// Check if element is primitive
        [[nodiscard]] bool is_primitive(Element alpha) const noexcept
        {
            if (alpha <= 1)
                return false;

            Element current = alpha;
            for (size_t i = 1; i < field_size - 1; ++i)
            {
                if (current == 1)
                    return false;
                current = multiply(current, alpha);
            }
            return current == 1;
        }

    private:
        void build_tables()
        {
            // Build exponential table
            exp_table[0] = 1;
            Element value = 1;

            for (size_t i = 1; i < field_size; ++i)
            {
                value <<= 1;
                if (value & field_size)
                {
                    value ^= primitive_poly;
                }
                exp_table[i % (field_size - 1)] = value;
            }

            // Build logarithm table
            log_table[0] = 0; // Undefined, but we'll use 0
            for (size_t i = 0; i < field_size - 1; ++i)
            {
                log_table[exp_table[i]] = i;
            }
        }
    };

    /// Polynomial operations in GF(2^m)
    template <size_t m>
    class GFPolynomial
    {
    public:
        using Field = GaloisField<m>;
        using Element = typename Field::Element;

    private:
        std::vector<Element> coefficients;
        const Field *field;

    public:
        GFPolynomial(const Field &gf, std::vector<Element> coeffs = {0})
            : coefficients(std::move(coeffs)), field(&gf)
        {
            normalize();
        }

        /// Degree of polynomial
        [[nodiscard]] size_t degree() const noexcept
        {
            return coefficients.empty() ? 0 : coefficients.size() - 1;
        }

        /// Access coefficient
        [[nodiscard]] Element operator[](size_t index) const noexcept
        {
            return (index < coefficients.size()) ? coefficients[index] : 0;
        }

        /// Set coefficient
        void set_coefficient(size_t index, Element value)
        {
            if (index >= coefficients.size())
            {
                coefficients.resize(index + 1, 0);
            }
            coefficients[index] = value;
            normalize();
        }

        /// Polynomial addition
        [[nodiscard]] GFPolynomial operator+(const GFPolynomial &other) const
        {
            size_t max_size = std::max(coefficients.size(), other.coefficients.size());
            std::vector<Element> result(max_size, 0);

            for (size_t i = 0; i < max_size; ++i)
            {
                Element a = (i < coefficients.size()) ? coefficients[i] : 0;
                Element b = (i < other.coefficients.size()) ? other.coefficients[i] : 0;
                result[i] = field->add(a, b);
            }

            return GFPolynomial(*field, std::move(result));
        }

        /// Polynomial multiplication
        [[nodiscard]] GFPolynomial operator*(const GFPolynomial &other) const
        {
            if (is_zero() || other.is_zero())
            {
                return GFPolynomial(*field, {0});
            }

            size_t result_size = coefficients.size() + other.coefficients.size() - 1;
            std::vector<Element> result(result_size, 0);

            for (size_t i = 0; i < coefficients.size(); ++i)
            {
                for (size_t j = 0; j < other.coefficients.size(); ++j)
                {
                    Element product = field->multiply(coefficients[i], other.coefficients[j]);
                    result[i + j] = field->add(result[i + j], product);
                }
            }

            return GFPolynomial(*field, std::move(result));
        }

        /// Polynomial evaluation at point x
        [[nodiscard]] Element evaluate(Element x) const noexcept
        {
            if (coefficients.empty())
                return 0;

            Element result = coefficients.back();
            for (int i = static_cast<int>(coefficients.size()) - 2; i >= 0; --i)
            {
                result = field->add(field->multiply(result, x), coefficients[i]);
            }

            return result;
        }

        /// Check if polynomial is zero
        [[nodiscard]] bool is_zero() const noexcept
        {
            return std::all_of(coefficients.begin(), coefficients.end(),
                               [](Element e)
                               { return e == 0; });
        }

        /// Get roots of polynomial
        [[nodiscard]] std::vector<Element> find_roots() const
        {
            std::vector<Element> roots;

            for (Element x = 0; x < Field::field_size; ++x)
            {
                if (evaluate(x) == 0)
                {
                    roots.push_back(x);
                }
            }

            return roots;
        }

    private:
        void normalize()
        {
            while (!coefficients.empty() && coefficients.back() == 0)
            {
                coefficients.pop_back();
            }
            if (coefficients.empty())
            {
                coefficients.push_back(0);
            }
        }
    };

    /// Specialized Galois fields for common applications
    using GF256 = GaloisField<8>;   // For Reed-Solomon codes
    using GF1024 = GaloisField<10>; // For extended Reed-Solomon
    using GF4096 = GaloisField<12>; // For high-rate applications

    /// Primitive polynomials for common field sizes
    constexpr uint32_t primitive_poly_8 = 0x11D;   // x^8 + x^4 + x^3 + x^2 + 1
    constexpr uint32_t primitive_poly_10 = 0x409;  // x^10 + x^3 + 1
    constexpr uint32_t primitive_poly_12 = 0x1053; // x^12 + x^6 + x^4 + x + 1

    /// Factory functions for creating common Galois fields
    namespace galois
    {
        /// Create GF(2^8) field for Reed-Solomon codes
        std::unique_ptr<GF256> create_gf256();

        /// Create GF(2^10) field for extended applications
        std::unique_ptr<GF1024> create_gf1024();

        /// Create GF(2^12) field for high-rate applications
        std::unique_ptr<GF4096> create_gf4096();
    }

    /// Utility functions for Galois field operations
    namespace galois_utils
    {
        /// Check if a polynomial is primitive over GF(2)
        template <size_t m>
        bool is_primitive_polynomial(typename GaloisField<m>::Element poly);

        /// Find all primitive polynomials of degree m
        template <size_t m>
        std::vector<typename GaloisField<m>::Element> find_primitive_polynomials();

        /// Get default primitive polynomial for common field sizes
        template <size_t m>
        constexpr typename GaloisField<m>::Element get_default_primitive()
        {
            if constexpr (m == 3)
                return 0x0B; // x^3 + x + 1
            else if constexpr (m == 4)
                return 0x13; // x^4 + x + 1
            else if constexpr (m == 5)
                return 0x25; // x^5 + x^2 + 1
            else if constexpr (m == 6)
                return 0x43; // x^6 + x + 1
            else if constexpr (m == 7)
                return 0x89; // x^7 + x^3 + 1
            else if constexpr (m == 8)
                return primitive_poly_8;
            else if constexpr (m == 10)
                return primitive_poly_10;
            else if constexpr (m == 12)
                return primitive_poly_12;
            else
                return (1u << m) | 3; // Default: x^m + x + 1
        }

        /// Convert polynomial to string representation
        template <size_t m>
        std::string polynomial_to_string(typename GaloisField<m>::Element poly);

        /// Compute minimal polynomial of an element
        template <size_t m>
        std::vector<typename GaloisField<m>::Element> minimal_polynomial(
            const GaloisField<m> &field, typename GaloisField<m>::Element alpha);
    }

    /// Galois field arithmetic benchmarks
    namespace galois_benchmark
    {
        /// Benchmark results structure
        template <size_t m>
        struct BenchmarkResults
        {
            double add_time_ns;
            double mul_time_ns;
            double div_time_ns;
            double inv_time_ns;
            double exp_time_ns;
            size_t iterations;
        };

        /// Benchmark basic field operations
        template <size_t m>
        BenchmarkResults<m> benchmark_field_operations(size_t iterations = 1000000);

        /// Print benchmark results
        template <size_t m>
        void print_benchmark_results(const BenchmarkResults<m> &results);
    }

    /// Galois field testing utilities
    namespace galois_test
    {
        /// Verify field axioms for a given field
        template <size_t m>
        bool verify_field_axioms(const GaloisField<m> &field, size_t test_count = 1000);

        /// Test polynomial operations
        template <size_t m>
        bool test_polynomial_operations(size_t test_count = 100);

        /// Comprehensive field test suite
        template <size_t m>
        bool run_comprehensive_tests();
    }

    /// Demo functions for showcasing Galois field capabilities
    namespace galois_demo
    {
        /// Demonstrate basic field operations
        template <size_t m>
        void demonstrate_basic_operations();

        /// Demonstrate polynomial operations
        template <size_t m>
        void demonstrate_polynomial_operations();

        /// Run complete demo
        void run_complete_demo();
    }
} // namespace ecc
