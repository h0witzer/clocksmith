"""Serial transport to the tester firmware.

Handles the Mega's DTR auto-reset by waiting for the firmware's ``READY``
banner after opening the port, then runs a background reader thread that
classifies incoming lines into telemetry samples and command acknowledgements.
"""

from __future__ import annotations

import queue
import threading
import time
from dataclasses import dataclass, field
from typing import Callable, Optional

import serial


@dataclass
class Telemetry:
    """One ``DATA`` sample decoded from the firmware stream."""

    t_ms: int = 0
    target: float = 0.0
    actual: float = 0.0
    rpm: float = 0.0
    sg: int = 0
    diag: bool = False
    steps: int = 0
    irun: int = 0
    otpw: bool = False
    ot: bool = False
    conn: bool = False
    raw: str = ""
    extra: dict = field(default_factory=dict)

    @classmethod
    def parse(cls, line: str) -> "Telemetry":
        """Parse a ``DATA k=v k=v ...`` line into a Telemetry sample."""
        sample = cls(raw=line)
        body = line[len("DATA"):].strip()
        for token in body.split():
            if "=" not in token:
                continue
            key, _, value = token.partition("=")
            if key == "t":
                sample.t_ms = int(value)
            elif key == "target":
                sample.target = float(value)
            elif key == "actual":
                sample.actual = float(value)
            elif key == "rpm":
                sample.rpm = float(value)
            elif key == "sg":
                sample.sg = int(value)
            elif key == "diag":
                sample.diag = value != "0"
            elif key == "steps":
                sample.steps = int(value)
            elif key == "irun":
                sample.irun = int(value)
            elif key == "otpw":
                sample.otpw = value != "0"
            elif key == "ot":
                sample.ot = value != "0"
            elif key == "conn":
                sample.conn = value != "0"
            else:
                sample.extra[key] = value
        return sample


class TesterLink:
    """Threaded serial link that separates telemetry from command replies."""

    def __init__(self, port: str, baud: int = 115200, ready_timeout: float = 6.0):
        self.port = port
        self.baud = baud
        self.ready_timeout = ready_timeout
        self._serial: Optional[serial.Serial] = None
        self._reader: Optional[threading.Thread] = None
        self._stop = threading.Event()
        self._replies: "queue.Queue[str]" = queue.Queue()
        self._ready_line = ""
        self.on_telemetry: Optional[Callable[[Telemetry], None]] = None
        self.on_line: Optional[Callable[[str], None]] = None

    # -- lifecycle ----------------------------------------------------------
    def open(self) -> str:
        """Open the port, wait through the auto-reset for ``READY``."""
        self._serial = serial.Serial(self.port, self.baud, timeout=0.2)
        # Opening toggles DTR and resets the Mega; give the bootloader time and
        # then look for the firmware banner.
        deadline = time.time() + self.ready_timeout
        banner = ""
        while time.time() < deadline:
            raw = self._serial.readline()
            if not raw:
                continue
            text = raw.decode("ascii", errors="replace").strip()
            if text.startswith("READY"):
                banner = text
                break
        self._ready_line = banner
        self._stop.clear()
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()
        return banner

    def close(self) -> None:
        self._stop.set()
        if self._reader:
            self._reader.join(timeout=1.0)
        if self._serial:
            try:
                self._serial.close()
            finally:
                self._serial = None

    @property
    def is_open(self) -> bool:
        return self._serial is not None and self._serial.is_open

    @property
    def ready_banner(self) -> str:
        return self._ready_line

    # -- reader -------------------------------------------------------------
    def _read_loop(self) -> None:
        assert self._serial is not None
        while not self._stop.is_set():
            try:
                raw = self._serial.readline()
            except (serial.SerialException, OSError):
                break
            if not raw:
                continue
            line = raw.decode("ascii", errors="replace").strip()
            if not line:
                continue
            if self.on_line:
                self.on_line(line)
            if line.startswith("DATA"):
                if self.on_telemetry:
                    self.on_telemetry(Telemetry.parse(line))
            else:
                # OK / ERR / READY and anything else are command-channel lines.
                self._replies.put(line)

    # -- command channel ----------------------------------------------------
    def send(self, command: str, expect_reply: bool = True,
             timeout: float = 1.0) -> Optional[str]:
        """Send one command line; optionally wait for the OK/ERR reply."""
        if not self.is_open:
            raise RuntimeError("serial port is not open")
        # Drain stale replies so we match this command's response.
        while not self._replies.empty():
            self._replies.get_nowait()
        assert self._serial is not None
        self._serial.write((command.strip() + "\n").encode("ascii"))
        self._serial.flush()
        if not expect_reply:
            return None
        try:
            return self._replies.get(timeout=timeout)
        except queue.Empty:
            return None
