import pytest
from pathlib import Path
from src.config import AppConfig

def test_app_config_valid(tmp_path):
    # Given
    dummy_video = tmp_path / "test.mp4"
    dummy_video.touch()
    roi = (100, 100, 320, 240)
    
    # When
    config = AppConfig(video_path=dummy_video, roi=roi)
    
    # Then
    assert config.video_path == dummy_video
    assert config.roi == (100, 100, 320, 240)

def test_app_config_file_not_found():
    # Given
    invalid_path = Path("non_existent.mp4")
    roi = (0, 0, 100, 100)
    
    # When/Then
    with pytest.raises(FileNotFoundError):
        AppConfig(video_path=invalid_path, roi=roi)

def test_app_config_invalid_roi(tmp_path):
    # Given
    dummy_video = tmp_path / "test.mp4"
    dummy_video.touch()
    invalid_roi = (-10, 0, 100, 100)
    
    # When/Then
    with pytest.raises(ValueError):
        AppConfig(video_path=dummy_video, roi=invalid_roi)
