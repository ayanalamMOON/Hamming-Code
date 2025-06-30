#include <iostream>
#include <bitset>

// Simple test to verify basic compilation
int main()
{
    std::cout << "Hello, World!" << std::endl;
    std::bitset<7> test("1010101");
    std::cout << "Test bitset: " << test << std::endl;
    return 0;
}
