#!/bin/bash

# KSL CLI Test Script

VIDEO_PATH="./5.mp4"
ROI="700,400,211,206"
MODEL_PATH="./ksl-cli/protos/pose_landmarker_full.task"
SERVER_ADDR="192.168.1.10:50051"

echo "=== Scenario 1: Keyframe Extraction (Local Save, No gRPC) ==="
python ksl-cli/src/main.py "$VIDEO_PATH" "$ROI" \
    --frame 560 669 \
    --keyframeOnly 1 \
    --no-gui \
    --model-path "$MODEL_PATH" \
    --log-level INFO

echo -e "\n=== Scenario 2: Full gRPC Stream (Offline Simulation) ==="
# 서버가 없는 경우에도 인코딩 로직을 테스트하기 위해 --keyframeOnly 2 사용
python ksl-cli/src/main.py "$VIDEO_PATH" "$ROI" \
    --frame 560 669 \
    --keyframeOnly 2 \
    --no-gui \
    --model-path "$MODEL_PATH" \
    --log-level INFO

echo -e "\n=== Scenario 3: Debug Mode (Save Motion Values to CSV) ==="
python ksl-cli/src/main.py "$VIDEO_PATH" "$ROI" \
    --frame 560 600 \
    --debugFile \
    --keyframeOnly 1 \
    --no-gui \
    --model-path "$MODEL_PATH" \
    --log-level DEBUG

echo -e "\n=== Scenario 4: Full Stream to Server (Actual gRPC) ==="
echo "# To run this, ensure the server is active at $SERVER_ADDR"
# python ksl-cli/src/main.py "$VIDEO_PATH" "$ROI" --server "$SERVER_ADDR" --model-path "$MODEL_PATH"
