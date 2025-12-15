import pytest
import numpy as np
import cv2
from unittest.mock import MagicMock, patch
from src.video import parse_roi, VideoLoader
from src.config import AppConfig

def test_parse_roi_valid():
    assert parse_roi("10,20,300,400") == (10, 20, 300, 400)
    assert parse_roi(" 0, 0, 100, 100 ") == (0, 0, 100, 100)

def test_parse_roi_invalid():
    with pytest.raises(ValueError):
        parse_roi("10,20,300")  # 3 elements
    with pytest.raises(ValueError):
        parse_roi("invalid,num,ber,s")

class TestVideoLoader:
    @pytest.fixture
    def mock_config(self, tmp_path):
        dummy_video = tmp_path / "dummy.mp4"
        dummy_video.touch()
        # ROI: x=10, y=10, w=100, h=100
        return AppConfig(video_path=dummy_video, roi=(10, 10, 100, 100))

    def test_preprocess_crop_and_resize(self, mock_config):
        loader = VideoLoader(mock_config)
        
        # 400x400 White Image
        original_frame = np.ones((400, 400, 3), dtype=np.uint8) * 255
        
        # ROI 영역 (10,10,100,100) -> 100x100
        # C++ 로직: if width >= 320 resize. 여기선 100이므로 resize 안 함.
        # 강제로 Resize 테스트를 위해 ROI를 크게 잡아봅니다.
        
        processed = loader._preprocess(original_frame)
        
        # Expected: ROI (100x100) 그대로 유지 (width < 320)
        assert processed.shape == (100, 100, 3)
    
    def test_preprocess_resize_logic(self, tmp_path):
        # width가 320 이상인 경우 테스트
        dummy_video = tmp_path / "dummy.mp4"
        dummy_video.touch()
        
        # ROI: 640x480
        large_roi_config = AppConfig(video_path=dummy_video, roi=(0, 0, 640, 480))
        loader = VideoLoader(large_roi_config)
        
        original_frame = np.zeros((480, 640, 3), dtype=np.uint8)
        processed = loader._preprocess(original_frame)
        
        # C++ Logic: target_x = 320.0, scale = 320 / 640 = 0.5
        # dst_width = 320, dst_height = 480 * 0.5 = 240
        assert processed.shape[1] == 320
        assert processed.shape[0] == 240

    @patch('cv2.VideoCapture')
    def test_get_frames_yields_data(self, mock_cap_cls, mock_config):
        # Mock VideoCapture
        mock_cap = MagicMock()
        mock_cap_cls.return_value = mock_cap
        mock_cap.isOpened.return_value = True
        
        # 1st call: success, returns frame
        fake_frame = np.zeros((400, 400, 3), dtype=np.uint8)
        # 2nd call: fail (end of video), returns False, None
        mock_cap.read.side_effect = [(True, fake_frame), (False, None)]
        
        loader = VideoLoader(mock_config)
        frames = list(loader.get_frames())
        
        assert len(frames) == 1
        assert frames[0].shape == (100, 100, 3) # ROI applied
        mock_cap.release.assert_called_once()
