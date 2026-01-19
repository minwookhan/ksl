import cv2
import numpy as np
import logging
import time
from typing import Generator, Tuple, List, Optional
from src.config import AppConfig

# Generated gRPC modules
try:
    from src import ksl_sentence_recognition_pb2 as pb2
except ImportError:
    import ksl_sentence_recognition_pb2 as pb2

from src.ai import MediaPipePoseEstimator, SkeletonPoint

logger = logging.getLogger(__name__)

def parse_roi(roi_str: str) -> Tuple[int, int, int, int]:
    """
    Parses 'x,y,w,h' string into a tuple of integers.
    """
    try:
        parts = [int(p.strip()) for p in roi_str.split(',')]
        if len(parts) != 4:
            raise ValueError
        return tuple(parts) # type: ignore
    except (ValueError, AttributeError):
        raise ValueError(f"Invalid ROI format. Expected 'x,y,w,h', got '{roi_str}'")

class VideoLoader:
    def __init__(self, config: AppConfig, pose_estimator: Optional[MediaPipePoseEstimator] = None):
        self.config = config
        self.pose_estimator = pose_estimator

    def get_frames(self) -> Generator[Tuple[np.ndarray, np.ndarray, List[pb2.Point3], List[SkeletonPoint]], None, None]:
        """
        Yields preprocessed frames and generated pose data.
        Returns:
            Tuple[np.ndarray, np.ndarray, List[pb2.Point3], List[SkeletonPoint]]
            - processed_frame: RGB image (Resized if needed, for AI/Display)
            - original_roi_frame: RGB image (Original ROI size, for Optical Flow)
            - proto_pose_points: List of Point3 for gRPC
            - skeleton: List of SkeletonPoint for internal logic
        """
        video_path = str(self.config.video_path)
        logger.info(f"Opening video file: {video_path}")
        cap = cv2.VideoCapture(video_path)
        
        try:
            if not cap.isOpened():
                raise IOError(f"Cannot open video file: {self.config.video_path}")
            
            total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
            logger.info(f"Video opened successfully. Total frames: {total_frames}")

            current_frame_index = 0
            end_frame = total_frames

            if self.config.frame_range:
                start_frame, end_frame = self.config.frame_range
                if start_frame > 0:
                    logger.info(f"Seeking to frame {start_frame}")
                    cap.set(cv2.CAP_PROP_POS_FRAMES, start_frame)
                    current_frame_index = start_frame

            while True:
                start_read = time.time()
                ret, frame = cap.read()
                logger.debug(f"[ETA_LOG] 1. VideoLoading: {time.time() - start_read:.6f} sec")

                if not ret:
                    logger.info("End of video reached or failed to read frame.")
                    break
                
                current_frame_index += 1
                
                if current_frame_index > end_frame:
                    logger.info(f"Reached end of specified frame range ({end_frame}). Stopping.")
                    break

                # Check if frame is valid
                if frame is None or frame.size == 0:
                    logger.warning("Empty frame read.")
                    continue
                    
                start_pre = time.time()
                processed_frame, original_roi_frame = self._preprocess(frame)
                logger.debug(f"[ETA_LOG] 2. Preprocessed: {time.time() - start_pre:.6f} sec")

                # Generate Pose Data if estimator is available
                proto_pose_points: List[pb2.Point3] = []
                current_skeleton: List[SkeletonPoint] = []

                if self.pose_estimator:
                    start_ai = time.time()
                    # Use processed_frame (resized) for AI
                    current_skeleton = self.pose_estimator.process_frame(processed_frame)
                    logger.debug(f"[ETA_LOG] 4. AI 추론: {time.time() - start_ai:.6f} sec")
                    
                    # Convert to gRPC format (Pose Data Only)
                    for sp in current_skeleton:
                        proto_pose_points.append(pb2.Point3(x=sp.x, y=sp.y, z=sp.z))

                yield processed_frame, original_roi_frame, proto_pose_points, current_skeleton
                
        finally:
            cap.release()
            logger.info("VideoLoader released.")
    
    def _preprocess(self, frame: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
        """
        Applies ROI crop, Resize (if needed), and Color conversion.
        Returns:
            Tuple[np.ndarray, np.ndarray]: (resized_rgb_frame, original_roi_rgb_frame)
        """
        x, y, w, h = self.config.roi
        
        # 1. ROI Crop (Safe slicing)
        roi_frame = frame[y:y+h, x:x+w]
        
        if roi_frame.size == 0:
             return roi_frame, roi_frame

        # Create original ROI frame in RGB for return (used for Optical Flow calculation match with C++)
        # C++ calculates flow on 'roi' which is the original cropped BGR frame.
        # Python calculates flow on GRAY converted from this return.
        # We return RGB here to be consistent with image format, main.py will convert to GRAY.
        original_roi_rgb = cv2.cvtColor(roi_frame, cv2.COLOR_BGR2RGB)

        # 2. Resize Logic (Match C++ implementation)
        rows, cols = roi_frame.shape[:2]
        if cols >= 320:
            target_x = 320.0
            scale = target_x / cols
            new_width = int(cols * scale)
            new_height = int(rows * scale)
            roi_frame = cv2.resize(roi_frame, (new_width, new_height), interpolation=cv2.INTER_LINEAR)
            
        # 3. BGR to RGB (MediaPipe requires RGB)
        rgb_frame = cv2.cvtColor(roi_frame, cv2.COLOR_BGR2RGB)
        
        return rgb_frame, original_roi_rgb

def calculate_optical_flow_value(prev_gray: np.ndarray, curr_gray: np.ndarray) -> float:
    """
    Calculates the average motion magnitude between two grayscale frames using Sparse Optical Flow (Lucas-Kanade).
    Matches the C++ implementation to ensure consistent thresholding behavior.
    
    Input: prev_gray, curr_gray (uint8 grayscale images)
    Output: float (average motion magnitude of tracked points)
    """
    if prev_gray is None or curr_gray is None:
        return 0.0
    
    if prev_gray.shape != curr_gray.shape:
        return 0.0

    # 1. Detect feature points to track (Sparse)
    # Parameters match typical C++ OpenCV defaults or common usage if not specified
    feature_params = dict(maxCorners=300,
                          qualityLevel=0.01,
                          minDistance=7,
                          blockSize=3)
    
    p0 = cv2.goodFeaturesToTrack(prev_gray, mask=None, **feature_params)
    
    # If no features found, motion is 0
    if p0 is None:
        return 0.0

    # 2. Calculate Optical Flow (Lucas-Kanade)
    lk_params = dict(winSize=(21, 21),
                     maxLevel=3,
                     criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 30, 0.01))
    
    p1, st, err = cv2.calcOpticalFlowPyrLK(prev_gray, curr_gray, p0, None, **lk_params)

    # 3. Calculate Average Motion of Valid Points
    if p1 is not None:
        # Select good points
        good_new = p1[st == 1]
        good_old = p0[st == 1]
        
        if len(good_new) == 0:
            return 0.0
            
        # Calculate Euclidean distances
        displacements = np.linalg.norm(good_new - good_old, axis=1)
        
        # Average magnitude
        avg_motion = np.mean(displacements)
        return float(avg_motion)
        
    return 0.0
