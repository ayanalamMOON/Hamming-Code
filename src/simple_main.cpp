#include "ecc/hamming_code.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

namespace ecc
{

    /// Simple demo class for the ECC library
    class SimpleDemo
    {
    public:
        void run()
        {
            std::cout << "=== Advanced Error Correction Codes Demo ===\n\n";
            demo_hamming_basic();
            demo_hamming_error_correction();
            demo_hamming_secded();
        }

    private:
        void demo_hamming_basic()
        {
            std::cout << "1. Hamming(7,4) Basic Demo:\n";
            std::cout << std::string(30, '-') << "\n";

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

            auto decoded = code.decode(codeword);
            std::cout << "Decoded data:      ";
            for (int i = 3; i >= 0; --i)
                std::cout << decoded[i];
            std::cout << "\n";

            std::cout << "Code parameters:\n";
            std::cout << "- Code length (n): " << code.code_length << "\n";
            std::cout << "- Data length (k): " << code.data_length << "\n";
            std::cout << "- Code rate: " << code.get_code_rate() << "\n";
            std::cout << "- Min distance: " << code.get_min_distance() << "\n\n";
        }

        void demo_hamming_error_correction()
        {
            std::cout << "2. Error Correction Demo:\n";
            std::cout << std::string(25, '-') << "\n";

            Hamming_7_4 code;
            std::bitset<4> data("1010");

            auto codeword = code.encode(data);
            std::cout << "Original codeword: ";
            for (int i = 6; i >= 0; --i)
                std::cout << codeword[i];
            std::cout << "\n";

            // Introduce single-bit error
            auto received = codeword;
            received.flip(2); // Flip bit at position 2
            std::cout << "Received (error):  ";
            for (int i = 6; i >= 0; --i)
                std::cout << received[i];
            std::cout << " (error at position 2)\n";

            auto result = code.decode_with_detection(received);
            std::cout << "Decoded data:      ";
            for (int i = 3; i >= 0; --i)
                std::cout << result.data[i];
            std::cout << "\n";

            if (result.error_detected)
            {
                std::cout << "✓ Error detected and corrected at position " << result.error_position << "\n";
            }

            std::cout << "\n";
        }

        void demo_hamming_secded()
        {
            std::cout << "3. Extended Hamming(15,11) Demo:\n";
            std::cout << std::string(35, '-') << "\n";

            Hamming_15_11 code;
            std::bitset<11> data("10110100101");

            std::cout << "Original data:     ";
            for (int i = 10; i >= 0; --i)
                std::cout << data[i];
            std::cout << "\n";

            auto codeword = code.encode(data);
            std::cout << "Encoded codeword:  ";
            for (int i = 14; i >= 0; --i)
                std::cout << codeword[i];
            std::cout << "\n";

            // Test single error
            auto received = codeword;
            received.flip(5);
            auto result = code.decode_with_detection(received);

            std::cout << "\nSingle error test:\n";
            std::cout << "Received:          ";
            for (int i = 14; i >= 0; --i)
                std::cout << received[i];
            std::cout << " (error at position 5)\n";

            if (result.error_detected)
            {
                std::cout << "✓ Error detected and corrected at position " << result.error_position << "\n";
            }

            std::cout << "Decoded data:      ";
            for (int i = 10; i >= 0; --i)
                std::cout << result.data[i];
            std::cout << "\n";

            std::cout << "\nCode parameters:\n";
            std::cout << "- Code length (n): " << code.code_length << "\n";
            std::cout << "- Data length (k): " << code.data_length << "\n";
            std::cout << "- Code rate: " << std::fixed << std::setprecision(3) << code.get_code_rate() << "\n";
            std::cout << "- Error correction capability: " << code.get_error_correction_capability() << " bit(s)\n";

            std::cout << "\n";
        }
    };

} // namespace ecc

int main(int argc, char *argv[])
{
    if (argc > 1 && std::string(argv[1]) == "demo")
    {
        ecc::SimpleDemo demo;
        demo.run();
        return 0;
    }

    std::cout << "Advanced Error Correction Codes Library\n";
    std::cout << "Usage: " << argv[0] << " demo\n";
    std::cout << "\nAvailable codes:\n";
    std::cout << "- Hamming(7,4): Single error correction\n";
    std::cout << "- Hamming(15,11): Extended single error correction\n";
    std::cout << "- SECDED: Single error correction, double error detection\n";
    std::cout << "- Reed-Solomon: Powerful symbol-based error correction\n";

    return 0;
}
