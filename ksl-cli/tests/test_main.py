import pytest
import sys
import os
import argparse
from unittest.mock import MagicMock, patch
from pathlib import Path
import logging

# Ensure src path is available
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../src')))

# Import from src.main directly if possible, or we might need to import main functionality
# Since main.py is a script, it's better to structure it so functions can be imported.
# Assuming the main logic is encapsulated in functions in src/main.py as proposed.
from src.main import main, process_video_stream, setup_logging
from src.config import AppConfig
from src.ai import SkeletonPoint
from ksl_sentence_recognition_pb2 import SubmitResultResponse

# --- Test setup_logging ---
def test_setup_logging():
    with patch('logging.getLogger') as mock_get_logger:
        setup_logging(logging.DEBUG)
        # Check if root logger level was set
        mock_get_logger.return_value.setLevel.assert_any_call(logging.DEBUG)
        # Check if grpc/mediapipe loggers were silenced
        assert mock_get_logger.call_count >= 3 

# --- Test process_video_stream (Pipeline Integration) ---
@patch('src.main.encode_frame')
def test_process_video_stream(mock_encode_frame):
    # Setup Mocks
    mock_config = MagicMock(spec=AppConfig)
    mock_config.video_path = Path("dummy_video.mp4")
    
    mock_video_loader = MagicMock()
    # Simulate 2 frames
    mock_video_loader.get_frames.return_value = iter([MagicMock(), MagicMock()])
    
    mock_pose_estimator = MagicMock()
    mock_pose_estimator.process_frame.return_value = [SkeletonPoint(x=0.1, y=0.1, z=0.1)] * 33
    
    mock_grpc_client = MagicMock()
    # Mock send_stream to consume the iterator and return a response iterator
    def side_effect_send_stream(iterator):
        list(iterator) # Consume generator to trigger internal logic
        return iter([SubmitResultResponse(session_id="test", frame_count=2, message="Done")])
    mock_grpc_client.send_stream.side_effect = side_effect_send_stream

    # Run Pipeline
    process_video_stream(mock_config, mock_video_loader, mock_pose_estimator, mock_grpc_client)

    # Verifications
    assert mock_video_loader.get_frames.called
    assert mock_pose_estimator.process_frame.call_count == 2
    assert mock_encode_frame.call_count == 2
    mock_grpc_client.send_stream.assert_called_once()
    mock_grpc_client.close.assert_called_once()

# --- Test main function (CLI Arguments & Orchestration) ---
@patch('src.main.process_video_stream')
@patch('src.main.GrpcClient')
@patch('src.main.MediaPipePoseEstimator')
@patch('src.main.VideoLoader')
@patch('src.main.AppConfig')
@patch('argparse.ArgumentParser.parse_args')
def test_main_success(mock_parse_args, mock_app_config, mock_video_loader, 
                      mock_pose_estimator, mock_grpc_client, mock_process_stream):
    
    # Mock CLI arguments
    mock_args = MagicMock()
    mock_args.video_path = Path("test.mp4")
    mock_args.roi = "0,0,100,100"
    mock_args.server = "localhost:50051"
    mock_args.model_path = Path("model.task")
    mock_args.output = None
    mock_args.log_level = "INFO"
    mock_parse_args.return_value = mock_args

    # Mock component initialization
    mock_app_config.return_value = MagicMock()

    # Run main
    with patch('src.main.setup_logging') as mock_setup_logging:
        main()

    # Verifications
    mock_app_config.assert_called_once()
    mock_video_loader.assert_called_once()
    mock_pose_estimator.assert_called_once_with(str(mock_args.model_path))
    mock_grpc_client.assert_called_once_with(mock_args.server)
    mock_process_stream.assert_called_once()

@patch('argparse.ArgumentParser.parse_args')
def test_main_invalid_roi(mock_parse_args):
    # Mock CLI arguments with invalid ROI
    mock_args = MagicMock()
    mock_args.video_path = Path("test.mp4")
    mock_args.roi = "invalid_roi"
    mock_args.log_level = "INFO"
    mock_parse_args.return_value = mock_args

    # Run main and expect exit
    with patch('sys.exit') as mock_exit:
        main()
        mock_exit.assert_called_with(1)

@patch('argparse.ArgumentParser.parse_args')
@patch('src.main.AppConfig')
def test_main_file_not_found(mock_app_config, mock_parse_args):
    # Mock CLI arguments
    mock_args = MagicMock()
    mock_args.video_path = Path("non_existent.mp4")
    mock_args.roi = "0,0,100,100"
    mock_args.log_level = "INFO"
    mock_parse_args.return_value = mock_args

    # AppConfig raises FileNotFoundError
    mock_app_config.side_effect = FileNotFoundError("Video not found")

    # Run main and expect exit
    with patch('sys.exit') as mock_exit:
        main()
        mock_exit.assert_called_with(1)
