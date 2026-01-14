import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Tuple

@dataclass
class AppConfig:
    video_path: Path
    roi: Tuple[int, int, int, int]
    output_path: Optional[Path] = None
    frame_range: Optional[Tuple[int, int]] = None
    save_keyframes_only: int = 0 # 0: gRPC, 1: Save Images, 2: Encode Only
    log_level: int = logging.INFO
    no_gui: bool = False
    debug_file: bool = False
    
    def __post_init__(self):
        """기본 유효성 검사"""
        if not self.video_path.exists():
            raise FileNotFoundError(f"Video file not found: {self.video_path}")
        
        if any(v < 0 for v in self.roi):
            raise ValueError(f"ROI values must be non-negative: {self.roi}")
