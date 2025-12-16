import cv2
import numpy as np

def decode_frame(image_bytes: bytes) -> np.ndarray:
    """Decodes image bytes into an OpenCV numpy array."""
    np_arr = np.frombuffer(image_bytes, np.uint8)
    frame = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
    return frame
