import argparse
import logging
import sys
import os
from pathlib import Path
from typing import Iterator, List, Optional
import cv2

# Ensure src path is available for imports
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from src.config import AppConfig
from src.video import VideoLoader, parse_roi
from src.ai import MediaPipePoseEstimator, SkeletonPoint
from src.logic import HandTurnDetector, get_rough_hand_status_from_mp, get_motion_status_from_mp, \
    RESET_MOTION_STATUS, READY_MOTION_STATUS, SPEAK_MOTION_STATUS, RAPID_MOTION_STATUS
from src.network import GrpcClient, encode_frame

# Generated Protobuf modules
try:
    from src import ksl_sentence_recognition_pb2 as pb2
except ImportError:
    import ksl_sentence_recognition_pb2 as pb2

# Configure root logger
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def setup_logging(level: int):
    """Sets up logging for the application."""
    logging.getLogger().setLevel(level)
    # Other loggers might need to be adjusted here if specific libraries are too verbose
    logging.getLogger('grpc').setLevel(logging.WARNING) # Reduce grpc verbosity
    logging.getLogger('mediapipe').setLevel(logging.WARNING) # Reduce mediapipe verbosity
    logging.getLogger('protobuf').setLevel(logging.WARNING) # Reduce protobuf verbosity


def process_video_stream(
    config: AppConfig,
    video_loader: VideoLoader,
    pose_estimator: MediaPipePoseEstimator,
    grpc_client: GrpcClient
) -> None:
    """
    Main pipeline to process video, perform AI inference, apply logic, and stream via gRPC.
    """
    session_id = f"session_{Path(config.video_path).stem}_{os.getpid()}"
    frame_count = 0
    prev_skeleton: List[SkeletonPoint] = []
    current_motion_status: int = READY_MOTION_STATUS # Initial state
    right_hand_detector = HandTurnDetector()
    left_hand_detector = HandTurnDetector()

    try:
        logger.info(f"Starting video processing for session: {session_id}")
        # Optional: Connect explicitly if desired, but gRPC channel is lazy
        # grpc_client.connect() 

        def frame_generator() -> Iterator[pb2.Frame]:
            nonlocal frame_count, prev_skeleton, current_motion_status # For updating outer scope variables
            
            for frame_image_rgb in video_loader.get_frames():
                frame_count += 1
                logger.debug(f"Processing frame {frame_count}")

                # AI Inference
                current_skeleton = pose_estimator.process_frame(frame_image_rgb)

                # Business Logic & State Management
                # Update motion status
                current_motion_status = get_motion_status_from_mp(
                    current_motion_status,
                    current_skeleton,
                    prev_skeleton
                )
                
                # Update hand turn detectors (using dummy dt for now)
                # For more accuracy, pass real dt from video_loader or constant frame rate.
                if len(current_skeleton) > 16: # Ensure wrist landmarks exist
                    right_wrist = (current_skeleton[16].x, current_skeleton[16].y)
                    left_wrist = (current_skeleton[15].x, current_skeleton[15].y)
                    
                    # Assuming a constant frame rate for dt if not explicitly calculated
                    # This should ideally come from video_loader.get_fps()
                    DUMMY_DT = 1 / 30.0 

                    if right_hand_detector.update(right_wrist, DUMMY_DT):
                        logger.info(f"Frame {frame_count}: Right hand turn detected!")
                    if left_hand_detector.update(left_wrist, DUMMY_DT):
                        logger.info(f"Frame {frame_count}: Left hand turn detected!")

                # Encode to Protobuf Frame
                # Flag logic: Use 1 (NORMAL) for all frames for now,
                # START/END flags can be added if specific start/end events are detected.
                encoded_frame = encode_frame(
                    session_id=session_id,
                    index=frame_count,
                    flag=current_motion_status, # Use motion status as flag for now
                    image=frame_image_rgb,
                    skeleton=current_skeleton
                )
                
                prev_skeleton = current_skeleton # Update for next frame
                yield encoded_frame
        
        # Stream frames to gRPC server
        response_iterator = grpc_client.send_stream(frame_generator())
        
        # Process final response from server
        for response in response_iterator:
            logger.info(f"Server response for session {response.session_id}: "
                        f"Frame Count: {response.frame_count}, Message: {response.message}")

    except ConnectionRefusedError as e:
        logger.error(f"Failed to connect to gRPC server: {e}")
        sys.exit(1)
    except Exception as e:
        logger.exception(f"An unexpected error occurred during processing: {e}")
        sys.exit(1)
    finally:
        grpc_client.close()
        logger.info(f"Finished processing for session: {session_id}. Total frames: {frame_count}")


def main():
    parser = argparse.ArgumentParser(
        description="Process video for KSL detection and stream to gRPC server."
    )
    parser.add_argument("video_path", type=Path,
                        help="Path to the input video file (e.g., movieTitle.mp4)")
    parser.add_argument("roi", type=str,
                        help="Region of Interest in format 'x,y,w,h' (e.g., '100,100,320,240')")
    parser.add_argument("--server", type=str, default="localhost:50051",
                        help="gRPC server address (default: localhost:50051)")
    parser.add_argument("--model-path", type=Path,
                        default=Path("ksl-cli/protos/pose_landmarker_full.task"),
                        help="Path to the MediaPipe Pose Landmarker model (.task file)")
    parser.add_argument("--output", type=Path,
                        help="Optional: Path to save the raw protobuf packet bytes stream (.bin file)")
    parser.add_argument("--log-level", type=str, default="INFO",
                        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
                        help="Set the logging level (default: INFO)")

    args = parser.parse_args()

    # Setup logging based on argument
    log_level_val = getattr(logging, args.log_level.upper())
    setup_logging(log_level_val)

    try:
        # Parse ROI
        parsed_roi = parse_roi(args.roi)
        
        # Create AppConfig
        config = AppConfig(
            video_path=args.video_path,
            roi=parsed_roi,
            output_path=args.output,
            log_level=log_level_val
        )

        # Initialize components
        # Note: MediaPipe model path handling might need adjustment if default path is relative
        # Ensure model path exists
        if not args.model_path.exists():
             logger.warning(f"Model path {args.model_path} does not exist. Trying absolute path or check config.")
             # You might want to raise an error here depending on strictness

        video_loader = VideoLoader(config)
        pose_estimator = MediaPipePoseEstimator(str(args.model_path)) # Model path as string
        grpc_client = GrpcClient(args.server)

        # Run processing pipeline
        process_video_stream(config, video_loader, pose_estimator, grpc_client)

    except ValueError as e:
        logger.error(f"Configuration Error: {e}")
        sys.exit(1)
    except FileNotFoundError as e:
        logger.error(f"File Error: {e}")
        sys.exit(1)
    except ConnectionRefusedError as e:
        logger.error(f"Network Error: {e}")
        sys.exit(1)
    except Exception as e:
        logger.exception(f"An unexpected error occurred: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
