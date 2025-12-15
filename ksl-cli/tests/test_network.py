import pytest
import numpy as np
import cv2
from unittest.mock import MagicMock, patch, call
import sys
import os
import copy # Import copy module for deepcopy

# Ensure src path is in sys.path to find generated modules
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../src')))

try:
    from src.ksl_sentence_recognition_pb2 import Frame, Point3, SubmitResultResponse
    from src.ksl_sentence_recognition_pb2_grpc import SequenceServiceStub
except ImportError:
     # Fallback if running from src directory context
    from ksl_sentence_recognition_pb2 import Frame, Point3, SubmitResultResponse
    from ksl_sentence_recognition_pb2_grpc import SequenceServiceStub

from src.network import GrpcClient, encode_frame
from src.ai import SkeletonPoint

# --- Test encode_frame function ---
def test_encode_frame_valid_data():
    session_id = "test_session"
    index = 123
    flag = 1 # NORMAL
    image_np = np.zeros((240, 320, 3), dtype=np.uint8) # RGB image
    skeleton_points = [
        SkeletonPoint(x=0.1, y=0.2, z=0.3),
        SkeletonPoint(x=0.4, y=0.5, z=0.6)
    ]

    frame = encode_frame(session_id, index, flag, image_np, skeleton_points)

    assert isinstance(frame, Frame)
    assert frame.session_id == session_id
    assert frame.index == index
    assert frame.flag == flag
    assert frame.width == 320
    assert frame.height == 240
    # Implementation uses cv2.CV_8UC3 (16) for 3-channel images
    assert frame.type == cv2.CV_8UC3 
    assert len(frame.data) == 240 * 320 * 3
    assert len(frame.pose_points) == 2
    # Use pytest.approx for float comparisons
    assert frame.pose_points[0].x == pytest.approx(0.1)
    assert frame.pose_points[1].z == pytest.approx(0.6)

def test_encode_frame_empty_skeleton():
    session_id = "test_session_empty"
    index = 1
    flag = 0
    image_np = np.zeros((100, 100, 3), dtype=np.uint8)
    skeleton_points = []

    frame = encode_frame(session_id, index, flag, image_np, skeleton_points)
    assert len(frame.pose_points) == 0

# --- Test GrpcClient class ---
@pytest.fixture
def mock_grpc_stub():
    # Patch the class where it is IMPORTED/USED in src.network
    with patch('src.network.pb2_grpc.SequenceServiceStub') as MockStub:
        yield MockStub # Yield the Mock Class itself so we can check instantiation calls

def test_grpc_client_init(mock_grpc_stub):
    client = GrpcClient("localhost:50051")
    assert client.stub is not None
    # Verify instantiation of the Stub class
    mock_grpc_stub.assert_called_once_with(client.channel)

def test_grpc_client_send_stream(mock_grpc_stub):
    client = GrpcClient("localhost:50051")

    # Get the mock instance created by the constructor call
    mock_stub_instance = mock_grpc_stub.return_value
    
    # Simulate a response from the server directly (as SendFrames returns the response/future)
    # Note: send_stream implementation expects SendFrames to return the response directly for this blocking call simulation
    expected_response = SubmitResultResponse(session_id="test", frame_count=2, message="OK")
    mock_stub_instance.SendFrames.return_value = expected_response

    # Create dummy frames to send
    dummy_frame1 = Frame(session_id="s1", index=1, width=10, height=10, data=b'abc')
    dummy_frame2 = Frame(session_id="s1", index=2, width=10, height=10, data=b'def')
    frames = [dummy_frame1, dummy_frame2]
    frame_iterator = iter(frames)

    response_iterator = client.send_stream(frame_iterator)
    response = next(response_iterator)

    # Verify SendFrames was called on the mock instance
    mock_stub_instance.SendFrames.assert_called_once()
    
    # Retrieve the iterator passed to SendFrames
    args, _ = mock_stub_instance.SendFrames.call_args
    passed_iterator = args[0]
    
    # Verify the iterator yields the expected frames
    # Since we passed the original iterator, we can verify it's the same object
    # OR if we want to be strict about content:
    assert passed_iterator is frame_iterator
    
    # Verify response
    assert response.session_id == "test"
    assert response.message == "OK"
