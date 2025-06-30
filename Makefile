# Advanced Error Correction Codes Makefile
# Modern C++20 build system

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O3 -march=native -static-libgcc -static-libstdc++
INCLUDES = -Iinclude
LDFLAGS = -pthread -static-libgcc -static-libstdc++

# Directories
SRCDIR = src
TESTDIR = tests
BENCHDIR = benchmarks
BUILDDIR = build
BINDIR = bin

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# Test files
TEST_SOURCES = $(wildcard $(TESTDIR)/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.cpp=$(BUILDDIR)/test_%.o)

# Benchmark files
BENCH_SOURCES = $(wildcard $(BENCHDIR)/*.cpp)
BENCH_OBJECTS = $(BENCH_SOURCES:$(BENCHDIR)/%.cpp=$(BUILDDIR)/bench_%.o)

# Targets
MAIN_TARGET = $(BINDIR)/ecc_demo
TEST_TARGET = $(BINDIR)/ecc_test
BENCH_TARGET = $(BINDIR)/ecc_benchmark
ERROR_SIM_TARGET = $(BINDIR)/error_simulation_example

# Default target
all: directories $(MAIN_TARGET) $(TEST_TARGET) $(BENCH_TARGET) $(ERROR_SIM_TARGET)

# Create directories
directories:
	@mkdir -p $(BUILDDIR) $(BINDIR)

# Main executable
$(MAIN_TARGET): $(BUILDDIR)/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Test executable
$(TEST_TARGET): $(BUILDDIR)/test_test_hamming.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Benchmark executable
$(BENCH_TARGET): $(BUILDDIR)/bench_benchmark_codes.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Error simulation example
$(ERROR_SIM_TARGET): examples/error_simulation_example.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -static $< -o $@

# BER analysis test
ber-analysis: examples/ber_analysis_test.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -static $< -o output/ber_analysis_test.exe

# Object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/test_%.o: $(TESTDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/bench_%.o: $(BENCHDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Convenience targets
demo: $(MAIN_TARGET)
	@echo "Running demo..."
	./$(MAIN_TARGET) demo

test: $(TEST_TARGET)
	@echo "Running tests..."
	./$(TEST_TARGET)

benchmark: $(BENCH_TARGET)
	@echo "Running benchmarks..."
	./$(BENCH_TARGET)

# Performance analysis
analyze:
	@echo "Analyzing Hamming code performance..."
	./$(MAIN_TARGET) analyze --code hamming --snr 0:10:1 --iterations 1000

compare:
	@echo "Comparing error correction codes..."
	./$(MAIN_TARGET) compare --snr 5 --iterations 10000

# Error simulation demo
error-sim: $(ERROR_SIM_TARGET)
	@echo "Running error simulation demo..."
	./$(ERROR_SIM_TARGET)

# BER analysis demo
ber-demo: ber-analysis
	@echo "Running BER analysis demo..."
	timeout 60 ./output/ber_analysis_test.exe

# Clean
clean:
	rm -rf $(BUILDDIR) $(BINDIR)

# Install (copy to system directories)
install: all
	@echo "Installing ECC library..."
	sudo cp $(MAIN_TARGET) /usr/local/bin/
	sudo cp $(TEST_TARGET) /usr/local/bin/
	sudo cp $(BENCH_TARGET) /usr/local/bin/
	sudo cp -r include/ecc /usr/local/include/

# Development targets
debug: CXXFLAGS += -g -DDEBUG
debug: all

profile: CXXFLAGS += -pg
profile: all

# Code formatting (requires clang-format)
format:
	find include src tests benchmarks -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Documentation (requires doxygen)
docs:
	doxygen Doxyfile

# Package creation
package:
	tar -czf ecc-library.tar.gz include/ src/ tests/ benchmarks/ CMakeLists.txt Makefile README.md LICENSE

# Help
help:
	@echo "Available targets:"
	@echo "  all         - Build all executables"
	@echo "  demo        - Build and run demo"
	@echo "  test        - Build and run tests"
	@echo "  benchmark   - Build and run benchmarks"
	@echo "  error-sim   - Build and run error simulation demo"
	@echo "  ber-analysis - Build BER analysis program"
	@echo "  ber-demo    - Build and run BER analysis demo"
	@echo "  analyze     - Run performance analysis"
	@echo "  compare     - Compare different codes"
	@echo "  clean       - Remove build files"
	@echo "  install     - Install to system"
	@echo "  debug       - Build with debug symbols"
	@echo "  format      - Format source code"
	@echo "  docs        - Generate documentation"
	@echo "  package     - Create distribution package"

.PHONY: all directories demo test benchmark error-sim ber-analysis ber-demo analyze compare clean install debug profile format docs package help
