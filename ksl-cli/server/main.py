import grpc
import concurrent.futures
import threading
import queue
import time
import cv2
import numpy as np
import sys
import os

# Add the project root (parent directory of this file) to sys.path
# This ensures that 'import server...' works even if running from inside server/
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(CURRENT_DIR)
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

# Ensure the path setup is run
import server.paths

import server.generated.ksl_sentence_recognition_pb2_grpc as pb2_grpc
from server.service import SequenceServiceServicer
from server.utils import decode_frame

SERVER_PORT = '50051'

# Global queue to pass frames from gRPC thread to main (OpenCV) thread
frame_queue = queue.Queue()

def serve():
    server = grpc.server(concurrent.futures.ThreadPoolExecutor(max_workers=10))
    pb2_grpc.add_SequenceServiceServicer_to_server(
        SequenceServiceServicer(frame_queue),
        server
    )
    server.add_insecure_port(f'[::]:{SERVER_PORT}')
    server.start()
    print(f"gRPC server started on port {SERVER_PORT}")
    server.wait_for_termination()

if __name__ == '__main__':
    # Start gRPC server in a separate thread
    grpc_thread = threading.Thread(target=serve, daemon=True)
    grpc_thread.start()

    # OpenCV display loop in the main thread
    cv2.namedWindow('KSL Detector Stream')

    print("OpenCV display window created. Waiting for frames...")
    try:
        while True:
            if not frame_queue.empty():
                # Item is now the full Frame protobuf message
                frame_msg = frame_queue.get()
                
                # Check if it's the expected object (legacy tuple check removed for clarity as we changed service.py)
                # But to be safe against mixed versions (though local), we assume it's the object.
                # Access fields directly
                
                # Protobuf fields: data, width, height, type, flag
                image_bytes = frame_msg.data
                width = frame_msg.width
                height = frame_msg.height
                cv_type = frame_msg.type
                flag = frame_msg.flag

                frame = decode_frame(image_bytes, width, height, cv_type)
                
                if frame is not None:
                    # Visual indication for Keyframe (Flag 0)
                    if flag == 0:
                        cv2.rectangle(frame, (0, 0), (frame.shape[1]-1, frame.shape[0]-1), (0, 0, 255), 5)
                        cv2.putText(frame, "KEYFRAME", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 
                                    1.5, (0, 0, 255), 3)
                        
                    cv2.imshow('KSL Detector Stream', frame)

            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):  # Press 'q' to quit
                break
            elif key == 27: # ESC key
                break
            
            time.sleep(0.001) # Small delay to prevent busy-waiting when queue is empty

    except KeyboardInterrupt:
        print("Server stopped by user.")
    finally:
        cv2.destroyAllWindows()
        print("OpenCV windows destroyed.")
