import sys
import os
import unittest

# This is what we want to test: that importing this module fixes the path
try:
    import server.paths
except ImportError:
    # If running from root, server.paths might be accessible, 
    # but we need to ensure the relative path logic inside it works.
    sys.path.append(os.getcwd())
    import server.paths

class TestImports(unittest.TestCase):
    def test_proto_imports(self):
        """Test that we can import the generated proto definitions."""
        try:
            import ksl_sentence_recognition_pb2
            import ksl_sentence_recognition_pb2_grpc
        except ImportError as e:
            self.fail(f"Failed to import proto modules: {e}")

if __name__ == '__main__':
    unittest.main()
