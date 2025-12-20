#!/bin/bash
set -e

echo "Installing dependencies for KSL C++ Client..."

# Update package list
sudo apt-get update

# Install CMake and Build Tools
sudo apt-get install -y build-essential cmake

# Install OpenCV
sudo apt-get install -y libopencv-dev

# Install gRPC and Protobuf
sudo apt-get install -y libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc

# Install Python3 Dev
sudo apt-get install -y python3-dev python3-pip

# Install pybind11
# Try apt first, else pip
if ! sudo apt-get install -y pybind11-dev; then
    echo "pybind11-dev not found in apt, trying pip..."
    pip3 install pybind11
fi

echo "Dependencies installed."
