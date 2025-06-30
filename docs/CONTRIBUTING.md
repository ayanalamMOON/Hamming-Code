# Contributing Guidelines

## Getting Started

### Prerequisites
1. Fork the repository on GitHub
2. Clone your fork locally
3. Set up the development environment
4. Read the documentation in `docs/`

### Development Setup
```bash
# Clone your fork
git clone https://github.com/yourusername/Hamming-Code.git
cd Hamming-Code

# Add upstream remote
git remote add upstream https://github.com/original/Hamming-Code.git

# Install development dependencies
# See docs/BUILD_INSTRUCTIONS.md
```

## Code Standards

### C++ Style Guidelines
- Follow C++20 standards
- Use meaningful variable and function names
- Add comments for complex algorithms
- Include header guards in all header files
- Use template metaprogramming where appropriate

### Code Formatting
```cpp
// Use consistent indentation (4 spaces)
class ExampleClass {
public:
    void function_name(int parameter) {
        if (condition) {
            // Code here
        }
    }

private:
    int member_variable_;
};
```

### Naming Conventions
- **Classes**: PascalCase (`HammingCode`, `GaloisField`)
- **Functions**: snake_case (`encode_data`, `calculate_syndrome`)
- **Variables**: snake_case (`code_length`, `error_count`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_ERRORS`, `FIELD_SIZE`)
- **Template parameters**: lowercase (`m`, `n`, `k`)

## Contribution Workflow

### 1. Create Feature Branch
```bash
# Update main branch
git checkout main
git pull upstream main

# Create feature branch
git checkout -b feature/your-feature-name
```

### 2. Make Changes
- Write code following style guidelines
- Add tests for new functionality
- Update documentation if needed
- Ensure all tests pass

### 3. Commit Changes
```bash
# Stage changes
git add .

# Commit with descriptive message
git commit -m "Add BCH decoder optimization

- Implement faster syndrome calculation
- Add performance benchmarks
- Update API documentation"
```

### 4. Push and Create PR
```bash
# Push to your fork
git push origin feature/your-feature-name

# Create pull request on GitHub
# Include description of changes and testing done
```

## Types of Contributions

### Bug Fixes
- Search existing issues first
- Create issue if none exists
- Include minimal reproduction case
- Add regression test

### New Features
- Discuss in issue first for large changes
- Follow existing code patterns
- Add comprehensive tests
- Update documentation

### Documentation
- Fix typos and clarify explanations
- Add examples and use cases
- Update API references
- Improve build instructions

### Performance Improvements
- Include benchmarks showing improvement
- Ensure correctness is maintained
- Add performance tests
- Document algorithmic changes

## Testing Requirements

### Unit Tests
```cpp
// Add tests in test/ directory
#include "../include/ecc/hamming_code.hpp"
#include <cassert>

void test_hamming_7_4_encoding() {
    ecc::HammingCode<7, 4> code;
    auto encoded = code.encode(0b1010);
    assert(encoded.to_ulong() == expected_value);
}
```

### Integration Tests
- Test complete encode/decode cycles
- Test error correction capabilities
- Test performance under various conditions

### Benchmark Tests
- Include performance comparisons
- Test scalability with different parameters
- Measure memory usage

## Code Review Process

### What Reviewers Look For
- Correctness of implementation
- Code clarity and maintainability
- Adequate test coverage
- Performance implications
- Documentation updates

### Addressing Feedback
- Respond to all review comments
- Make requested changes promptly
- Ask for clarification if needed
- Update PR description if scope changes

## Release Process

### Version Numbering
- Follow semantic versioning (MAJOR.MINOR.PATCH)
- MAJOR: Breaking API changes
- MINOR: New features, backward compatible
- PATCH: Bug fixes, backward compatible

### Changelog
- Update CHANGELOG.md for all changes
- Include migration notes for breaking changes
- Credit contributors

## Getting Help

### Documentation
- Read `docs/` directory thoroughly
- Check API reference for usage examples
- Review existing code for patterns

### Communication
- Create GitHub issues for questions
- Tag maintainers for urgent issues
- Be respectful and constructive

### Common Issues
- Build problems: See `docs/BUILD_INSTRUCTIONS.md`
- API usage: See `docs/API_REFERENCE.md`
- Examples: Check `examples/` directory

## Recognition

Contributors will be:
- Listed in CONTRIBUTORS.md
- Credited in release notes
- Mentioned in documentation updates

Thank you for contributing to the Hamming Code project!
