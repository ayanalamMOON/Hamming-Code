#include "ecc/galois_field.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <map>
#include <set>
#include <cassert>

namespace ecc
{

    /// Factory functions for creating common Galois fields
    namespace galois
    {

        /// Create GF(2^8) field for Reed-Solomon codes
        std::unique_ptr<GF256> create_gf256()
        {
            return std::make_unique<GF256>(primitive_poly_8);
        }

        /// Create GF(2^10) field for extended applications
        std::unique_ptr<GF1024> create_gf1024()
        {
            return std::make_unique<GF1024>(primitive_poly_10);
        }

        /// Create GF(2^12) field for high-rate applications
        std::unique_ptr<GF4096> create_gf4096()
        {
            return std::make_unique<GF4096>(primitive_poly_12);
        }

    } // namespace galois

    /// Utility functions for Galois field operations
    namespace galois_utils
    {

        /// Check if a polynomial is primitive over GF(2)
        template <size_t m>
        bool is_primitive_polynomial(typename GaloisField<m>::Element poly)
        {
            try
            {
                GaloisField<m> field(poly);
                return field.is_primitive(field.get_primitive());
            }
            catch (...)
            {
                return false;
            }
        }

        /// Find all primitive polynomials of degree m
        template <size_t m>
        std::vector<typename GaloisField<m>::Element> find_primitive_polynomials()
        {
            static_assert(m <= 16, "Field size too large for exhaustive search");

            std::vector<typename GaloisField<m>::Element> primitives;
            const auto max_poly = (1u << (m + 1)) - 1;
            const auto min_poly = (1u << m) | 1; // Must have x^m and constant terms

            for (auto poly = min_poly; poly <= max_poly; poly += 2) // Only odd polynomials
            {
                if (is_primitive_polynomial<m>(poly))
                {
                    primitives.push_back(poly);
                }
            }

            return primitives;
        }

        /// Convert polynomial to string representation
        template <size_t m>
        std::string polynomial_to_string(typename GaloisField<m>::Element poly)
        {
            std::ostringstream oss;
            bool first = true;

            for (int i = m; i >= 0; --i)
            {
                if (poly & (1u << i))
                {
                    if (!first)
                        oss << " + ";

                    if (i == 0)
                        oss << "1";
                    else if (i == 1)
                        oss << "x";
                    else
                        oss << "x^" << i;

                    first = false;
                }
            }

            return first ? "0" : oss.str();
        }

        /// Compute minimal polynomial of an element
        template <size_t m>
        std::vector<typename GaloisField<m>::Element> minimal_polynomial(
            const GaloisField<m> &field, typename GaloisField<m>::Element alpha)
        {
            using Element = typename GaloisField<m>::Element;
            std::vector<Element> conjugates;
            std::set<Element> seen;

            Element current = alpha;
            while (seen.find(current) == seen.end())
            {
                seen.insert(current);
                conjugates.push_back(current);
                current = field.power(current, 2); // Frobenius map: x -> x^2
            }

            // Build polynomial with these roots
            std::vector<Element> poly = {1}; // Start with polynomial "1"

            for (Element root : conjugates)
            {
                // Multiply by (x - root) = (x + root) in GF(2^m)
                std::vector<Element> factor = {root, 1}; // x + root

                // Polynomial multiplication
                std::vector<Element> new_poly(poly.size() + factor.size() - 1, 0);
                for (size_t i = 0; i < poly.size(); ++i)
                {
                    for (size_t j = 0; j < factor.size(); ++j)
                    {
                        new_poly[i + j] = field.add(new_poly[i + j],
                                                    field.multiply(poly[i], factor[j]));
                    }
                }
                poly = std::move(new_poly);
            }

            return poly;
        }

    } // namespace galois_utils

    /// Galois field arithmetic benchmarks
    namespace galois_benchmark
    {

