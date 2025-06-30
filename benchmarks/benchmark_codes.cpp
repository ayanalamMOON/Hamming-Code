#include "ecc/hamming_code.hpp"
#include "ecc/reed_solomon.hpp"
#include "ecc/performance_analyzer.hpp"
#include <iostream>
#include <chrono>

namespace ecc::benchmark
{

    class CodeBenchmark
    {
    public:
        void run_benchmarks()
        {
            std::cout << "=== Error Correction Codes Benchmark Suite ===\n\n";

            benchmark_hamming_codes();
            benchmark_throughput();
            analyze_scalability();
        }

    private:
        void benchmark_hamming_codes()
        {
            std::cout << "Hamming Codes Performance:\n";
            std::cout << std::string(40, '-') << "\n";

            const size_t iterations = 100000;

            // Benchmark Hamming(7,4)
            {
                Hamming_7_4 code;
                auto start = std::chrono::high_resolution_clock::now();

                for (size_t i = 0; i < iterations; ++i)
                {
                    std::bitset<4> data(i % 16);
                    auto codeword = code.encode(data);
                    auto decoded = code.decode(codeword);
                    (void)decoded; // Suppress warning
                }

                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                double throughput = (iterations * 7) / (duration / 1000.0) / 1e6;

                std::cout << "Hamming(7,4):  " << std::fixed << std::setprecision(1)
                          << throughput << " Mbps\n";
            }

            // Benchmark Hamming(15,11)
            {
                Hamming_15_11 code;
                auto start = std::chrono::high_resolution_clock::now();

                for (size_t i = 0; i < iterations; ++i)
                {
                    std::bitset<11> data(i % 2048);
                    auto codeword = code.encode(data);
                    auto decoded = code.decode(codeword);
                    (void)decoded; // Suppress warning
                }

                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::milli>(end - start).count();
                double throughput = (iterations * 15) / (duration / 1000.0) / 1e6;

                std::cout << "Hamming(15,11): " << std::fixed << std::setprecision(1)
                          << throughput << " Mbps\n";
            }
        }

        void benchmark_throughput()
        {
            std::cout << "\nThroughput Analysis:\n";
            std::cout << std::string(25, '-') << "\n";

            PerformanceAnalyzer analyzer;

            // Compare different codes at fixed SNR
            std::cout << "Code comparison at 5 dB SNR:\n";
            analyzer.compare_codes<Hamming_7_4, Hamming_15_11>(
                ChannelType::AWGN, 5.0, 10000);
        }

        void analyze_scalability()
        {
            std::cout << "\nScalability Analysis:\n";
            std::cout << std::string(25, '-') << "\n";
            std::cout << "Code parameters and theoretical limits:\n";

            std::cout << "Hamming(7,4):   Rate = 0.571, dmin = 3, t = 1\n";
            std::cout << "Hamming(15,11): Rate = 0.733, dmin = 3, t = 1\n";
            std::cout << "Hamming(31,26): Rate = 0.839, dmin = 3, t = 1\n";
            std::cout << "RS(255,223):    Rate = 0.874, dmin = 33, t = 16\n";
        }
    };

} // namespace ecc::benchmark

int main()
{
    ecc::benchmark::CodeBenchmark benchmark;
    benchmark.run_benchmarks();
    return 0;
}
