"""High-level command helpers that build the firmware's ASCII protocol lines.

Keeping the protocol vocabulary in one place lets the GUI and any scripted test
runner share the exact same command strings. See docs/protocol.md.
"""

from __future__ import annotations

from typing import Optional

from .transport import TesterLink

# Microstep options supported by the TMC2209 (0 means full step).
MICROSTEP_OPTIONS = [0, 2, 4, 8, 16, 32, 64, 128, 256]

# RAMPS 1.4 socket names in firmware order.
AXIS_NAMES = ["X", "Y", "Z", "E0", "E1"]


class Controller:
    """Typed wrapper over :class:`TesterLink` for tester commands."""

    def __init__(self, link: TesterLink):
        self.link = link

    def _send(self, command: str) -> bool:
        reply = self.link.send(command)
        return reply is not None and reply.startswith("OK")

    def ping(self) -> bool:
        reply = self.link.send("PING")
        return reply is not None and reply.startswith("OK")

    def select_axis(self, axis: str) -> bool:
        return self._send(f"AXIS={axis}")

    def set_microsteps(self, microsteps: int) -> bool:
        return self._send(f"MS={microsteps}")

    def set_run_current(self, milliamps: int) -> bool:
        return self._send(f"IRUN={int(milliamps)}")

    def set_hold_current(self, milliamps: int) -> bool:
        return self._send(f"IHOLD={int(milliamps)}")

    def set_mode(self, stealth: bool) -> bool:
        return self._send(f"MODE={'stealth' if stealth else 'spread'}")

    def set_tpwm_threshold(self, tstep: int) -> bool:
        return self._send(f"TPWM={int(tstep)}")

    def set_stall_threshold(self, sgthrs: int) -> bool:
        return self._send(f"SGT={int(sgthrs)}")

    def set_cool_threshold(self, tstep: int) -> bool:
        return self._send(f"TCOOL={int(tstep)}")

    def set_steps_per_rev(self, full_steps: int) -> bool:
        return self._send(f"STEPSPERREV={int(full_steps)}")

    def set_accel(self, steps_per_sec2: float) -> bool:
        return self._send(f"ACCEL={int(steps_per_sec2)}")

    def enable(self, on: bool) -> bool:
        return self._send(f"EN={1 if on else 0}")

    def run(self, steps_per_sec: float, steps: Optional[int] = None) -> bool:
        cmd = f"RUN sps={int(steps_per_sec)}"
        if steps:
            cmd += f" steps={int(steps)}"
        return self._send(cmd)

    def stop(self) -> bool:
        return self._send("STOP")

    def emergency_stop(self) -> bool:
        return self._send("ESTOP")

    def stream(self, on: bool) -> bool:
        return self._send(f"STREAM={1 if on else 0}")

    def query(self) -> None:
        # QUERY answers on the telemetry channel, not with OK/ERR.
        self.link.send("QUERY", expect_reply=False)


def rpm_to_sps(rpm: float, full_steps_per_rev: int, microsteps: int) -> float:
    """Convert motor RPM to microsteps per second for a given configuration."""
    effective = microsteps if microsteps else 1
    return rpm / 60.0 * full_steps_per_rev * effective


def sps_to_rpm(sps: float, full_steps_per_rev: int, microsteps: int) -> float:
    """Convert microsteps per second back to motor RPM."""
    effective = microsteps if microsteps else 1
    denom = full_steps_per_rev * effective
    return (sps / denom) * 60.0 if denom else 0.0
