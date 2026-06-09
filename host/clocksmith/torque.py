"""Holding-torque estimation from a motor's datasheet rating.

Stepper holding torque scales roughly linearly with phase current below
magnetic saturation, so the torque available at a chosen run current can be
estimated from the rated holding torque at rated current. This gives a useful
relative figure for comparing motors pulled from a drawer; it is not a
substitute for measuring dynamic torque under load.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class MotorSpec:
    """Datasheet figures for one stepper, used for torque estimation."""

    name: str
    full_steps_per_rev: int = 200
    rated_current_ma: int = 1000          # rated phase current, RMS mA
    holding_torque_ncm: float = 0.0       # holding torque at rated current
    step_angle_deg: float = 1.8


def estimate_holding_torque_ncm(spec: MotorSpec, run_current_ma: int) -> float:
    """Estimate holding torque (N·cm) at ``run_current_ma``.

    Linear below saturation and clamped so currents above the rating do not
    imply proportionally more torque, which is where saturation sets in.
    """
    if spec.rated_current_ma <= 0 or spec.holding_torque_ncm <= 0:
        return 0.0
    ratio = run_current_ma / spec.rated_current_ma
    if ratio > 1.0:
        ratio = 1.0
    return spec.holding_torque_ncm * ratio
