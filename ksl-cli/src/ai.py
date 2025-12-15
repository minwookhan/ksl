import numpy as np
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from typing import List, Dict, Any, Optional, Tuple
from dataclasses import dataclass, field

@dataclass(frozen=True)
class SkeletonPoint:
    """단일 골격 포인트 (x, y, z) 및 visibility"""
    x: float
    y: float
    z: float
    visibility: Optional[float] = None # MediaPipe는 visibility도 제공

    def to_tuple(self) -> Tuple[float, float, float]:
        return (self.x, self.y, self.z)

class MediaPipePoseEstimator:
    def __init__(self, model_path: str):
        self.model_path = model_path
        self.base_options = python.BaseOptions(model_asset_path=self.model_path)
        self.options = vision.PoseLandmarkerOptions(base_options=self.base_options,
                                                    output_segmentation_masks=False)
        self.landmarker: Optional[vision.PoseLandmarker] = None
        self._initialize_landmarker()

    def _initialize_landmarker(self):
        """MediaPipe PoseLandmarker를 초기화합니다."""
        self.landmarker = vision.PoseLandmarker.create_from_options(self.options)

    def process_frame(self, image_np: np.ndarray) -> List[SkeletonPoint]:
        """
        전처리된 이미지 프레임(RGB numpy array)을 받아 골격 좌표를 추론합니다.
        Input: image_np (np.ndarray, RGB)
        Output: List[SkeletonPoint] (2D/3D Pose Landmarker Results)
        """
        if self.landmarker is None:
            raise RuntimeError("MediaPipe PoseLandmarker not initialized.")

        # numpy array -> MediaPipe Image
        # MediaPipe expects RGB image_format
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=image_np)
        
        # 추론 수행
        detection_result = self.landmarker.detect(mp_image)
        
        # 결과 파싱
        return self._parse_landmarks(detection_result)

    def _parse_landmarks(self, result: vision.PoseLandmarkerResult) -> List[SkeletonPoint]:
        """
        MediaPipe 추론 결과를 SkeletonPoint 리스트로 파싱합니다.
        """
        all_landmarks: List[SkeletonPoint] = []
        if result.pose_landmarks:
            # 단일 인물 가정 (첫 번째 인물의 랜드마크만 사용)
            for landmark in result.pose_landmarks[0]:
                all_landmarks.append(SkeletonPoint(
                    x=landmark.x,
                    y=landmark.y,
                    z=landmark.z,
                    visibility=getattr(landmark, 'visibility', None) # visibility는 없을 수도 있음
                ))
        return all_landmarks
