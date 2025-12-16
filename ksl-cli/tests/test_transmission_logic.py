import pytest
import numpy as np
import cv2
import sys
import os

# 경로 설정
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../src')))

from src.network import encode_frame

def test_frame_type_compatibility_with_opencv():
    """
    gRPC로 전송되는 Frame 메시지의 type 필드가
    C++ OpenCV 서버가 이해할 수 있는 상수 값(CV_8UC3 등)과 일치하는지 테스트합니다.
    """
    # 3채널 컬러 이미지 (BGR)
    color_img = np.zeros((100, 100, 3), dtype=np.uint8)
    frame_color = encode_frame("test", 1, 0, color_img, [])
    
    # Python cv2.CV_8UC3의 값은 16입니다.
    assert frame_color.type == cv2.CV_8UC3, \
        f"Expected CV_8UC3 ({cv2.CV_8UC3}), but got {frame_color.type}"

    # 1채널 그레이스케일 이미지
    gray_img = np.zeros((100, 100), dtype=np.uint8)
    frame_gray = encode_frame("test", 2, 0, gray_img, [])
    
    # Python cv2.CV_8UC1의 값은 0입니다.
    assert frame_gray.type == cv2.CV_8UC1, \
        f"Expected CV_8UC1 ({cv2.CV_8UC1}), but got {frame_gray.type}"

def test_frame_type_compatibility_float32():
    """
    float32 이미지 타입에 대한 OpenCV 상수 매핑 테스트
    """
    # 3채널 float32 이미지
    float_img = np.zeros((100, 100, 3), dtype=np.float32)
    frame_float = encode_frame("test", 3, 0, float_img, [])
    
    # Python cv2.CV_32FC3의 값은 21입니다.
    assert frame_float.type == cv2.CV_32FC3, \
        f"Expected CV_32FC3 ({cv2.CV_32FC3}), but got {frame_float.type}"
