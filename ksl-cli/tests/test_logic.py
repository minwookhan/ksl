import pytest
import numpy as np
from typing import List, Tuple

from src.logic import (
    HandTurnDetector,
    get_ref_hl_2d_points_mp,
    get_rough_hand_status_from_mp,
    get_motion_status_from_mp,
    RESET_MOTION_STATUS, READY_MOTION_STATUS, SPEAK_MOTION_STATUS, RAPID_MOTION_STATUS,
    READY_LOCATION, RAPID_DISTANCE
)
from src.ai import SkeletonPoint

# --- Fixtures for SkeletonPoint Data ---
def create_skeleton_list(left_y, right_y, filler_y=0.9):
    # Create a list of 33 points (standard MediaPipe Pose)
    # Default filler_y=0.9 (Safe "Down" position, > 0.648)
    points = [SkeletonPoint(x=0.5, y=filler_y, z=0.1)] * 33
    points[15] = SkeletonPoint(x=0.5, y=left_y, z=0.1) # Left Wrist
    points[16] = SkeletonPoint(x=0.6, y=right_y, z=0.1) # Right Wrist
    return points

@pytest.fixture
def dummy_skeleton_points_below_ready() -> List[SkeletonPoint]:
    # Both hands below READY_LOCATION (y > 0.648)
    return create_skeleton_list(left_y=0.8, right_y=0.8)

@pytest.fixture
def dummy_skeleton_points_right_hand_up() -> List[SkeletonPoint]:
    # Right hand up (y <= 0.648), Left hand down
    return create_skeleton_list(left_y=0.8, right_y=0.5)

@pytest.fixture
def dummy_skeleton_points_left_hand_up() -> List[SkeletonPoint]:
    # Left hand up (y <= 0.648), Right hand down
    return create_skeleton_list(left_y=0.5, right_y=0.8)

@pytest.fixture
def dummy_skeleton_points_both_hands_up() -> List[SkeletonPoint]:
    # Both hands up (y <= 0.648)
    return create_skeleton_list(left_y=0.5, right_y=0.5)

@pytest.fixture
def dummy_skeleton_points_no_hands() -> List[SkeletonPoint]:
    return []

# --- Test HandTurnDetector ---
def test_hand_turn_detector_reset_and_update():
    detector = HandTurnDetector()
    assert detector.prev_pos is None
    assert detector.prev_velocity is None
    assert detector.state == "IDLE"

    detector.update(current_pos=(0.1, 0.1), dt=0.033) # First update
    assert detector.prev_pos == (0.1, 0.1)
    
    # No significant change
    assert not detector.update(current_pos=(0.11, 0.11), dt=0.033)
    assert detector.prev_pos == (0.11, 0.11)

    # Significant change (should trigger dummy turn)
    assert detector.update(current_pos=(0.2, 0.2), dt=0.033)
    assert detector.prev_pos == (0.2, 0.2)

    detector.reset()
    assert detector.prev_pos is None
    assert detector.state == "IDLE"

# --- Test get_ref_hl_2d_points_mp ---
def test_get_ref_hl_2d_points_mp_valid_input(dummy_skeleton_points_both_hands_up):
    # Using a fixture that has valid wrist points
    result = get_ref_hl_2d_points_mp(dummy_skeleton_points_both_hands_up)
    assert len(result) == 2
    # Correct assertion: compare tuples
    expected_left = (dummy_skeleton_points_both_hands_up[15].x, dummy_skeleton_points_both_hands_up[15].y)
    expected_right = (dummy_skeleton_points_both_hands_up[16].x, dummy_skeleton_points_both_hands_up[16].y)
    assert result[0] == expected_left
    assert result[1] == expected_right

def test_get_ref_hl_2d_points_mp_empty_input():
    result = get_ref_hl_2d_points_mp([])
    assert result == []

def test_get_ref_hl_2d_points_mp_insufficient_points():
    # Test with less than 17 points (e.g., no wrist data)
    result = get_ref_hl_2d_points_mp([SkeletonPoint(x=0,y=0,z=0)] * 5)
    assert result == []

