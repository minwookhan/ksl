# KSL Client (C++ Port)

This project is a C++ port of the KSL gRPC client, removing MFC dependencies and designed for CLI usage on Linux (and Windows).

## Prerequisites

To build and run this project, you need both **System Libraries** (for C++ compilation) and a **Python Virtual Environment** (for MediaPipe inference via pybind11).

### 1. Install System Dependencies (Ubuntu/Debian)

Use the provided helper script or install manually:

```bash
# Helper Script
chmod +x install_deps.sh
./install_deps.sh
```

**Manual Installation:**
```bash
sudo apt update
sudo apt install -y build-essential cmake \
    libopencv-dev \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc \
    python3-dev python3-pip
```

*Note: `pybind11` must be installed as a system package (`sudo apt install pybind11-dev`) or via pip for the system python.*

### 2. Setup Python Virtual Environment (Important)

The C++ binary embeds Python to run MediaPipe. It requires `numpy` and `mediapipe` to be available.
To avoid conflicts with system libraries (especially Qt/GUI), use `opencv-python-headless` in the virtual environment.

```bash
# Create venv (e.g., in parent directory or ~/.venvs)
python3 -m venv .venv
source .venv/bin/activate

# Install required packages
pip install --upgrade pip
pip install numpy mediapipe protobuf grpcio
# Use headless opencv to prevent Qt conflicts with system OpenCV
pip install opencv-python-headless
```

## Build

Configure CMake to use the System Python for linking (to ensure compatibility with system libraries), but we will point it to the venv packages at runtime.

```bash
mkdir build && cd build

# Clean previous cache if needed
rm -rf *

# Configure (Force using system python executable to link against libpython correctly)
cmake .. -DPython3_EXECUTABLE=/usr/bin/python3

# Build
make -j$(nproc)
```

## Run

Since the binary is linked against System Libraries but needs Python packages from the Virtual Environment, you **must** set specific environment variables (`PYTHONPATH`, `PYTHONHOME`) and clear conflicting ones (`LD_LIBRARY_PATH`).

**We recommend creating a wrapper script `run_ksl.sh` in the build directory:**

```bash
#!/bin/bash
# run_ksl.sh

# 1. Unset conflicting library paths (e.g. from Anaconda)
unset LD_LIBRARY_PATH
unset PYTHONHOME
unset PYTHONPATH

# 2. Set path to your Virtual Environment
VENV_PATH="/home/minux/work/ksl/.venv"  # <--- UPDATE THIS PATH

# 3. Force embedded Python to load packages from Venv
export PYTHONPATH="$VENV_PATH/lib/python3.12/site-packages"

# 4. Run
./ksl-cli-cpp "$@"
```

**Execution:**

```bash
chmod +x run_ksl.sh

# Basic Run
./run_ksl.sh ../../5.mp4 "100,100,640,480" --debugFile

# Headless Mode (No GUI)
./run_ksl.sh ../../5.mp4 "100,100,640,480" --no-gui

# Frame Range
./run_ksl.sh ../../5.mp4 "100,100,640,480" --frame 100 500
```

### Options

*   `<video_path>`: Path to input video file.
*   `<roi>`: Region of Interest "x,y,w,h".
*   `--server <addr>`: gRPC server address (default: localhost:50051).
*   `--no-gui`: Disable OpenCV window (useful for servers).
*   `--debugFile`: Save analysis data to `.csv`.
*   `--frame <start> <end>`: Process specific frame range.