        template <size_t m>
        BenchmarkResults<m> benchmark_field_operations(size_t iterations)
        {
            GaloisField<m> field(galois_utils::get_default_primitive<m>());
            BenchmarkResults<m> results{};
            results.iterations = iterations;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dis(1, GaloisField<m>::field_size - 1);

            // Generate random test data
            std::vector<typename GaloisField<m>::Element> data1, data2;
            data1.reserve(iterations);
            data2.reserve(iterations);

            for (size_t i = 0; i < iterations; ++i)
            {
                data1.push_back(dis(gen));
                data2.push_back(dis(gen));
            }

            // Benchmark addition
            {
                auto start = std::chrono::high_resolution_clock::now();
                volatile typename GaloisField<m>::Element result = 0;

                for (size_t i = 0; i < iterations; ++i)
                {
                    result = field.add(data1[i], data2[i]);
                }

                auto end = std::chrono::high_resolution_clock::now();
                results.add_time_ns = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
                (void)result; // Suppress unused variable warning
            }

            // Benchmark multiplication
            {
                auto start = std::chrono::high_resolution_clock::now();
                volatile typename GaloisField<m>::Element result = 0;

                for (size_t i = 0; i < iterations; ++i)
                {
                    result = field.multiply(data1[i], data2[i]);
                }

                auto end = std::chrono::high_resolution_clock::now();
                results.mul_time_ns = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
                (void)result;
            }

            // Benchmark division
            {
                auto start = std::chrono::high_resolution_clock::now();
                volatile typename GaloisField<m>::Element result = 0;

                for (size_t i = 0; i < iterations; ++i)
                {
                    result = field.divide(data1[i], data2[i]);
                }

                auto end = std::chrono::high_resolution_clock::now();
                results.div_time_ns = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
                (void)result;
            }

            // Benchmark inverse
            {
                auto start = std::chrono::high_resolution_clock::now();
                volatile typename GaloisField<m>::Element result = 0;

                for (size_t i = 0; i < iterations; ++i)
                {
                    result = field.inverse(data1[i]);
                }

                auto end = std::chrono::high_resolution_clock::now();
                results.inv_time_ns = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
                (void)result;
            }

            // Benchmark exponentiation
            {
                auto start = std::chrono::high_resolution_clock::now();
                volatile typename GaloisField<m>::Element result = 0;

                for (size_t i = 0; i < iterations; ++i)
                {
                    result = field.power(data1[i], data2[i] % 16); // Small exponents
                }

                auto end = std::chrono::high_resolution_clock::now();
                results.exp_time_ns = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
                (void)result;
            }

            return results;
        }

        /// Print benchmark results
        template <size_t m>
        void print_benchmark_results(const BenchmarkResults<m> &results)
        {
            std::cout << "\n=== GF(2^" << m << ") Performance Benchmark ===\n";
            std::cout << "Iterations: " << results.iterations << "\n";
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Addition:       " << results.add_time_ns << " ns/op\n";
            std::cout << "Multiplication: " << results.mul_time_ns << " ns/op\n";
            std::cout << "Division:       " << results.div_time_ns << " ns/op\n";
            std::cout << "Inverse:        " << results.inv_time_ns << " ns/op\n";
            std::cout << "Exponentiation: " << results.exp_time_ns << " ns/op\n";
            std::cout << std::string(40, '=') << "\n";
        }

    } // namespace galois_benchmark

    /// Galois field testing utilities
    namespace galois_test
    {

