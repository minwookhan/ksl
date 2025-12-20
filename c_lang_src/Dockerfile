FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libopencv-dev \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    python3-dev \
    python3-pip \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

# Install pybind11
RUN pip3 install pybind11

# Set working directory
WORKDIR /app

# Copy source
COPY . .

# Build
RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Default command
CMD ["./build/ksl-cli-cpp"]
