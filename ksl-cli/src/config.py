import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Tuple

@dataclass(frozen=True)
class AppConfig:
    """애플리케이션 실행 설정 (Immutable)"""
    video_path: Path
    roi: Tuple[int, int, int, int]  # (x, y, w, h)
    output_path: Optional[Path] = None
    frame_range: Optional[Tuple[int, int]] = None  # (start_frame, end_frame)
    save_keyframes_only: bool = False
    log_level: int = logging.INFO
    
    def __post_init__(self):
        """기본 유효성 검사"""
        if not self.video_path.exists():
            raise FileNotFoundError(f"Video file not found: {self.video_path}")
        
        if any(v < 0 for v in self.roi):
            raise ValueError(f"ROI values must be non-negative: {self.roi}")
