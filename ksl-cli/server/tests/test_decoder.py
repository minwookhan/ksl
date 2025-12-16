import unittest
import sys
import os
import cv2
import numpy as np

# Ensure the path setup is run
try:
    import server.paths
except ImportError:
    sys.path.append(os.getcwd())
    import server.paths

from server.utils import decode_frame

class TestImageDecoder(unittest.TestCase):

    def test_decode_frame_returns_numpy_array(self):
        """Test that decode_frame decodes image bytes into a numpy array."""
        # Create dummy JPEG image bytes (a very small red square)
        # Using OpenCV to encode a simple image into bytes for testing
        dummy_image = np.zeros((10, 10, 3), dtype=np.uint8)
        dummy_image[:, :] = [0, 0, 255]  # Red color
        _, encoded_image = cv2.imencode('.jpg', dummy_image)
        image_bytes = encoded_image.tobytes()

        # Call the decoder function (using mock for now, will replace)
        decoded_frame = decode_frame(image_bytes)

        # Assertions for a failing test:
        # We expect it to be a numpy array, but the mock returns None
        self.assertIsInstance(decoded_frame, np.ndarray, "Decoded frame should be a numpy array")
        self.assertEqual(decoded_frame.shape, (10, 10, 3), "Decoded frame should have correct shape")
        self.assertEqual(decoded_frame.dtype, np.uint8, "Decoded frame should have correct dtype")

if __name__ == '__main__':
    unittest.main()
