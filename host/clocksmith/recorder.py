"""CSV logging and per-run summary for tester telemetry."""

from __future__ import annotations

import csv
import os
from dataclasses import dataclass
from datetime import datetime
from typing import Optional, TextIO

from .transport import Telemetry

FIELDNAMES = [
    "t_ms", "target", "actual", "rpm", "sg", "diag",
    "steps", "irun", "otpw", "ot", "conn",
]


@dataclass
class RunSummary:
    """Rolling aggregate over a recorded run, used for the viability verdict."""

    samples: int = 0
    max_actual_sps: float = 0.0
    max_rpm: float = 0.0
    min_sg: Optional[int] = None
    max_sg: int = 0
    stalled: bool = False
    overtemp: bool = False

    def update(self, sample: Telemetry) -> None:
        self.samples += 1
        self.max_actual_sps = max(self.max_actual_sps, abs(sample.actual))
        self.max_rpm = max(self.max_rpm, abs(sample.rpm))
        self.max_sg = max(self.max_sg, sample.sg)
        self.min_sg = sample.sg if self.min_sg is None else min(self.min_sg, sample.sg)
        if sample.diag:
            self.stalled = True
        if sample.ot or sample.otpw:
            self.overtemp = True


class CsvRecorder:
    """Append telemetry samples to a timestamped CSV file."""

    def __init__(self, directory: str = "results"):
        self.directory = directory
        self._file: Optional[TextIO] = None
        self._writer: Optional[csv.DictWriter] = None
        self.path: Optional[str] = None
        self.summary = RunSummary()

    def start(self, label: str = "run") -> str:
        os.makedirs(self.directory, exist_ok=True)
        stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
        safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in label)
        self.path = os.path.join(self.directory, f"{stamp}_{safe}.csv")
        self._file = open(self.path, "w", newline="")
        self._writer = csv.DictWriter(self._file, fieldnames=FIELDNAMES)
        self._writer.writeheader()
        self.summary = RunSummary()
        return self.path

    def write(self, sample: Telemetry) -> None:
        if not self._writer or not self._file:
            return
        self._writer.writerow({
            "t_ms": sample.t_ms,
            "target": sample.target,
            "actual": sample.actual,
            "rpm": sample.rpm,
            "sg": sample.sg,
            "diag": int(sample.diag),
            "steps": sample.steps,
            "irun": sample.irun,
            "otpw": int(sample.otpw),
            "ot": int(sample.ot),
            "conn": int(sample.conn),
        })
        self._file.flush()
        self.summary.update(sample)

    def stop(self) -> Optional[str]:
        if self._file:
            self._file.close()
            self._file = None
            self._writer = None
        return self.path

    @property
    def is_recording(self) -> bool:
        return self._file is not None
