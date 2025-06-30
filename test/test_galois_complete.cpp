#include "ecc/galois_field.hpp"
#include <iostream>

int main()
{
    std::cout << "Testing Galois Field Implementation\n";
    std::cout << "===================================\n";

    try
    {
        // Test factory functions
        std::cout << "\n1. Testing factory functions...\n";
        auto gf256 = ecc::galois::create_gf256();
        auto gf1024 = ecc::galois::create_gf1024();
        auto gf4096 = ecc::galois::create_gf4096();

        std::cout << "✓ GF(256) factory created successfully\n";
        std::cout << "✓ GF(1024) factory created successfully\n";
        std::cout << "✓ GF(4096) factory created successfully\n";

        // Test basic operations
        std::cout << "\n2. Testing basic GF(256) operations...\n";
        auto result_add = gf256->add(15, 240);
        auto result_mul = gf256->multiply(15, 17);
        auto result_inv = gf256->inverse(15);
        auto result_pow = gf256->power(2, 10);

        std::cout << "15 + 240 = " << result_add << "\n";
        std::cout << "15 × 17 = " << result_mul << "\n";
        std::cout << "15^(-1) = " << result_inv << "\n";
        std::cout << "2^10 = " << result_pow << "\n";

        // Verify inverse property
        auto verify_inv = gf256->multiply(15, result_inv);
        std::cout << "Verify: 15 × 15^(-1) = " << verify_inv << " (should be 1)\n";

        // Test polynomial operations
        std::cout << "\n3. Testing polynomial operations...\n";
        ecc::GFPolynomial<8> poly1(*gf256, {1, 2, 3}); // 3x^2 + 2x + 1
        ecc::GFPolynomial<8> poly2(*gf256, {2, 1});    // x + 2

        auto poly_sum = poly1 + poly2;
        auto poly_product = poly1 * poly2;

        std::cout << "Polynomial 1 degree: " << poly1.degree() << "\n";
        std::cout << "Polynomial 2 degree: " << poly2.degree() << "\n";
        std::cout << "Sum degree: " << poly_sum.degree() << "\n";
        std::cout << "Product degree: " << poly_product.degree() << "\n";

        // Test evaluation
        auto eval_at_5 = poly1.evaluate(5);
        std::cout << "Polynomial 1 evaluated at x=5: " << eval_at_5 << "\n";

        // Run utility tests
        std::cout << "\n4. Testing utility functions...\n";
        auto default_prim = ecc::galois_utils::get_default_primitive<8>();
        std::cout << "Default primitive for GF(256): 0x" << std::hex << default_prim << std::dec << "\n";

        auto poly_str = ecc::galois_utils::polynomial_to_string<8>(default_prim);
        std::cout << "Polynomial representation: " << poly_str << "\n";

        // Run comprehensive tests for smaller fields
        std::cout << "\n5. Running comprehensive tests...\n";
        bool test_result = ecc::galois_test::run_comprehensive_tests<4>();
        std::cout << "GF(16) comprehensive test: " << (test_result ? "PASSED" : "FAILED") << "\n";

        // Run demo
        std::cout << "\n6. Running basic operations demo...\n";
        ecc::galois_demo::demonstrate_basic_operations<4>();

        // Run performance benchmark
        std::cout << "\n7. Running performance benchmark...\n";
        auto bench_results = ecc::galois_benchmark::benchmark_field_operations<8>(100000);
        ecc::galois_benchmark::print_benchmark_results(bench_results);

        std::cout << "\n✓ All tests completed successfully!\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
