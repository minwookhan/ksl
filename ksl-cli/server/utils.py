import cv2
import numpy as np

def decode_frame(image_bytes: bytes, width: int, height: int, cv_type: int) -> np.ndarray:
    """
    Decodes raw image bytes into an OpenCV numpy array using width, height, and type.
    """
    if width <= 0 or height <= 0 or len(image_bytes) == 0:
        return None

    # OpenCV type macro logic: type = ((depth) & 7) + ((channels - 1) << 3)
    # We need to extract depth and channels.
    depth_id = cv_type & 7
    channels = (cv_type >> 3) + 1

    # Mapping OpenCV depth ID to Numpy dtype
    # CV_8U=0, CV_8S=1, CV_16U=2, CV_16S=3, CV_32S=4, CV_32F=5, CV_64F=6
    dtype_map = {
        0: np.uint8,
        1: np.int8,
        2: np.uint16,
        3: np.int16,
        4: np.int32,
        5: np.float32,
        6: np.float64
    }
    
    dtype = dtype_map.get(depth_id, np.uint8) # Default to uint8 if unknown

    try:
        # 1. Create numpy array from raw buffer
        np_arr = np.frombuffer(image_bytes, dtype=dtype)
        
        # 2. Reshape to (Height, Width, Channels)
        # Note: If channels is 1, sometimes it's (H, W) or (H, W, 1). OpenCV usually handles both.
        if channels > 1:
            frame = np_arr.reshape((height, width, channels))
        else:
            frame = np_arr.reshape((height, width))
            
        # 3. Make a writable copy
        # np.frombuffer creates a read-only view of the bytes.
        # OpenCV drawing functions need a writable array.
        return frame.copy()
    except Exception as e:
        print(f"Error decoding frame: {e}, size: {len(image_bytes)}, w: {width}, h: {height}, type: {cv_type}")
        return None
