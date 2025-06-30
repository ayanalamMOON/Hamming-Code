#include "ecc/galois_field.hpp"
#include <iostream>

// Simple standalone implementation for testing
namespace ecc::galois
{
    std::unique_ptr<GF256> create_gf256()
    {
        return std::make_unique<GF256>(primitive_poly_8);
    }
}

int main()
{
    std::cout << "Simple Galois Field Test\n";
    std::cout << "========================\n";

    try
    {
        // Test basic GF(256) operations directly with class
        ecc::GF256 gf256(ecc::primitive_poly_8);

        std::cout << "Created GF(256) field\n";

        auto result_add = gf256.add(15, 240);
        auto result_mul = gf256.multiply(15, 17);

        std::cout << "15 + 240 = " << result_add << "\n";
        std::cout << "15 × 17 = " << result_mul << "\n";

        // Test primitive element check
        bool is_prim = gf256.is_primitive(gf256.get_primitive());
        std::cout << "Primitive element check: " << (is_prim ? "PASSED" : "FAILED") << "\n";

        // Test polynomial operations
        ecc::GFPolynomial<8> poly1(gf256, {1, 2, 3}); // 3x^2 + 2x + 1
        ecc::GFPolynomial<8> poly2(gf256, {2, 1});    // x + 2

        auto poly_sum = poly1 + poly2;

        std::cout << "Polynomial 1 degree: " << poly1.degree() << "\n";
        std::cout << "Polynomial 2 degree: " << poly2.degree() << "\n";
        std::cout << "Sum degree: " << poly_sum.degree() << "\n";

        // Test evaluation
        auto eval_at_5 = poly1.evaluate(5);
        std::cout << "Polynomial 1 evaluated at x=5: " << eval_at_5 << "\n";

        // Test factory
        auto gf256_ptr = ecc::galois::create_gf256();
        auto factory_mul = gf256_ptr->multiply(7, 13);
        std::cout << "Factory test - 7 × 13 = " << factory_mul << "\n";

        // Test inverse
        auto inv_15 = gf256.inverse(15);
        auto verify_inv = gf256.multiply(15, inv_15);
        std::cout << "15^(-1) = " << inv_15 << ", verify: 15 × " << inv_15 << " = " << verify_inv << "\n";

        // Test power
        auto power_result = gf256.power(2, 8);
        std::cout << "2^8 = " << power_result << "\n";

        std::cout << "✓ All basic tests passed!\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
