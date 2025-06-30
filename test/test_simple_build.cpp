#include <iostream>
#include <bitset>

namespace ecc
{

    // Simplified Hamming(7,4) implementation to test basic linking
    class SimpleHamming
    {
    public:
        std::bitset<7> encode(const std::bitset<4> &data)
        {
            std::bitset<7> codeword;

            // Copy data bits to positions 3, 5, 6, 7 (1-indexed)
            codeword[2] = data[0]; // d1 -> position 3
            codeword[4] = data[1]; // d2 -> position 5
            codeword[5] = data[2]; // d3 -> position 6
            codeword[6] = data[3]; // d4 -> position 7

            // Calculate parity bits
            codeword[0] = data[0] ^ data[1] ^ data[3]; // p1 = d1 ^ d2 ^ d4
            codeword[1] = data[0] ^ data[2] ^ data[3]; // p2 = d1 ^ d3 ^ d4
            codeword[3] = data[1] ^ data[2] ^ data[3]; // p3 = d2 ^ d3 ^ d4

            return codeword;
        }

        std::bitset<4> decode(const std::bitset<7> &received)
        {
            std::bitset<4> data;

            // Extract data bits
            data[0] = received[2];
            data[1] = received[4];
            data[2] = received[5];
            data[3] = received[6];

            return data;
        }
    };

}

int main()
{
    std::cout << "Simple ECC Test\n";

    ecc::SimpleHamming hamming;
    std::bitset<4> data("1011");

    std::cout << "Data: " << data << std::endl;

    auto codeword = hamming.encode(data);
    std::cout << "Encoded: " << codeword << std::endl;

    auto decoded = hamming.decode(codeword);
    std::cout << "Decoded: " << decoded << std::endl;

    return 0;
}