        /// Verify field axioms for a given field
        template <size_t m>
        bool verify_field_axioms(const GaloisField<m> &field, size_t test_count)
        {
            using Element = typename GaloisField<m>::Element;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dis(0, GaloisField<m>::field_size - 1);

            // Test additive identity: a + 0 = a
            for (size_t i = 0; i < test_count; ++i)
            {
                Element a = dis(gen);
                if (field.add(a, 0) != a)
                {
                    std::cerr << "Additive identity failed for " << a << "\n";
                    return false;
                }
            }

            // Test multiplicative identity: a * 1 = a
            for (size_t i = 0; i < test_count; ++i)
            {
                Element a = dis(gen);
                if (a != 0 && field.multiply(a, 1) != a)
                {
                    std::cerr << "Multiplicative identity failed for " << a << "\n";
                    return false;
                }
            }

            // Test additive inverse: a + a = 0 (in GF(2^m))
            for (size_t i = 0; i < test_count; ++i)
            {
                Element a = dis(gen);
                if (field.add(a, a) != 0)
                {
                    std::cerr << "Additive inverse failed for " << a << "\n";
                    return false;
                }
            }

            // Test multiplicative inverse: a * a^(-1) = 1
            for (size_t i = 0; i < test_count; ++i)
            {
                Element a = dis(gen);
                if (a != 0)
                {
                    try
                    {
                        Element inv_a = field.inverse(a);
                        if (field.multiply(a, inv_a) != 1)
                        {
                            std::cerr << "Multiplicative inverse failed for " << a << "\n";
                            return false;
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Inverse calculation failed for " << a << ": " << e.what() << "\n";
                        return false;
                    }
                }
            }

            // Test distributivity: a * (b + c) = a*b + a*c
            for (size_t i = 0; i < test_count; ++i)
            {
                Element a = dis(gen);
                Element b = dis(gen);
                Element c = dis(gen);

                Element left = field.multiply(a, field.add(b, c));
                Element right = field.add(field.multiply(a, b), field.multiply(a, c));

                if (left != right)
                {
                    std::cerr << "Distributivity failed for a=" << a << ", b=" << b << ", c=" << c << "\n";
                    return false;
                }
            }

            return true;
        }

        /// Test polynomial operations
        template <size_t m>
        bool test_polynomial_operations(size_t test_count)
        {
            GaloisField<m> field(galois_utils::get_default_primitive<m>());

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dis(0, GaloisField<m>::field_size - 1);
            std::uniform_int_distribution<size_t> deg_dis(1, 5); // Small degree polynomials

            for (size_t test = 0; test < test_count; ++test)
            {
                // Create random polynomials
                size_t deg1 = deg_dis(gen);
                size_t deg2 = deg_dis(gen);

                std::vector<typename GaloisField<m>::Element> coeffs1(deg1 + 1);
                std::vector<typename GaloisField<m>::Element> coeffs2(deg2 + 1);

                for (auto &coeff : coeffs1)
                    coeff = dis(gen);
                for (auto &coeff : coeffs2)
                    coeff = dis(gen);

                // Ensure leading coefficients are non-zero
                coeffs1.back() = std::max(coeffs1.back(), 1u);
                coeffs2.back() = std::max(coeffs2.back(), 1u);

                GFPolynomial<m> poly1(field, coeffs1);
                GFPolynomial<m> poly2(field, coeffs2);

                // Test commutativity of addition: p1 + p2 = p2 + p1
                auto sum1 = poly1 + poly2;
                auto sum2 = poly2 + poly1;

                if (sum1.degree() != sum2.degree())
                {
                    std::cerr << "Addition commutativity failed (degree mismatch)\n";
                    return false;
                }

                // Test evaluation consistency
                typename GaloisField<m>::Element x = dis(gen);
                auto eval_sum = sum1.evaluate(x);
                auto eval_parts = field.add(poly1.evaluate(x), poly2.evaluate(x));

                if (eval_sum != eval_parts)
                {
                    std::cerr << "Polynomial evaluation failed\n";
                    return false;
                }
            }

            return true;
        }

        /// Comprehensive field test suite
        template <size_t m>
        bool run_comprehensive_tests()
        {
            std::cout << "\n=== Testing GF(2^" << m << ") ===\n";

            GaloisField<m> field(galois_utils::get_default_primitive<m>());

            std::cout << "Field size: " << GaloisField<m>::field_size << "\n";
            std::cout << "Primitive polynomial: 0x" << std::hex
                      << galois_utils::get_default_primitive<m>() << std::dec << "\n";
            std::cout << "Primitive polynomial: "
                      << galois_utils::polynomial_to_string<m>(galois_utils::get_default_primitive<m>()) << "\n";

            // Test field axioms
            std::cout << "Testing field axioms... " << std::flush;
            if (!verify_field_axioms(field))
            {
                std::cout << "FAILED\n";
                return false;
            }
            std::cout << "PASSED\n";

            // Test polynomial operations
            std::cout << "Testing polynomial operations... " << std::flush;
            if (!test_polynomial_operations<m>())
            {
                std::cout << "FAILED\n";
                return false;
            }
            std::cout << "PASSED\n";

            // Test primitive element
            std::cout << "Testing primitive element... " << std::flush;
            if (!field.is_primitive(field.get_primitive()))
            {
                std::cout << "FAILED\n";
                return false;
            }
            std::cout << "PASSED\n";

            std::cout << "All tests passed for GF(2^" << m << ")\n";
            return true;
        }

    } // namespace galois_test

    /// Demo functions for showcasing Galois field capabilities
    namespace galois_demo
    {

