"""PySide6 + pyqtgraph GUI for the clocksmith stepper viability tester.

Left panel "draws" the driver/motor test configuration (axis, microstep,
current, chopper mode, StallGuard, speed and accel). Right side shows live
plots of StallGuard load and commanded-vs-actual step rate plus a large stall
indicator, so a motor can be configured and judged at a glance.
"""

from __future__ import annotations

import sys
from collections import deque
from typing import Optional

import pyqtgraph as pg
from PySide6 import QtCore, QtWidgets
from serial.tools import list_ports

from .protocol import (
    AXIS_NAMES,
    MICROSTEP_OPTIONS,
    Controller,
    rpm_to_sps,
)
from .recorder import CsvRecorder
from .torque import MotorSpec, estimate_holding_torque_ncm
from .transport import Telemetry, TesterLink

PLOT_WINDOW = 600  # samples retained on the live plots


class TelemetryBridge(QtCore.QObject):
    """Marshals telemetry from the reader thread onto the GUI thread."""

    sample = QtCore.Signal(object)
    line = QtCore.Signal(str)


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("clocksmith — stepper viability tester")
        self.link: Optional[TesterLink] = None
        self.ctrl: Optional[Controller] = None
        self.recorder = CsvRecorder()
        self.bridge = TelemetryBridge()
        self.bridge.sample.connect(self._on_sample)
        self.bridge.line.connect(self._on_line)

        self._t = deque(maxlen=PLOT_WINDOW)
        self._target = deque(maxlen=PLOT_WINDOW)
        self._actual = deque(maxlen=PLOT_WINDOW)
        self._sg = deque(maxlen=PLOT_WINDOW)

        self._build_ui()

    # -- UI construction ----------------------------------------------------
    def _build_ui(self) -> None:
        central = QtWidgets.QWidget()
        self.setCentralWidget(central)
        layout = QtWidgets.QHBoxLayout(central)
        layout.addWidget(self._build_config_panel(), 0)
        layout.addWidget(self._build_plot_panel(), 1)

    def _build_config_panel(self) -> QtWidgets.QWidget:
        panel = QtWidgets.QWidget()
        panel.setMaximumWidth(340)
        form = QtWidgets.QVBoxLayout(panel)

        # Connection.
        conn_box = QtWidgets.QGroupBox("Connection")
        conn_layout = QtWidgets.QFormLayout(conn_box)
        self.port_combo = QtWidgets.QComboBox()
        self._refresh_ports()
        refresh_btn = QtWidgets.QPushButton("Refresh")
        refresh_btn.clicked.connect(self._refresh_ports)
        self.connect_btn = QtWidgets.QPushButton("Connect")
        self.connect_btn.clicked.connect(self._toggle_connection)
        port_row = QtWidgets.QHBoxLayout()
        port_row.addWidget(self.port_combo, 1)
        port_row.addWidget(refresh_btn)
        conn_layout.addRow("Port", self._wrap(port_row))
        conn_layout.addRow(self.connect_btn)
        self.status_label = QtWidgets.QLabel("Disconnected")
        conn_layout.addRow("Status", self.status_label)
        form.addWidget(conn_box)

        # Driver configuration.
        drv_box = QtWidgets.QGroupBox("Driver")
        drv_layout = QtWidgets.QFormLayout(drv_box)
        self.axis_combo = QtWidgets.QComboBox()
        self.axis_combo.addItems(AXIS_NAMES)
        drv_layout.addRow("Axis", self.axis_combo)

        self.ms_combo = QtWidgets.QComboBox()
        for ms in MICROSTEP_OPTIONS:
            self.ms_combo.addItem("full" if ms == 0 else f"1/{ms}", ms)
        self.ms_combo.setCurrentIndex(MICROSTEP_OPTIONS.index(16))
        self.ms_combo.currentIndexChanged.connect(self._update_sps_label)
        drv_layout.addRow("Microstep", self.ms_combo)

        self.irun_spin = QtWidgets.QSpinBox()
        self.irun_spin.setRange(0, 2000)
        self.irun_spin.setSingleStep(50)
        self.irun_spin.setValue(800)
        self.irun_spin.setSuffix(" mA")
        self.irun_spin.valueChanged.connect(self._update_torque_label)
        drv_layout.addRow("Run current", self.irun_spin)

        self.ihold_spin = QtWidgets.QSpinBox()
        self.ihold_spin.setRange(0, 2000)
        self.ihold_spin.setSingleStep(50)
        self.ihold_spin.setValue(400)
        self.ihold_spin.setSuffix(" mA")
        drv_layout.addRow("Hold current", self.ihold_spin)

        self.mode_combo = QtWidgets.QComboBox()
        self.mode_combo.addItems(["stealthChop2", "spreadCycle"])
        drv_layout.addRow("Chopper", self.mode_combo)

        self.tpwm_spin = QtWidgets.QSpinBox()
        self.tpwm_spin.setRange(0, 1048575)
        self.tpwm_spin.setValue(0)
        drv_layout.addRow("TPWMTHRS", self.tpwm_spin)

        self.sgt_spin = QtWidgets.QSpinBox()
        self.sgt_spin.setRange(0, 255)
        self.sgt_spin.setValue(60)
        drv_layout.addRow("StallGuard SGTHRS", self.sgt_spin)
        form.addWidget(drv_box)

        # Motor / torque estimate.
        motor_box = QtWidgets.QGroupBox("Motor")
        motor_layout = QtWidgets.QFormLayout(motor_box)
        self.steps_rev_spin = QtWidgets.QSpinBox()
        self.steps_rev_spin.setRange(4, 1000)
        self.steps_rev_spin.setValue(200)
        self.steps_rev_spin.valueChanged.connect(self._update_torque_label)
        motor_layout.addRow("Full steps/rev", self.steps_rev_spin)

        self.rated_current_spin = QtWidgets.QSpinBox()
        self.rated_current_spin.setRange(50, 3000)
        self.rated_current_spin.setSingleStep(50)
        self.rated_current_spin.setValue(1000)
        self.rated_current_spin.setSuffix(" mA")
        self.rated_current_spin.valueChanged.connect(self._update_torque_label)
        motor_layout.addRow("Rated current", self.rated_current_spin)

        self.rated_torque_spin = QtWidgets.QDoubleSpinBox()
        self.rated_torque_spin.setRange(0.0, 1000.0)
        self.rated_torque_spin.setValue(0.0)
        self.rated_torque_spin.setSuffix(" N·cm")
        self.rated_torque_spin.valueChanged.connect(self._update_torque_label)
        motor_layout.addRow("Rated holding", self.rated_torque_spin)

        self.torque_label = QtWidgets.QLabel("—")
        motor_layout.addRow("Est. holding", self.torque_label)
        form.addWidget(motor_box)

        # Motion.
        motion_box = QtWidgets.QGroupBox("Motion")
        motion_layout = QtWidgets.QFormLayout(motion_box)
        self.rpm_spin = QtWidgets.QDoubleSpinBox()
        self.rpm_spin.setRange(0.0, 3000.0)
        self.rpm_spin.setValue(120.0)
        self.rpm_spin.setSuffix(" rpm")
        self.rpm_spin.valueChanged.connect(self._update_sps_label)
        motion_layout.addRow("Target speed", self.rpm_spin)

        self.sps_label = QtWidgets.QLabel("—")
        motion_layout.addRow("= step rate", self.sps_label)

        self.accel_spin = QtWidgets.QSpinBox()
        self.accel_spin.setRange(100, 200000)
        self.accel_spin.setSingleStep(1000)
        self.accel_spin.setValue(20000)
        self.accel_spin.setSuffix(" st/s²")
        motion_layout.addRow("Accel", self.accel_spin)

        apply_btn = QtWidgets.QPushButton("Apply config")
        apply_btn.clicked.connect(self._apply_config)
        motion_layout.addRow(apply_btn)

        btn_row = QtWidgets.QHBoxLayout()
        self.run_btn = QtWidgets.QPushButton("Run")
        self.run_btn.clicked.connect(self._run)
        self.stop_btn = QtWidgets.QPushButton("Stop")
        self.stop_btn.clicked.connect(self._stop)
        self.estop_btn = QtWidgets.QPushButton("E-STOP")
        self.estop_btn.clicked.connect(self._estop)
        btn_row.addWidget(self.run_btn)
        btn_row.addWidget(self.stop_btn)
        btn_row.addWidget(self.estop_btn)
        motion_layout.addRow(self._wrap(btn_row))

        self.record_check = QtWidgets.QCheckBox("Record CSV on run")
        self.record_check.setChecked(True)
        motion_layout.addRow(self.record_check)
        form.addWidget(motion_box)

        form.addStretch(1)
        self._update_torque_label()
        self._update_sps_label()
        self._set_controls_enabled(False)
        return panel

    def _build_plot_panel(self) -> QtWidgets.QWidget:
        panel = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout(panel)

        self.stall_banner = QtWidgets.QLabel("NO STALL")
        self.stall_banner.setAlignment(QtCore.Qt.AlignCenter)
        self.stall_banner.setStyleSheet(
            "font-size: 20px; font-weight: bold; padding: 8px;"
            " background: #1f7a1f; color: white; border-radius: 4px;")
        layout.addWidget(self.stall_banner)

        rate_plot = pg.PlotWidget(title="Step rate (steps/s)")
        rate_plot.addLegend()
        rate_plot.setLabel("bottom", "sample")
        self._target_curve = rate_plot.plot(pen=pg.mkPen("#3b8eea", width=2),
                                            name="commanded")
        self._actual_curve = rate_plot.plot(pen=pg.mkPen("#e2a23b", width=2),
                                            name="actual")
        layout.addWidget(rate_plot)

        sg_plot = pg.PlotWidget(title="StallGuard load (SG_RESULT — lower = more load)")
        sg_plot.setLabel("bottom", "sample")
        self._sg_curve = sg_plot.plot(pen=pg.mkPen("#cc3b3b", width=2))
        layout.addWidget(sg_plot)

        self.readout = QtWidgets.QLabel("—")
        self.readout.setStyleSheet("font-family: monospace;")
        layout.addWidget(self.readout)
        return panel

    @staticmethod
    def _wrap(child_layout: QtWidgets.QLayout) -> QtWidgets.QWidget:
        w = QtWidgets.QWidget()
        w.setLayout(child_layout)
        return w

    # -- connection ---------------------------------------------------------
    def _refresh_ports(self) -> None:
        self.port_combo.clear()
        for info in list_ports.comports():
            self.port_combo.addItem(info.device)

    def _toggle_connection(self) -> None:
        if self.link and self.link.is_open:
            self._disconnect()
        else:
            self._connect()

    def _connect(self) -> None:
        port = self.port_combo.currentText().strip()
        if not port:
            self.status_label.setText("No port selected")
            return
        self.link = TesterLink(port)
        self.link.on_telemetry = self.bridge.sample.emit
        self.link.on_line = self.bridge.line.emit
        try:
            banner = self.link.open()
        except Exception as exc:  # surface any serial error to the user
            self.status_label.setText(f"Error: {exc}")
            self.link = None
            return
        self.ctrl = Controller(self.link)
        self.status_label.setText(banner or "Connected (no banner)")
        self.connect_btn.setText("Disconnect")
        self._set_controls_enabled(True)

    def _disconnect(self) -> None:
        if self.recorder.is_recording:
            self.recorder.stop()
        if self.link:
            self.link.close()
        self.link = None
        self.ctrl = None
        self.status_label.setText("Disconnected")
        self.connect_btn.setText("Connect")
        self._set_controls_enabled(False)

    def _set_controls_enabled(self, on: bool) -> None:
        for btn in (self.run_btn, self.stop_btn, self.estop_btn):
            btn.setEnabled(on)

    # -- config / motion ----------------------------------------------------
    def _current_microsteps(self) -> int:
        return self.ms_combo.currentData()

    def _apply_config(self) -> None:
        if not self.ctrl:
            return
        self.ctrl.select_axis(self.axis_combo.currentText())
        self.ctrl.set_steps_per_rev(self.steps_rev_spin.value())
        self.ctrl.set_microsteps(self._current_microsteps())
        self.ctrl.set_run_current(self.irun_spin.value())
        self.ctrl.set_hold_current(self.ihold_spin.value())
        self.ctrl.set_mode(self.mode_combo.currentIndex() == 0)
        self.ctrl.set_tpwm_threshold(self.tpwm_spin.value())
        self.ctrl.set_stall_threshold(self.sgt_spin.value())
        self.ctrl.set_accel(self.accel_spin.value())

    def _run(self) -> None:
        if not self.ctrl:
            return
        self._apply_config()
        sps = rpm_to_sps(self.rpm_spin.value(), self.steps_rev_spin.value(),
                         self._current_microsteps())
        self._clear_plots()
        if self.record_check.isChecked():
            label = f"{self.axis_combo.currentText()}_ms{self._current_microsteps()}"
            self.recorder.start(label)
        self.ctrl.stream(True)
        self.ctrl.run(sps)

    def _stop(self) -> None:
        if self.ctrl:
            self.ctrl.stop()
            self.ctrl.stream(False)
        self._finish_recording()

    def _estop(self) -> None:
        if self.ctrl:
            self.ctrl.emergency_stop()
            self.ctrl.stream(False)
        self._finish_recording()

    def _finish_recording(self) -> None:
        if self.recorder.is_recording:
            path = self.recorder.stop()
            s = self.recorder.summary
            verdict = (
                f"Saved {path} | max {s.max_rpm:.0f} rpm "
                f"({s.max_actual_sps:.0f} st/s) | "
                f"SG {s.min_sg}-{s.max_sg} | "
                f"{'STALLED' if s.stalled else 'no stall'}"
            )
            self.status_label.setText(verdict)

    # -- live updates -------------------------------------------------------
    @QtCore.Slot(object)
    def _on_sample(self, sample: Telemetry) -> None:
        self._t.append(sample.t_ms)
        self._target.append(sample.target)
        self._actual.append(sample.actual)
        self._sg.append(sample.sg)
        self._target_curve.setData(list(self._target))
        self._actual_curve.setData(list(self._actual))
        self._sg_curve.setData(list(self._sg))

        if sample.diag or sample.ot:
            self._set_stall(True, sample)
        else:
            self._set_stall(False, sample)

        self.readout.setText(
            f"target {sample.target:.0f} st/s   actual {sample.actual:.0f} st/s   "
            f"{sample.rpm:.1f} rpm   SG {sample.sg}   "
            f"IRUN {sample.irun} mA   otpw {int(sample.otpw)}   ot {int(sample.ot)}"
        )
        if self.recorder.is_recording:
            self.recorder.write(sample)

    def _set_stall(self, stalled: bool, sample: Telemetry) -> None:
        if stalled:
            text = "STALL / DIAG" if sample.diag else "OVERTEMP"
            self.stall_banner.setText(text)
            self.stall_banner.setStyleSheet(
                "font-size: 20px; font-weight: bold; padding: 8px;"
                " background: #b22222; color: white; border-radius: 4px;")
        else:
            self.stall_banner.setText("NO STALL")
            self.stall_banner.setStyleSheet(
                "font-size: 20px; font-weight: bold; padding: 8px;"
                " background: #1f7a1f; color: white; border-radius: 4px;")

    @QtCore.Slot(str)
    def _on_line(self, line: str) -> None:
        if line.startswith("ERR"):
            self.status_label.setText(line)

    def _clear_plots(self) -> None:
        for buf in (self._t, self._target, self._actual, self._sg):
            buf.clear()

    def _update_torque_label(self) -> None:
        spec = MotorSpec(
            name="under-test",
            full_steps_per_rev=self.steps_rev_spin.value(),
            rated_current_ma=self.rated_current_spin.value(),
            holding_torque_ncm=self.rated_torque_spin.value(),
        )
        est = estimate_holding_torque_ncm(spec, self.irun_spin.value())
        self.torque_label.setText(f"{est:.1f} N·cm" if est > 0 else "—")

    def _update_sps_label(self) -> None:
        sps = rpm_to_sps(self.rpm_spin.value(), self.steps_rev_spin.value(),
                         self._current_microsteps())
        self.sps_label.setText(f"{sps:.0f} st/s")

    def closeEvent(self, event) -> None:  # noqa: N802 (Qt override)
        self._disconnect()
        super().closeEvent(event)


def main() -> int:
    pg.setConfigOptions(antialias=True)
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    window.resize(1100, 720)
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
