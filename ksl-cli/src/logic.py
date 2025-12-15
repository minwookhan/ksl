from dataclasses import dataclass, field
from typing import List, Tuple, Optional
from src.ai import SkeletonPoint # Using SkeletonPoint defined in Phase 2
import numpy as np

# Reflecting C++ macros/constants from gRPCFileClientDlg.h
RESET_MOTION_STATUS = -1
READY_MOTION_STATUS = 0
SPEAK_MOTION_STATUS = 1
RAPID_MOTION_STATUS = 2

READY_LOCATION = 700 # Y-axis threshold for hand position in C++ (relative to 1080p)
RAPID_DISTANCE = 0.1 # Normalized distance threshold for rapid motion

@dataclass
class HandTurnDetector:
    """
    Porting the concept of C++ HandTurnDetector.hpp to Python.
    Detects abrupt changes in hand movement (e.g., start of sign language gesture).
    """
    prev_pos: Optional[Tuple[float, float]] = None
    prev_velocity: Optional[Tuple[float, float]] = None
    state: str = "IDLE" # Example states: IDLE, ACCELERATING, DECELERATING

    def reset(self):
        """Resets the detector's internal state."""
        self.prev_pos = None
        self.prev_velocity = None
        self.state = "IDLE"

    def update(self, current_pos: Tuple[float, float], dt: float) -> bool:
        """
        Updates the detector with new hand position and time delta,
        returning True if an abrupt motion (turn) is detected.
        Input: current_pos (Tuple[float, float], 2D normalized wrist coordinates)
               dt (float, time elapsed since previous frame)
        Output: bool (True if abrupt motion detected)
        """
        # This is a simplified/dummy implementation based on the C++ original
        # as the full C++ logic involves velocity, angle changes, and state transitions.
        # For now, if position changes significantly, consider it a "turn".
        if self.prev_pos is None:
            self.prev_pos = current_pos
            return False

        current_np_pos = np.array(current_pos)
        prev_np_pos = np.array(self.prev_pos)
        
        # Check for significant displacement
        if np.linalg.norm(current_np_pos - prev_np_pos) > 0.05: # Arbitrary threshold for dummy detection
            self.prev_pos = current_pos
            return True
        
        self.prev_pos = current_pos
        return False

def get_ref_hl_2d_points_mp(joints: List[SkeletonPoint]) -> List[Tuple[float, float]]:
    """
    Extracts 2D reference points for wrists from MediaPipe 3D skeleton coordinates.
    Similar to C++ GetRefHL2DPointsMP function.
    Input: joints (List[SkeletonPoint], MediaPipe 3D skeleton coordinates)
    Output: List[Tuple[float, float]] (Normalized Left/Right Hand Wrist 2D Coordinates)
    """
    if not joints or len(joints) < 17: # Ensure enough points for wrists (idx 15 and 16)
        return []

    # MediaPipe Pose Landmarker indices:
    # Left Wrist: 15, Right Wrist: 16
    left_wrist_idx = 15
    right_wrist_idx = 16

    # Returning normalized coordinates (0-1) for flexibility.
    # Final scaling to pixel coordinates can be done in the UI layer if needed.
    left_wrist = (joints[left_wrist_idx].x, joints[left_wrist_idx].y)
    right_wrist = (joints[right_wrist_idx].x, joints[right_wrist_idx].y)
    
    return [left_wrist, right_wrist]


def get_rough_hand_status_from_mp(mp_pose: List[SkeletonPoint]) -> int:
    """
    Determines the rough hand status (Ready/Dominant/Non-dominant/Both) from MediaPipe skeleton coordinates.
    Similar to C++ GetRoughHandStatusFromMP function.
    Input: mp_pose (List[SkeletonPoint], MediaPipe 3D skeleton coordinates)
    Output: int (Status code: -1=error, 0=ready, 1=right hand, 2=left hand, 3=both hands)
    """
    if not mp_pose or len(mp_pose) < 17:
        return RESET_MOTION_STATUS # -1

    hl_points = get_ref_hl_2d_points_mp(mp_pose)
    if not hl_points or len(hl_points) < 2:
        return RESET_MOTION_STATUS

    # C++ READY_LOCATION (700) is relative to 1080p. 
    # We convert it to a normalized value (0-1) for MediaPipe coordinates.
    # Assuming 1080 is the reference height for normalization if not dynamically retrieved from config.
    normalized_ready_location = READY_LOCATION / 1080.0 

    left_hand_y = hl_points[0][1] # y-coordinate of left wrist
    right_hand_y = hl_points[1][1] # y-coordinate of right wrist

    left_hand_up = left_hand_y <= normalized_ready_location
    right_hand_up = right_hand_y <= normalized_ready_location

    if right_hand_up and left_hand_up:
        return 3 # Both hands up
    elif right_hand_up and not left_hand_up:
        return 1 # Right hand up
    elif not right_hand_up and left_hand_up:
        return 2 # Left hand up
    else:
        return 0 # Ready status (both down or below threshold)


def get_motion_status_from_mp(current_motion_status: int, cur_mp: List[SkeletonPoint], prev_mp: List[SkeletonPoint]) -> int:
    """
    Updates the motion status (READY/SPEAK/RAPID) by comparing current and previous skeleton coordinates.
    Similar to C++ GetMotionStatusFromMP function.
    Input: current_motion_status (int, current motion status)
           cur_mp (List[SkeletonPoint], current frame skeleton coordinates)
           prev_mp (List[SkeletonPoint], previous frame skeleton coordinates)
    Output: int (New motion status)
    """
    if current_motion_status == RESET_MOTION_STATUS:
        return RESET_MOTION_STATUS # Request to reset state

    motion_status = current_motion_status

    if not prev_mp or not cur_mp or len(prev_mp) < 17 or len(cur_mp) < 17: 
        # If previous frame data is invalid, start in READY status
        return READY_MOTION_STATUS

    # C++ code uses 2D distance for Rdev/Ldev, so we will follow that.
    right_wrist_idx = 16
    left_wrist_idx = 15

    prev_R_wrist = np.array([prev_mp[right_wrist_idx].x, prev_mp[right_wrist_idx].y])
    cur_R_wrist = np.array([cur_mp[right_wrist_idx].x, cur_mp[right_wrist_idx].y])
    Rdev = np.linalg.norm(cur_R_wrist - prev_R_wrist)

    prev_L_wrist = np.array([prev_mp[left_wrist_idx].x, prev_mp[left_wrist_idx].y])
    cur_L_wrist = np.array([cur_mp[left_wrist_idx].x, cur_mp[left_wrist_idx].y])
    Ldev = np.linalg.norm(cur_L_wrist - prev_L_wrist)

    if Rdev > RAPID_DISTANCE or Ldev > RAPID_DISTANCE:
        return RAPID_MOTION_STATUS

    if current_motion_status == READY_MOTION_STATUS:
        hand_status = get_rough_hand_status_from_mp(cur_mp)
        if hand_status > 0: # If any hand is up
            motion_status = SPEAK_MOTION_STATUS
    elif current_motion_status == SPEAK_MOTION_STATUS:
        # C++ logic for SPEAK_MOTION_STATUS to READY_MOTION_STATUS transition is simplified:
        # if prev_mp and cur_mp exist, it moves to READY_MOTION_STATUS if no RAPID_MOTION.
        # This implies a detection of "end of speaking" or similar.
        # For a more robust state machine, additional heuristics would be needed here.
        motion_status = READY_MOTION_STATUS # Simplified transition based on C++ original

    return motion_status
