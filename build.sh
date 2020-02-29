#!/bin/bash

set -euo pipefail

mkdir -p build && cd build

# Configure
cmake -DCODE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug ..
# Build (for Make on Unix equivalent to `make -j $(nproc)`)
cmake --build . --config Debug -- -j $(nproc)
# Test
ctest -j $(nproc) --output-on-failure

cd ..

# Create lcov report
# capture coverage info
lcov --directory . --capture --output-file coverage.info
# filter out system and extra files.
# To also not include test code in coverage add them with full path to the patterns: '*/tests/*'
lcov --remove coverage.info "/usr/*" "*.hpp" --output-file coverage.info
# output coverage data for debugging (optional)
lcov --list coverage.info

wget https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py

python3 lcov_cobertura.py coverage.info --base-dir src/dir --excludes test.lib --output build/coverage.xml

bash <(curl -s https://codecov.io/bash) -f coverage.info || echo "Codecov did not collect coverage reports"

rm coverage.info
rm lcov_cobertura.py