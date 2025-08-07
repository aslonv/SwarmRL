# Getting Started with SwarmRL

Welcome to SwarmRL! This guide will help you get up and running with multi-agent reinforcement learning.

## Table of Contents
1. [Installation](#installation)
2. [Quick Start](#quick-start)
3. [Understanding the Architecture](#understanding-the-architecture)
4. [Creating Your First Swarm](#creating-your-first-swarm)
5. [Customizing Agents](#customizing-agents)
6. [Visualization](#visualization)
7. [Troubleshooting](#troubleshooting)

## Installation

### Prerequisites
- GCC 7+ or Clang 6+
- CMake 3.10+
- POSIX-compliant OS (Linux, macOS)
- Python 3.7+ (optional, for Python bindings)

### Building from Source

```bash
git clone https://github.com/aslonv/swarmrl.git
cd swarmrl

chmod +x build.sh
./build.sh

mkdir build && cd build
cmake ..
make -j4
```

### Docker Installation
```bash
docker build -t swarmrl .
docker run -it swarmrl

docker-compose up
```
