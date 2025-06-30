# Build Instructions

## Prerequisites

### Required Tools
- **C++ Compiler**: GCC 10+ or Clang 12+ (C++20 support required)
- **CMake**: Version 3.15 or later
- **Make**: GNU Make or equivalent
- **Git**: For version control

### Optional Tools
- **Ninja**: Fast build system (recommended)
- **OpenMP**: For parallel processing support
- **Doxygen**: For generating documentation

## Build Methods

### Method 1: CMake (Recommended)

#### Configure and Build
```bash
# Create build directory
mkdir build-cmake
cd build-cmake

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with make
make -j$(nproc)

# Or build with ninja (if available)
ninja
```

#### CMake Options
- `-DCMAKE_BUILD_TYPE=Release`: Optimized build
- `-DCMAKE_BUILD_TYPE=Debug`: Debug build with symbols
- `-DBUILD_TESTING=ON`: Enable test building (default)
- `-DUSE_OPENMP=ON`: Enable OpenMP support (default)

### Method 2: Makefile

#### Build All Targets
```bash
# Build main library and examples
make all

# Build tests
make tests

# Build benchmarks
make benchmarks

# Clean build artifacts
make clean
```

#### Makefile Targets
- `make all`: Build main targets
- `make tests`: Build test executables
- `make examples`: Build example programs
- `make benchmarks`: Build benchmark programs
- `make clean`: Remove build artifacts

## Platform-Specific Instructions

### Windows (MSYS2/MinGW)
```bash
# Install dependencies
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake

# Build with CMake
mkdir build-cmake && cd build-cmake
cmake .. -G "MinGW Makefiles"
mingw32-make
```

### Linux
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential cmake libgomp1

# Build
mkdir build-cmake && cd build-cmake
cmake ..
make -j$(nproc)
```

### macOS
```bash
# Install dependencies
brew install cmake llvm

# Build
mkdir build-cmake && cd build-cmake
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Troubleshooting

### Common Issues

#### C++20 Support
Ensure your compiler supports C++20:
```bash
# Check GCC version
g++ --version  # Should be 10+

# Check Clang version
clang++ --version  # Should be 12+
```

#### Missing OpenMP
If OpenMP is not available:
```bash
# Ubuntu/Debian
sudo apt install libomp-dev

# MSYS2
pacman -S mingw-w64-x86_64-openmp
```

#### CMake Version
Update CMake if version is too old:
```bash
# Download from https://cmake.org/download/
# Or use package manager
```

### Build Errors

#### Template Instantiation Errors
These are usually due to:
- Missing C++20 support
- Incorrect template parameters
- Missing include files

#### Linker Errors
Common solutions:
- Ensure all source files are compiled
- Check library linking order
- Verify OpenMP linking

## Verification

### Run Tests
```bash
# From build directory
ctest --verbose

# Or run individual tests
./output/test_basic
./output/test_hamming
```

### Run Examples
```bash
# Basic demonstration
./output/ecc_demo

# Performance analysis
./output/ber_analysis_test
```

### Run Benchmarks
```bash
# Code comparison benchmark
./output/ecc_benchmark
```
