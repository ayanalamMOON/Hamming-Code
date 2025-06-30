#include "benchmarks/ber_analysis.cpp"
#include <iostream>

int main()
{
    std::cout << "Advanced BER Analysis Suite\n";
    std::cout << "Using C++20 with concepts and modern features\n";
    std::cout << std::string(60, '=') << "\n\n";

    try
    {
        // Run the comprehensive BER analysis
        ecc::benchmark::analyze_ber_curves();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error during BER analysis: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
