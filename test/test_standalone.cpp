#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace ecc_test
{
    /// Simple Galois Field GF(2^8) implementation for testing
    class SimpleGF256
    {
    public:
        static constexpr size_t field_size = 256;
        static constexpr uint32_t primitive_poly = 0x11D; // x^8 + x^4 + x^3 + x^2 + 1
        using Element = uint32_t;

    private:
        std::array<Element, field_size> exp_table;
        std::array<Element, field_size> log_table;

    public:
        SimpleGF256()
        {
            build_tables();
        }

        Element add(Element a, Element b) const noexcept
        {
            return a ^ b;
        }

        Element multiply(Element a, Element b) const noexcept
        {
            if (a == 0 || b == 0)
                return 0;
            size_t log_sum = (log_table[a] + log_table[b]) % (field_size - 1);
            return exp_table[log_sum];
        }

        Element inverse(Element a) const
        {
            if (a == 0)
                throw std::invalid_argument("Zero has no multiplicative inverse");
            size_t inv_log = (field_size - 1 - log_table[a]) % (field_size - 1);
            return exp_table[inv_log];
        }

        Element power(Element base, size_t exponent) const noexcept
        {
            if (base == 0)
                return (exponent == 0) ? 1 : 0;
            size_t log_result = (log_table[base] * exponent) % (field_size - 1);
            return exp_table[log_result];
        }

    private:
        void build_tables()
        {
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

            log_table[0] = 0;
            for (size_t i = 0; i < field_size - 1; ++i)
            {
                log_table[exp_table[i]] = i;
            }
        }
    };
}

int main()
{
    std::cout << "Self-Contained Galois Field Test\n";
    std::cout << "================================\n";

    try
    {
        ecc_test::SimpleGF256 gf256;

        std::cout << "Created GF(256) field\n";

        auto result_add = gf256.add(15, 240);
        auto result_mul = gf256.multiply(15, 17);

        std::cout << "15 + 240 = " << result_add << "\n";
        std::cout << "15 × 17 = " << result_mul << "\n";

        // Test inverse
        auto inv_15 = gf256.inverse(15);
        auto verify_inv = gf256.multiply(15, inv_15);
        std::cout << "15^(-1) = " << inv_15 << ", verify: 15 × " << inv_15 << " = " << verify_inv << "\n";

        // Test power
        auto power_result = gf256.power(2, 8);
        std::cout << "2^8 = " << power_result << "\n";

        // Test field properties
        std::cout << "\nTesting field properties:\n";

        // Test additive identity
        for (int i = 1; i <= 5; ++i)
        {
            auto result = gf256.add(i, 0);
            std::cout << i << " + 0 = " << result << " (should be " << i << ")\n";
        }

        // Test multiplicative identity
        for (int i = 1; i <= 5; ++i)
        {
            auto result = gf256.multiply(i, 1);
            std::cout << i << " × 1 = " << result << " (should be " << i << ")\n";
        }

        // Test additive inverse (in GF(2^n), a + a = 0)
        for (int i = 1; i <= 5; ++i)
        {
            auto result = gf256.add(i, i);
            std::cout << i << " + " << i << " = " << result << " (should be 0)\n";
        }

        std::cout << "✓ All tests completed successfully!\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
