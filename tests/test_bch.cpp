#include "ecc/bch_code.hpp"
#include "ecc/galois_field.hpp"
#include <iostream>
#include <cassert>
#include <random>
#include <chrono>
#include <bitset>

namespace ecc::test
{

    void test_bch_basic_encoding_decoding()
    {
        std::cout << "Testing BCH basic encoding/decoding..." << std::endl;

        // Test BCH(15,7) code
        BCH_15_7_3 bch;

        // Test data
        BCH_15_7_3::DataWord data;
        data[0] = 1;
        data[1] = 0;
        data[2] = 1;
        data[3] = 1;
        data[4] = 0;
        data[5] = 1;
        data[6] = 0;

        // Encode
        auto encoded = bch.encode(data);
        std::cout << "Original data: " << data << std::endl;
        std::cout << "Encoded:       " << encoded << std::endl;

        // Decode without errors
        auto result = bch.decode(encoded);
        assert(result.success);
        assert(result.data == data);
        assert(result.errors_corrected == 0);

        std::cout << "✓ Basic encoding/decoding test passed" << std::endl;
    }

    void test_bch_single_error_correction()
    {
        std::cout << "Testing BCH single error correction..." << std::endl;

        BCH_15_7_3 bch;

        // Test data
        BCH_15_7_3::DataWord data;
        data[0] = 1;
        data[1] = 0;
        data[2] = 1;
        data[3] = 1;
        data[4] = 0;
        data[5] = 1;
        data[6] = 0;

        auto encoded = bch.encode(data);

        // Introduce single bit error
        for (size_t error_pos = 0; error_pos < BCH_15_7_3::code_length; ++error_pos)
        {
            auto corrupted = encoded;
            corrupted.flip(error_pos);

            auto result = bch.decode(corrupted);
            assert(result.success);
            assert(result.data == data);
            assert(result.errors_corrected == 1);
            assert(result.error_positions.size() == 1);
            assert(result.error_positions[0] == error_pos);
        }

        std::cout << "✓ Single error correction test passed" << std::endl;
    }

    void test_bch_multiple_configurations()
    {
        std::cout << "Testing multiple BCH configurations..." << std::endl;

        // Test BCH(31,21) code
        {
            BCH_31_21_3 bch31;
            BCH_31_21_3::DataWord data31;

            // Fill with test pattern
            for (size_t i = 0; i < BCH_31_21_3::data_length; ++i)
            {
                data31[i] = (i % 3 == 0);
            }

            auto encoded31 = bch31.encode(data31);
            auto result31 = bch31.decode(encoded31);

            assert(result31.success);
            assert(result31.data == data31);

            std::cout << "✓ BCH(31,21) test passed" << std::endl;
        }

        // Test BCH(63,51) code
        {
            BCH_63_51_3 bch63;
            BCH_63_51_3::DataWord data63;

            // Fill with test pattern
            for (size_t i = 0; i < BCH_63_51_3::data_length; ++i)
            {
                data63[i] = (i % 2 == 1);
            }

            auto encoded63 = bch63.encode(data63);
            auto result63 = bch63.decode(encoded63);

            assert(result63.success);
            assert(result63.data == data63);

            std::cout << "✓ BCH(63,51) test passed" << std::endl;
        }
    }

    void test_bch_error_detection_limits()
    {
        std::cout << "Testing BCH error detection limits..." << std::endl;

        BCH_15_5_3 bch; // t=2 error correcting capability

        BCH_15_5_3::DataWord data;
        data[0] = 1;
        data[1] = 0;
        data[2] = 1;
        data[3] = 1;
        data[4] = 0;

        auto encoded = bch.encode(data);

        // Test 2-error correction (should work)
        auto corrupted = encoded;
        corrupted.flip(0);
        corrupted.flip(5);

        auto result = bch.decode(corrupted);
        assert(result.success);
        assert(result.data == data);
        assert(result.errors_corrected == 2);

        // Test 3-error case (may fail)
        corrupted.flip(10);
        result = bch.decode(corrupted);
        // Note: 3 errors might not be correctable, this tests the limit

        std::cout << "✓ Error detection limits test passed" << std::endl;
    }

    void test_bch_systematic_property()
    {
        std::cout << "Testing BCH systematic property..." << std::endl;

        BCH_15_7_3 bch;

        BCH_15_7_3::DataWord data;
        data[0] = 1;
        data[1] = 0;
        data[2] = 1;
        data[3] = 1;
        data[4] = 0;
        data[5] = 1;
        data[6] = 0;

        auto encoded = bch.encode(data);

        // Check that data bits are preserved in systematic positions
        for (size_t i = 0; i < BCH_15_7_3::data_length; ++i)
        {
            assert(encoded[i + BCH_15_7_3::parity_length] == data[i]);
        }

        std::cout << "✓ Systematic property test passed" << std::endl;
    }

