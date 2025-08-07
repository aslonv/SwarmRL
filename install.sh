#!/bin/bash

set -e

echo "================================"
echo "SwarmRL Installation"
echo "================================"

if [ "$EUID" -ne 0 ]; then 
    echo "Note: Running without sudo. Installing to ~/.local"
    PREFIX="$HOME/.local"
else
    PREFIX="/usr/local"
fi

./build.sh

cd build
make install DESTDIR=$PREFIX

if [ "$EUID" -eq 0 ]; then
    ldconfig
fi

echo ""
echo "SwarmRL installed to $PREFIX"
echo "Add to your .bashrc or .zshrc:"
echo "  export LD_LIBRARY_PATH=$PREFIX/lib:\$LD_LIBRARY_PATH"
echo "  export PATH=$PREFIX/bin:\$PATH"
