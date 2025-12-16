import sys
import os

# Get the directory of this file (server/)
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

# Get the project root
PROJECT_ROOT = os.path.dirname(CURRENT_DIR)

# Path to src
SRC_DIR = os.path.join(PROJECT_ROOT, 'src')

if SRC_DIR not in sys.path:
    sys.path.insert(0, SRC_DIR)
