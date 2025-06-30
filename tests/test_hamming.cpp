#include "ecc/hamming_code.hpp"
#include "ecc/performance_analyzer.hpp"
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>

namespace ecc::test
{

    class HammingTestSuite
    {
    private:
        std::mt19937 rng;
        size_t passed_tests = 0;
        size_t total_tests = 0;

    public:
        HammingTestSuite() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

        void run_all_tests()
        {
            std::cout << "Running Hamming Code Test Suite...\n";
            std::cout << std::string(50, '=') << "\n";

            test_hamming_7_4_basic();
            test_hamming_15_11_basic();
            test_single_error_correction();
            test_double_error_detection();
            test_systematic_encoding();
            test_syndrome_calculation();
            test_performance_requirements();
            test_edge_cases();

            print_results();
        }

    private:
        void test_hamming_7_4_basic()
        {
            std::cout << "Testing Hamming(7,4) basic functionality... ";

            Hamming_7_4 code;
            std::bitset<4> data("1011");

            auto codeword = code.encode(data);
            auto decoded = code.decode(codeword);

            assert_test(data == decoded, "Basic encode/decode");
            assert_test(code.get_code_rate() == 4.0 / 7.0, "Code rate");
            assert_test(code.get_min_distance() == 3, "Minimum distance");

            std::cout << "✓\n";
        }

        void test_hamming_15_11_basic()
        {
            std::cout << "Testing Hamming(15,11) basic functionality... ";

            Hamming_15_11 code;
            std::bitset<11> data("10110100101");

            auto codeword = code.encode(data);
            auto decoded = code.decode(codeword);

            assert_test(data == decoded, "Basic encode/decode (15,11)");

            std::cout << "✓\n";
        }

        void test_single_error_correction()
        {
            std::cout << "Testing single error correction... ";

            Hamming_7_4 code;
            std::uniform_int_distribution<int> bit_dist(0, 1);

            // Test with random data
            for (int test = 0; test < 100; ++test)
            {
                std::bitset<4> data;
                for (int i = 0; i < 4; ++i)
                {
                    data[i] = bit_dist(rng);
                }

                auto codeword = code.encode(data);

                // Introduce single error at each position
                for (size_t error_pos = 0; error_pos < 7; ++error_pos)
                {
                    auto received = codeword;
                    received.flip(error_pos);

                    auto result = code.decode_with_detection(received);

                    assert_test(result.error_detected, "Error detection");
                    assert_test(result.error_position == error_pos, "Error localization");
                    assert_test(result.data == data, "Error correction");
                }
            }

            std::cout << "✓\n";
        }

        void test_double_error_detection()
        {
            std::cout << "Testing double error detection... ";

            SECDED_8_4 code;
            std::bitset<4> data("1010");

            auto codeword = code.encode(data);

            // Introduce two errors
            auto received = codeword;
            received.flip(0);
            received.flip(3);

            auto result = code.decode_secded(received);

            assert_test(result.status == SECDED_8_4::SECDEDResult::Status::DOUBLE_ERROR_DETECTED,
                        "Double error detection");

            std::cout << "✓\n";
        }

        void test_systematic_encoding()
        {
            std::cout << "Testing systematic encoding... ";

            Hamming_7_4 code;
            std::bitset<4> data("1101");

            auto codeword = code.encode(data);

            // Check that data bits are preserved in systematic positions
            for (int i = 0; i < 4; ++i)
            {
                assert_test(codeword[i] == data[i], "Systematic property");
            }

            std::cout << "✓\n";
        }

        void test_syndrome_calculation()
        {
            std::cout << "Testing syndrome calculation... ";

            Hamming_7_4 code;
            std::bitset<4> data("0000");

            auto codeword = code.encode(data);
            auto syndrome = code.calculate_syndrome(codeword);

            // Zero syndrome for valid codeword
            assert_test(syndrome.none(), "Zero syndrome for valid codeword");

            // Non-zero syndrome for corrupted codeword
            auto corrupted = codeword;
            corrupted.flip(2);
            auto error_syndrome = code.calculate_syndrome(corrupted);

            assert_test(!error_syndrome.none(), "Non-zero syndrome for error");

            std::cout << "✓\n";
        }

        void test_performance_requirements()
        {
            std::cout << "Testing performance requirements... ";

            const size_t iterations = 10000;
            Hamming_15_11 code;

            std::vector<std::bitset<11>> test_data(iterations);
            std::uniform_int_distribution<int> bit_dist(0, 1);

            // Generate test data
            for (auto &data : test_data)
            {
                for (int i = 0; i < 11; ++i)
                {
                    data[i] = bit_dist(rng);
                }
            }

            // Measure encoding performance
            auto start = std::chrono::high_resolution_clock::now();
            for (const auto &data : test_data)
            {
                auto codeword = code.encode(data);
                (void)codeword; // Suppress unused variable warning
            }
            auto end = std::chrono::high_resolution_clock::now();

            auto encoding_time = std::chrono::duration<double, std::milli>(end - start).count();
            double encoding_throughput = (iterations * 11) / (encoding_time / 1000.0) / 1e6; // Mbps

            assert_test(encoding_throughput > 100.0, "Encoding throughput > 100 Mbps");

            std::cout << "✓ (Throughput: " << std::fixed << std::setprecision(1)
                      << encoding_throughput << " Mbps)\n";
        }

        void test_edge_cases()
        {
            std::cout << "Testing edge cases... ";

            Hamming_7_4 code;

            // All zeros
            std::bitset<4> all_zeros("0000");
            auto codeword_zeros = code.encode(all_zeros);
            auto decoded_zeros = code.decode(codeword_zeros);
            assert_test(all_zeros == decoded_zeros, "All zeros");

            // All ones
            std::bitset<4> all_ones("1111");
            auto codeword_ones = code.encode(all_ones);
            auto decoded_ones = code.decode(codeword_ones);
            assert_test(all_ones == decoded_ones, "All ones");

            std::cout << "✓\n";
        }

        void assert_test(bool condition, const std::string &test_name)
        {
            total_tests++;
            if (condition)
            {
                passed_tests++;
            }
            else
            {
                std::cout << "\n  ✗ FAILED: " << test_name << "\n";
            }
        }

        void print_results()
        {
            std::cout << std::string(50, '=') << "\n";
            std::cout << "Test Results: " << passed_tests << "/" << total_tests << " passed";

            if (passed_tests == total_tests)
            {
                std::cout << " ✓ ALL TESTS PASSED!\n";
            }
            else
            {
                std::cout << " ✗ SOME TESTS FAILED!\n";
            }

            double pass_rate = static_cast<double>(passed_tests) / total_tests * 100.0;
            std::cout << "Pass rate: " << std::fixed << std::setprecision(1) << pass_rate << "%\n";
        }
    };

} // namespace ecc::test

int main()
{
    ecc::test::HammingTestSuite test_suite;
    test_suite.run_all_tests();
    return 0;
}
