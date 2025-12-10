#!/bin/bash

# Path to your compiler executable
COMPILER="./c99_compiler"

# Directory containing the test files
TEST_DIR="./tests"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "--- Running Compiler Test Suite ---"

# First, build the compiler
make clean
make

if [ ! -f "$COMPILER" ]; then
    echo -e "${RED}Compiler executable not found. Build failed.${NC}"
    exit 1
fi

# Run tests that are expected to pass
echo "--- Running Passing Tests ---"
for test_file in ${TEST_DIR}/test_*.c; do
    if [[ "$test_file" == *test_errors.c ]]; then
        continue # Skip the error test, it's handled below
    fi

    echo -n "Testing ${test_file}... "
    # Run the compiler and capture both stdout and stderr
    output=$($COMPILER "$test_file" "${test_file}.3ac" 2>&1)
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}PASS${NC}"
    else
        echo -e "${RED}FAIL${NC}"
        echo "Compiler output:"
        echo "$output"
    fi
done

echo ""
echo "--- Running Error-Reporting Tests ---"
ERROR_TEST="${TEST_DIR}/test_errors.c"
echo -n "Testing ${ERROR_TEST}... "

# Run the compiler on the error test, redirecting stderr to stdout
output=$($COMPILER "$ERROR_TEST" "${ERROR_TEST}.3ac" 2>&1)

# Check if the output contains the expected error messages
if echo "$output" | grep -q "Identifier 'c' is not declared" && \
   echo "$output" | grep -q "Type mismatch in assignment" && \
   echo "$output" | grep -q "Calling 'a' which is not a function"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL - Did not find all expected semantic errors.${NC}"
    echo "Compiler output:"
    echo "$output"
fi

echo "--- All tests complete ---"