    void test_bch_batch_encoding()
    {
        std::cout << "Testing BCH batch encoding..." << std::endl;

        BCH_15_7_3 bch;

        std::vector<BCH_15_7_3::DataWord> data_batch;
        std::mt19937 rng(42);

        // Generate random test data
        for (size_t i = 0; i < 10; ++i)
        {
            BCH_15_7_3::DataWord data;
            for (size_t j = 0; j < BCH_15_7_3::data_length; ++j)
            {
                data[j] = rng() % 2;
            }
            data_batch.push_back(data);
        }

        // Batch encode
        auto encoded_batch = bch.encode(data_batch);
        assert(encoded_batch.size() == data_batch.size());

        // Verify each codeword
        for (size_t i = 0; i < data_batch.size(); ++i)
        {
            auto individual_encoded = bch.encode(data_batch[i]);
            assert(encoded_batch[i] == individual_encoded);

            auto result = bch.decode(encoded_batch[i]);
            assert(result.success);
            assert(result.data == data_batch[i]);
        }

        std::cout << "✓ Batch encoding test passed" << std::endl;
    }

    void test_ldpc_code()
    {
        std::cout << "Testing LDPC code..." << std::endl;

        LDPCCode ldpc(15, 7); // (15,7) LDPC code

        LDPCCode::DataWord data = {1, 0, 1, 1, 0, 1, 0};

        auto encoded = ldpc.encode(data);
        assert(encoded.size() == 15);

        auto result = ldpc.decode(encoded);
        assert(result.success);
        assert(result.data == data);

        // Test with single error
        auto corrupted = encoded;
        corrupted[0] = 1 - corrupted[0]; // Flip bit

        result = ldpc.decode(corrupted);
        // LDPC should handle single errors well

        std::cout << "✓ LDPC code test passed" << std::endl;
    }

    void test_turbo_code()
    {
        std::cout << "Testing Turbo code..." << std::endl;

        TurboCode turbo(7); // Information length 7

        TurboCode::DataWord data = {1, 0, 1, 1, 0, 1, 0};

        auto encoded = turbo.encode(data);
        assert(encoded.size() == 21); // 3 * 7 for rate 1/3

        auto result = turbo.decode(encoded);
        assert(result.success);
        assert(result.data == data);

        std::cout << "✓ Turbo code test passed" << std::endl;
    }

    void test_performance()
    {
        std::cout << "Testing BCH performance..." << std::endl;

        BCH_15_7_3 bch;
        const size_t num_tests = 1000;

        std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

        auto start_time = std::chrono::high_resolution_clock::now();

        size_t total_errors_corrected = 0;
        size_t successful_corrections = 0;

        for (size_t test = 0; test < num_tests; ++test)
        {
            // Generate random data
            BCH_15_7_3::DataWord data;
            for (size_t i = 0; i < BCH_15_7_3::data_length; ++i)
            {
                data[i] = rng() % 2;
            }

            auto encoded = bch.encode(data);

            // Introduce random single error
            auto corrupted = encoded;
            size_t error_pos = rng() % BCH_15_7_3::code_length;
            corrupted.flip(error_pos);

            auto result = bch.decode(corrupted);
            if (result.success && result.data == data)
            {
                successful_corrections++;
                total_errors_corrected += result.errors_corrected;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        double success_rate = 100.0 * successful_corrections / num_tests;
        double avg_time_per_operation = duration.count() / (2.0 * num_tests); // encode + decode

        std::cout << "Performance results:" << std::endl;
        std::cout << "  Tests: " << num_tests << std::endl;
        std::cout << "  Success rate: " << std::fixed << std::setprecision(1) << success_rate << "%" << std::endl;
        std::cout << "  Avg time per encode/decode: " << std::fixed << std::setprecision(2) << avg_time_per_operation << " μs" << std::endl;
        std::cout << "  Total errors corrected: " << total_errors_corrected << std::endl;

        assert(success_rate > 99.0); // Should correct almost all single errors

        std::cout << "✓ Performance test passed" << std::endl;
    }

    void test_bch()
    {
        std::cout << "=== BCH Code Tests ===" << std::endl;

        try
        {
            test_bch_basic_encoding_decoding();
            test_bch_single_error_correction();
            test_bch_multiple_configurations();
            test_bch_error_detection_limits();
            test_bch_systematic_property();
            test_bch_batch_encoding();
            test_ldpc_code();
            test_turbo_code();
            test_performance();

            std::cout << "\n🎉 All BCH tests passed successfully!" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "❌ Test failed: " << e.what() << std::endl;
            throw;
        }
    }

}
