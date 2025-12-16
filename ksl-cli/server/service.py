import queue

import server.paths
import ksl_sentence_recognition_pb2 as pb2
import ksl_sentence_recognition_pb2_grpc as pb2_grpc

class SequenceServiceServicer(pb2_grpc.SequenceServiceServicer):
    """Provides methods that implement functionality of the KSL recognition server."""

    def __init__(self, frame_queue):
        self.frame_queue = frame_queue

    def SendFrames(self, request_iterator, context):
        frame_count = 0
        for request in request_iterator:
            # Check for Keyframe flag (0: START)
            if request.flag == 0:
                print(f"[Server] Received KEYFRAME (Flag 0) at index {request.index}")
            
            # Pass the full request object (Frame) to queue to access width/height/type for decoding
            self.frame_queue.put(request)
            frame_count += 1
        return pb2.SubmitResultResponse(session_id="test_session", frame_count=frame_count, message="Frames received successfully")
