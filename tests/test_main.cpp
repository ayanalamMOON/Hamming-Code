#include <iostream>

// Main test runner
namespace ecc::test
{
    void test_reed_solomon();
    void test_bch();
    void test_performance();
}

int main()
{
    std::cout << "Running additional test suites...\n";

    ecc::test::test_reed_solomon();
    ecc::test::test_bch();
    ecc::test::test_performance();

    return 0;
}
