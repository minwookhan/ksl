import pytest
import numpy as np
from unittest.mock import MagicMock, patch
from pathlib import Path
from src.ai import MediaPipePoseEstimator, SkeletonPoint
from mediapipe.tasks.python import vision
import mediapipe as mp

def test_skeleton_point_dataclass():
    point = SkeletonPoint(x=0.5, y=0.5, z=0.1, visibility=0.9)
    assert point.x == 0.5
    assert point.to_tuple() == (0.5, 0.5, 0.1)

@pytest.fixture
def mock_pose_landmarker_creation():
    with patch('mediapipe.tasks.python.vision.PoseLandmarker.create_from_options') as mock_create:
        yield mock_create

@pytest.fixture
def dummy_image_np():
    return np.zeros((240, 320, 3), dtype=np.uint8) # RGB image

def test_mediapipe_pose_estimator_init(mock_pose_landmarker_creation, tmp_path):
    # Given
    model_path = tmp_path / "pose_landmarker_full.task"
    model_path.touch() # Create dummy model file

    # When
    estimator = MediaPipePoseEstimator(str(model_path))

    # Then
    mock_pose_landmarker_creation.assert_called_once() # Ensure create_from_options was called
    assert estimator.landmarker is not None

def test_process_frame_dummy_image(mock_pose_landmarker_creation, dummy_image_np, tmp_path):
    # Given
    model_path = tmp_path / "pose_landmarker_full.task"
    model_path.touch()

    # Mock the PoseLandmarker instance and its detect method
    mock_landmarker_instance = MagicMock()
    mock_pose_landmarker_creation.return_value = mock_landmarker_instance
    
    # Create a dummy result for detect()
    mock_landmark = mp.tasks.vision.PoseLandmarkerResult(
        pose_landmarks=[
            [mp.tasks.components.containers.Landmark(x=0.1, y=0.2, z=0.3, visibility=0.9)]
        ],
        pose_world_landmarks=[] # 필수 인자이므로 빈 리스트로 제공
    )
    mock_landmarker_instance.detect.return_value = mock_landmark

    estimator = MediaPipePoseEstimator(str(model_path))

    # When
    result_skeleton = estimator.process_frame(dummy_image_np)

    # Then
    mock_landmarker_instance.detect.assert_called_once() # Ensure detect was called
    assert isinstance(result_skeleton, list)
    assert len(result_skeleton) == 1
    assert isinstance(result_skeleton[0], SkeletonPoint)
    assert result_skeleton[0].x == 0.1
    assert result_skeleton[0].y == 0.2
    assert result_skeleton[0].z == 0.3
    assert result_skeleton[0].visibility == 0.9

def test_process_frame_no_landmarks(mock_pose_landmarker_creation, dummy_image_np, tmp_path):
    # Given
    model_path = tmp_path / "pose_landmarker_full.task"
    model_path.touch()

    mock_landmarker_instance = MagicMock()
    mock_pose_landmarker_creation.return_value = mock_landmarker_instance
    
    # Simulate no landmarks detected
    mock_landmark_no_result = mp.tasks.vision.PoseLandmarkerResult(
        pose_landmarks=[],
        pose_world_landmarks=[]
    )
    mock_landmarker_instance.detect.return_value = mock_landmark_no_result

    estimator = MediaPipePoseEstimator(str(model_path))

    # When
    result_skeleton = estimator.process_frame(dummy_image_np)

    # Then
    assert isinstance(result_skeleton, list)
    assert len(result_skeleton) == 0