        /// Demonstrate basic field operations
        template <size_t m>
        void demonstrate_basic_operations()
        {
            std::cout << "\n=== GF(2^" << m << ") Basic Operations Demo ===\n";

            GaloisField<m> field(galois_utils::get_default_primitive<m>());

            // Show some basic operations
            const std::vector<typename GaloisField<m>::Element> elements = {1, 2, 3, 5, 7};

            std::cout << "Addition table (first few elements):\n";
            std::cout << "  +  ";
            for (auto b : elements)
                std::cout << std::setw(4) << b;
            std::cout << "\n";

            for (auto a : elements)
            {
                std::cout << std::setw(3) << a << " ";
                for (auto b : elements)
                {
                    std::cout << std::setw(4) << field.add(a, b);
                }
                std::cout << "\n";
            }

            std::cout << "\nMultiplication table (first few elements):\n";
            std::cout << "  ×  ";
            for (auto b : elements)
                std::cout << std::setw(4) << b;
            std::cout << "\n";

            for (auto a : elements)
            {
                std::cout << std::setw(3) << a << " ";
                for (auto b : elements)
                {
                    std::cout << std::setw(4) << field.multiply(a, b);
                }
                std::cout << "\n";
            }

            // Show inverses
            std::cout << "\nMultiplicative inverses:\n";
            for (auto a : elements)
            {
                auto inv_a = field.inverse(a);
                auto product = field.multiply(a, inv_a);
                std::cout << a << "^(-1) = " << inv_a << ", verify: " << a << " × " << inv_a << " = " << product << "\n";
            }
        }

        /// Demonstrate polynomial operations
        template <size_t m>
        void demonstrate_polynomial_operations()
        {
            std::cout << "\n=== GF(2^" << m << ") Polynomial Operations Demo ===\n";

            GaloisField<m> field(galois_utils::get_default_primitive<m>());

            // Create sample polynomials
            GFPolynomial<m> poly1(field, {1, 2, 3}); // 3x^2 + 2x + 1
            GFPolynomial<m> poly2(field, {2, 1});    // x + 2

            std::cout << "Polynomial 1: coefficients [1, 2, 3] (degree " << poly1.degree() << ")\n";
            std::cout << "Polynomial 2: coefficients [2, 1] (degree " << poly2.degree() << ")\n";

            auto sum = poly1 + poly2;
            auto product = poly1 * poly2;

            std::cout << "Sum: degree " << sum.degree() << "\n";
            std::cout << "Product: degree " << product.degree() << "\n";

            // Evaluate polynomials at different points
            std::cout << "\nEvaluation at different points:\n";
            for (typename GaloisField<m>::Element x = 0; x < 5 && x < GaloisField<m>::field_size; ++x)
            {
                auto val1 = poly1.evaluate(x);
                auto val2 = poly2.evaluate(x);
                auto val_sum = sum.evaluate(x);
                auto val_prod = product.evaluate(x);

                std::cout << "x=" << x << ": p1=" << val1 << ", p2=" << val2
                          << ", sum=" << val_sum << ", product=" << val_prod << "\n";
            }
        }

        /// Run complete demo
        void run_complete_demo()
        {
            std::cout << "Galois Field Operations Demo\n";
            std::cout << std::string(50, '=') << "\n";

            // Demo different field sizes
            demonstrate_basic_operations<3>();
            demonstrate_polynomial_operations<4>();

            // Run comprehensive tests
            galois_test::run_comprehensive_tests<3>();
            galois_test::run_comprehensive_tests<4>();
            galois_test::run_comprehensive_tests<8>();

            // Show performance benchmarks
            auto results8 = galois_benchmark::benchmark_field_operations<8>();
            galois_benchmark::print_benchmark_results(results8);
        }

    } // namespace galois_demo

    // Explicit template instantiations for common field sizes
    template class GaloisField<3>;
    template class GaloisField<4>;
    template class GaloisField<5>;
    template class GaloisField<6>;
    template class GaloisField<7>;
    template class GaloisField<8>;
    template class GaloisField<10>;
    template class GaloisField<12>;

    template class GFPolynomial<3>;
    template class GFPolynomial<4>;
    template class GFPolynomial<5>;
    template class GFPolynomial<6>;
    template class GFPolynomial<7>;
    template class GFPolynomial<8>;
    template class GFPolynomial<10>;
    template class GFPolynomial<12>;

} // namespace ecc
