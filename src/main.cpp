#include "ecc/hamming_code.hpp"
#include "ecc/reed_solomon.hpp"
#include "ecc/performance_analyzer.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace ecc
{

    /// Command-line interface for the ECC library
    class CLI
    {
    private:
        std::map<std::string, std::function<void(const std::vector<std::string> &)>> commands;
        PerformanceAnalyzer analyzer;

    public:
        CLI()
        {
            register_commands();
        }

        void run(int argc, char *argv[])
        {
            if (argc < 2)
            {
                print_help();
                return;
            }

            std::vector<std::string> args(argv + 1, argv + argc);
            std::string command = args[0];

            auto it = commands.find(command);
            if (it != commands.end())
            {
                try
                {
                    it->second(args);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
            }
            else
            {
                std::cerr << "Unknown command: " << command << std::endl;
                print_help();
            }
        }

    private:
        void register_commands()
        {
            commands["help"] = [this](const auto &)
            { print_help(); };
            commands["encode"] = [this](const auto &args)
            { encode_command(args); };
            commands["decode"] = [this](const auto &args)
            { decode_command(args); };
            commands["analyze"] = [this](const auto &args)
            { analyze_command(args); };
            commands["compare"] = [this](const auto &args)
            { compare_command(args); };
            commands["demo"] = [this](const auto &args)
            { demo_command(args); };
        }

        void print_help() const
        {
            std::cout << "Advanced Error Correction Codes Library\n";
            std::cout << "Usage: ecc_demo <command> [options]\n\n";
            std::cout << "Commands:\n";
            std::cout << "  help              Show this help message\n";
            std::cout << "  encode            Encode data with specified code\n";
            std::cout << "  decode            Decode received data\n";
            std::cout << "  analyze           Analyze code performance\n";
            std::cout << "  compare           Compare multiple codes\n";
            std::cout << "  demo              Run demonstration examples\n\n";
            std::cout << "Examples:\n";
            std::cout << "  ecc_demo encode --code hamming --n 7 --k 4 --data \"1011\"\n";
            std::cout << "  ecc_demo analyze --code hamming --snr 0:10:1 --iterations 1000\n";
            std::cout << "  ecc_demo compare --codes hamming,rs --snr 5\n";
        }

        void encode_command(const std::vector<std::string> &args)
        {
            std::string code_type = "hamming";
            int n = 7, k = 4;
            std::string data = "1011";

            // Parse arguments
            for (size_t i = 1; i < args.size(); i += 2)
            {
                if (i + 1 < args.size())
                {
                    if (args[i] == "--code")
                        code_type = args[i + 1];
                    else if (args[i] == "--n")
                        n = std::stoi(args[i + 1]);
                    else if (args[i] == "--k")
                        k = std::stoi(args[i + 1]);
                    else if (args[i] == "--data")
                        data = args[i + 1];
                }
            }

            if (code_type == "hamming" && n == 7 && k == 4)
            {
                encode_hamming<7, 4>(data);
            }
            else if (code_type == "hamming" && n == 15 && k == 11)
            {
                encode_hamming<15, 11>(data);
            }
            else if (code_type == "rs" && n == 255 && k == 223)
            {
                encode_reed_solomon<255, 223>(data);
            }
            else
            {
                std::cerr << "Unsupported code parameters\n";
            }
        }

        void decode_command(const std::vector<std::string> &args)
        {
            std::string code_type = "hamming";
            int n = 7, k = 4;
            std::string received_data;

            // Parse arguments
            for (size_t i = 1; i < args.size(); i += 2)
            {
                if (i + 1 < args.size())
                {
                    if (args[i] == "--code")
                        code_type = args[i + 1];
                    else if (args[i] == "--n")
                        n = std::stoi(args[i + 1]);
                    else if (args[i] == "--k")
                        k = std::stoi(args[i + 1]);
                    else if (args[i] == "--received")
                        received_data = args[i + 1];
                }
            }

            if (received_data.empty())
            {
                std::cerr << "No received data provided\n";
                return;
            }

            if (code_type == "hamming" && n == 7 && k == 4)
            {
                decode_hamming<7, 4>(received_data);
            }
            else if (code_type == "hamming" && n == 15 && k == 11)
            {
                decode_hamming<15, 11>(received_data);
            }
            else
            {
                std::cerr << "Unsupported code parameters\n";
            }
        }

        void analyze_command(const std::vector<std::string> &args)
        {
            std::string code_type = "hamming";
            std::string snr_range = "0:10:1";
            int iterations = 1000;

            // Parse arguments
            for (size_t i = 1; i < args.size(); i += 2)
            {
                if (i + 1 < args.size())
                {
                    if (args[i] == "--code")
                        code_type = args[i + 1];
                    else if (args[i] == "--snr")
                        snr_range = args[i + 1];
                    else if (args[i] == "--iterations")
                        iterations = std::stoi(args[i + 1]);
                }
            }

            // Parse SNR range
            auto [snr_min, snr_max, snr_step] = parse_range(snr_range);

            std::cout << "Analyzing " << code_type << " code performance...\n";

            if (code_type == "hamming")
            {
                auto results = analyzer.analyze_ber_curve<Hamming_7_4>(
                    snr_min, snr_max, snr_step, iterations);
                analyzer.save_results(results, "hamming_7_4_analysis.csv");
            }
            else if (code_type == "rs")
            {
                auto results = analyzer.analyze_ber_curve<RS_255_223>(
                    snr_min, snr_max, snr_step, iterations);
                analyzer.save_results(results, "rs_255_223_analysis.csv");
            }
        }

        void compare_command(const std::vector<std::string> &args)
        {
            double snr = 5.0;
            int iterations = 1000;

            // Parse arguments
            for (size_t i = 1; i < args.size(); i += 2)
            {
                if (i + 1 < args.size())
                {
                    if (args[i] == "--snr")
                        snr = std::stod(args[i + 1]);
                    else if (args[i] == "--iterations")
                        iterations = std::stoi(args[i + 1]);
                }
            }

            std::cout << "Comparing codes at SNR = " << snr << " dB\n";
            analyzer.compare_codes<Hamming_7_4, Hamming_15_11, RS_255_223>(
                ChannelType::AWGN, snr, iterations);
        }

        void demo_command(const std::vector<std::string> &args)
        {
            std::cout << "=== Advanced Error Correction Codes Demo ===\n\n";

            // Hamming Code Demo
            std::cout << "1. Hamming(7,4) Code Demo:\n";
            std::cout << std::string(30, '-') << "\n";
            demo_hamming();

            std::cout << "\n2. Reed-Solomon(255,223) Code Demo:\n";
            std::cout << std::string(35, '-') << "\n";
            demo_reed_solomon();

            std::cout << "\n3. Performance Comparison:\n";
            std::cout << std::string(25, '-') << "\n";
            analyzer.compare_codes<Hamming_7_4, Hamming_15_11>(
                ChannelType::AWGN, 5.0, 1000);
        }

        template <size_t n, size_t k>
        void encode_hamming(const std::string &data_str)
        {
            if (data_str.length() != k)
            {
                std::cerr << "Data length must be " << k << " bits\n";
                return;
            }

            HammingCode<n, k> code;
            typename HammingCode<n, k>::DataWord data;

            for (size_t i = 0; i < k; ++i)
            {
                data[i] = (data_str[i] == '1');
            }

            auto codeword = code.encode(data);

            std::cout << "Original data: " << data_str << "\n";
            std::cout << "Encoded data:  ";
            for (size_t i = 0; i < n; ++i)
            {
                std::cout << (codeword[i] ? '1' : '0');
            }
            std::cout << "\n";
            std::cout << "Code rate:     " << code.get_code_rate() << "\n";
            std::cout << "Min distance:  " << code.get_min_distance() << "\n";
        }

        template <size_t n, size_t k>
        void decode_hamming(const std::string &received_str)
        {
            if (received_str.length() != n)
            {
                std::cerr << "Received data length must be " << n << " bits\n";
                return;
            }

            HammingCode<n, k> code;
            typename HammingCode<n, k>::CodeWord received;

            for (size_t i = 0; i < n; ++i)
            {
                received[i] = (received_str[i] == '1');
            }

            auto result = code.decode_with_detection(received);

            std::cout << "Received data: " << received_str << "\n";
            std::cout << "Decoded data:  ";
            for (size_t i = 0; i < k; ++i)
            {
                std::cout << (result.data[i] ? '1' : '0');
            }
            std::cout << "\n";

            if (result.error_detected)
            {
                std::cout << "Error detected at position: " << result.error_position << "\n";
                std::cout << "Error corrected successfully!\n";
            }
            else
            {
                std::cout << "No errors detected.\n";
            }
        }

        template <size_t n, size_t k>
        void encode_reed_solomon(const std::string &data_str)
        {
            // Simplified RS demo with byte data
            std::cout << "Reed-Solomon encoding demo (simplified)\n";
            std::cout << "Code parameters: RS(" << n << "," << k << ")\n";
            std::cout << "Symbol size: 8 bits\n";
            std::cout << "Error correction capability: " << (n - k) / 2 << " symbols\n";
        }

        void demo_hamming()
        {
            Hamming_7_4 code;
            std::bitset<4> data("1011");

            std::cout << "Original data:     ";
            for (int i = 3; i >= 0; --i)
                std::cout << data[i];
            std::cout << "\n";

            auto codeword = code.encode(data);
            std::cout << "Encoded codeword:  ";
            for (int i = 6; i >= 0; --i)
                std::cout << codeword[i];
            std::cout << "\n";

            // Simulate single-bit error
            auto received = codeword;
            received.flip(2); // Flip bit at position 2
            std::cout << "Received (1 error): ";
            for (int i = 6; i >= 0; --i)
                std::cout << received[i];
            std::cout << "\n";

            auto result = code.decode_with_detection(received);
            std::cout << "Decoded data:      ";
            for (int i = 3; i >= 0; --i)
                std::cout << result.data[i];
            std::cout << "\n";

            if (result.error_detected)
            {
                std::cout << "✓ Error detected and corrected at position " << result.error_position << "\n";
            }
        }

        void demo_reed_solomon()
        {
            std::cout << "Reed-Solomon RS(255,223) Code:\n";
            std::cout << "- Symbols: 8-bit bytes\n";
            std::cout << "- Data symbols: 223\n";
            std::cout << "- Parity symbols: 32\n";
            std::cout << "- Error correction: up to 16 symbol errors\n";
            std::cout << "- Burst error tolerance: excellent\n";
            std::cout << "- Applications: storage, communication systems\n";
        }

        std::tuple<double, double, double> parse_range(const std::string &range_str)
        {
            double min_val = 0.0, max_val = 10.0, step = 1.0;

            size_t pos1 = range_str.find(':');
            if (pos1 != std::string::npos)
            {
                min_val = std::stod(range_str.substr(0, pos1));

                size_t pos2 = range_str.find(':', pos1 + 1);
                if (pos2 != std::string::npos)
                {
                    max_val = std::stod(range_str.substr(pos1 + 1, pos2 - pos1 - 1));
                    step = std::stod(range_str.substr(pos2 + 1));
                }
                else
                {
                    max_val = std::stod(range_str.substr(pos1 + 1));
                }
            }
            else
            {
                min_val = max_val = std::stod(range_str);
            }

            return {min_val, max_val, step};
        }
    };

} // namespace ecc

int main(int argc, char *argv[])
{
    ecc::CLI cli;
    cli.run(argc, argv);
    return 0;
}
