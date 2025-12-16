import unittest
import queue
import sys
import os
from unittest.mock import Mock

# Ensure the path setup is run
try:
    import server.paths
except ImportError:
    sys.path.append(os.getcwd())
    import server.paths

import ksl_sentence_recognition_pb2 as pb2
import ksl_sentence_recognition_pb2_grpc as pb2_grpc

from server.service import SequenceServiceServicer


class TestSequenceService(unittest.TestCase):

    def test_send_frames_pushes_to_queue(self):
        """Test that SendFrames method correctly pushes image data to the queue."""
        mock_queue = queue.Queue()
        servicer = SequenceServiceServicer(mock_queue)

        # Create mock Frame messages
        frame1 = pb2.Frame(session_id="test_session", index=0, data=b"frame_data_1")
        frame2 = pb2.Frame(session_id="test_session", index=1, data=b"frame_data_2")
        frame3 = pb2.Frame(session_id="test_session", index=2, data=b"frame_data_3")

        mock_request_iterator = iter([frame1, frame2, frame3])
        mock_context = Mock()

        servicer.SendFrames(mock_request_iterator, mock_context)

        # Assert data was put into the queue
        self.assertEqual(mock_queue.qsize(), 3)
        self.assertEqual(mock_queue.get(), b"frame_data_1")
        self.assertEqual(mock_queue.get(), b"frame_data_2")
        self.assertEqual(mock_queue.get(), b"frame_data_3")
        self.assertTrue(mock_queue.empty())

if __name__ == '__main__':
    unittest.main()