# --- Test get_rough_hand_status_from_mp ---
def test_get_rough_hand_status_from_mp_both_hands_down(dummy_skeleton_points_below_ready):
    status = get_rough_hand_status_from_mp(dummy_skeleton_points_below_ready)
    assert status == READY_MOTION_STATUS

def test_get_rough_hand_status_from_mp_right_hand_up(dummy_skeleton_points_right_hand_up):
    status = get_rough_hand_status_from_mp(dummy_skeleton_points_right_hand_up)
    assert status == 1 # Right hand up

def test_get_rough_hand_status_from_mp_left_hand_up(dummy_skeleton_points_left_hand_up):
    status = get_rough_hand_status_from_mp(dummy_skeleton_points_left_hand_up)
    assert status == 2 # Left hand up

def test_get_rough_hand_status_from_mp_both_hands_up(dummy_skeleton_points_both_hands_up):
    status = get_rough_hand_status_from_mp(dummy_skeleton_points_both_hands_up)
    assert status == 3 # Both hands up

def test_get_rough_hand_status_from_mp_empty_input(dummy_skeleton_points_no_hands):
    status = get_rough_hand_status_from_mp(dummy_skeleton_points_no_hands)
    assert status == RESET_MOTION_STATUS

# --- Test get_motion_status_from_mp_state_transitions ---
def test_get_motion_status_from_mp_reset():
    status = get_motion_status_from_mp(RESET_MOTION_STATUS, [], [])
    assert status == RESET_MOTION_STATUS

def test_get_motion_status_from_mp_initial_ready():
    cur_mp = create_skeleton_list(0.5, 0.5)
    status = get_motion_status_from_mp(READY_MOTION_STATUS, cur_mp, [])
    assert status == READY_MOTION_STATUS

def test_get_motion_status_from_mp_ready_to_speak():
    # Simulate hands moving up slowly (to avoid RAPID detection)
    # Distance change must be <= RAPID_DISTANCE (0.1)
    # Previous: y=0.7 (Down)
    # Current: y=0.61 (Up, since < 0.648)
    # Diff = 0.09 ( < 0.1 )
    
    prev_mp = create_skeleton_list(left_y=0.7, right_y=0.7)
    cur_mp = create_skeleton_list(left_y=0.61, right_y=0.61)
    
    status = get_motion_status_from_mp(READY_MOTION_STATUS, cur_mp, prev_mp)
    assert status == SPEAK_MOTION_STATUS

def test_get_motion_status_from_mp_speak_to_ready_no_rapid():
    # Simulate remaining in SPEAK status with no rapid motion (simplified C++ logic)
    prev_mp = create_skeleton_list(0.5, 0.5)
    cur_mp = create_skeleton_list(0.51, 0.51) # Small movement
    
    status = get_motion_status_from_mp(SPEAK_MOTION_STATUS, cur_mp, prev_mp)
    assert status == READY_MOTION_STATUS # Simplified: transitions to READY if not rapid

def test_get_motion_status_from_mp_rapid_motion():
    # Simulate rapid motion (large change in wrist position)
    # Diff > 0.1
    prev_mp = create_skeleton_list(0.5, 0.5)
    cur_mp = create_skeleton_list(0.5, 0.5 + RAPID_DISTANCE + 0.05) # Large movement y
    
    status = get_motion_status_from_mp(READY_MOTION_STATUS, cur_mp, prev_mp)
    assert status == RAPID_MOTION_STATUS

def test_get_motion_status_from_mp_insufficient_landmarks_for_motion_detection():
    # Should return READY_MOTION_STATUS if prev_mp or cur_mp is too short for wrist landmarks
    prev_mp = [SkeletonPoint(x=0,y=0,z=0)] * 5 # insufficient
    cur_mp = create_skeleton_list(0.5, 0.5)
    status = get_motion_status_from_mp(READY_MOTION_STATUS, cur_mp, prev_mp)
    assert status == READY_MOTION_STATUS

    prev_mp = create_skeleton_list(0.5, 0.5)
    cur_mp = [SkeletonPoint(x=0,y=0,z=0)] * 5 # insufficient
    status = get_motion_status_from_mp(READY_MOTION_STATUS, cur_mp, prev_mp)
    assert status == READY_MOTION_STATUS