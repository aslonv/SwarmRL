#!/bin/bash
# SwarmRL Build Script

set -e

echo "================================"
echo "SwarmRL Build System"
echo "================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}✗ $1 not found${NC}"
        echo "Please install $1 and try again"
        exit 1
    else
        echo -e "${GREEN}✓ $1 found${NC}"
    fi
}

check_command gcc
check_command cmake
check_command make
check_command git

# Create build directory
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

echo -e "\n${YELLOW}Configuring with CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

echo -e "\n${YELLOW}Building SwarmRL...${NC}"
make -j$(nproc)

echo -e "\n${GREEN}Build successful!${NC}"

if [ -f "tests/run_tests" ]; then
    echo -e "\n${YELLOW}Running tests...${NC}"
    ./tests/run_tests
fi

echo -e "\n${GREEN}SwarmRL built successfully!${NC}"
echo "Run examples with:"
echo "  ./build/simple_swarm"
echo ""
echo "Start master server with:"
echo "  ./build/master"
echo ""
echo "Start worker with:"
echo "  ./build/worker [master_ip]"
