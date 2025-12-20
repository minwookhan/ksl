import pytest
import numpy as np
from unittest.mock import MagicMock, patch
from src.ai import MediaPipePoseEstimator, SkeletonPoint

def test_skeleton_point_dataclass():
    point = SkeletonPoint(x=0.5, y=0.5, z=0.1, visibility=0.9)
    assert point.x == 0.5
    assert point.to_tuple() == (0.5, 0.5, 0.1)

@pytest.fixture
def dummy_image_np():
    return np.zeros((240, 320, 3), dtype=np.uint8) # RGB image

@pytest.fixture
def mock_mp_funcs():
    with patch('src.ai.mediapipe_pose_func') as mock_pose, \
         patch('src.ai.mediapipe_hand_func') as mock_hand:
        yield mock_pose, mock_hand

def test_mediapipe_pose_estimator_init(tmp_path):
    # Given
    model_path = tmp_path / "model.task"
    
    # When
    estimator = MediaPipePoseEstimator(str(model_path))
    
    # Then
    assert estimator.model_path == str(model_path)

def test_process_frame_combines_pose_and_hands(mock_mp_funcs, dummy_image_np, tmp_path):
    mock_pose, mock_hand = mock_mp_funcs
    
    # Mock Pose Data: 1 point
    # Returns [xs, ys, zs, vis]
    mock_pose.return_value = np.array([
        [0.1], # x
        [0.2], # y
        [0.3], # z
        [0.9]  # vis
    ])
    
    # Mock Hand Data: 1 point
    # Returns [xs, ys, zs, side]
    mock_hand.return_value = np.array([
        [0.4], # x
        [0.5], # y
        [0.6], # z
        [1.0]  # side (Right) -> visibility
    ])
    
    estimator = MediaPipePoseEstimator(str(tmp_path / "model.task"))
    
    # When
    result = estimator.process_frame(dummy_image_np)
    
    # Then
    assert len(result) == 2
    
    # Check Pose Point
    assert result[0].x == 0.1
    assert result[0].y == 0.2
    assert result[0].z == 0.3
    assert result[0].visibility == 0.9
    
    # Check Hand Point
    assert result[1].x == 0.4
    assert result[1].y == 0.5
    assert result[1].z == 0.6
    assert result[1].visibility == 1.0

    # Verify calls
    mock_pose.assert_called_once()
    mock_hand.assert_called_once()

def test_process_frame_handles_empty_results(mock_mp_funcs, dummy_image_np, tmp_path):
    mock_pose, mock_hand = mock_mp_funcs
    
    # Mock Empty/Invalid Pose Data
    mock_pose.return_value = np.array([])
    
    # Mock Empty/Invalid Hand Data
    mock_hand.return_value = np.array([])
    
    estimator = MediaPipePoseEstimator(str(tmp_path / "model.task"))
    
    # When
    result = estimator.process_frame(dummy_image_np)
    
    # Then
    assert len(result) == 0