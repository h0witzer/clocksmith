# Methodology — judging motor viability

The tester answers a narrow question: **for a given microstep / current /
chopper setting, how fast can this motor go before it loses sync or stalls
under load, and is the holding torque in the ballpark I need?**

## What the numbers mean

- **Holding torque (static, absolute-ish).** Holding torque scales roughly
  linearly with phase current below saturation. Enter the motor's rated
  holding torque and rated current in the GUI; it shows the estimated holding
  torque at your selected run current. Good for comparing drawer motors, not a
  dynamic-torque measurement.
- **Dynamic limit (relative, by feel + StallGuard).** Pull-out torque falls as
  speed rises, and at 12 V the drop-off is steep because the low bus voltage
  limits how fast winding current can rise. `SG_RESULT` drops toward 0 as load
  increases; `diag=1` is a hard stall. Pinch the shaft by hand at a test speed
  and watch SG / DIAG to feel where the motor gives up.
- **Step-rate ceiling (the Mega, not the motor).** A 16 MHz AVR tops out around
  30–40 k steps/s. If `actual` step rate plateaus below `target`, you have hit
  the controller, not the motor. Drop microstepping to reach higher RPM.

## Suggested procedure

1. **Wire and connect.** Plug the motor into the selected socket, connect in the
   GUI, confirm `tmc=ok` in the status banner.
2. **Set current for holding torque.** Set `IRUN` to the current matching the
   holding torque you need; confirm the driver holds without `otpw`.
3. **Pick a chopper.** stealthChop2 for quiet low/mid speed; spreadCycle (or a
   `TPWMTHRS` auto-switch) for high-speed traverse where you want more torque.
4. **Sweep speed.** Step `target_rpm` upward. At each speed, apply your load
   (hand-stall or the real slide-whistle mechanism) and watch:
   - `actual` vs `target` — divergence = lost steps / controller ceiling.
   - `SG_RESULT` — margin above 0 = torque headroom.
   - `diag` — stall event.
5. **Record.** Enable CSV recording; each run is timestamped under `results/`.
   The status line reports max RPM, achieved step rate, SG range and whether a
   stall occurred — the per-config viability verdict.

## StallGuard tuning

StallGuard gives a repeatable *relative* load signal, not calibrated N·cm. At a
fixed moderate speed with no load, note the baseline `SG_RESULT`, then raise
`SGTHRS` until `diag` trips near your minimum acceptable load. For absolute
units, correlate `SG_RESULT` against a torque arm + known weights once.

## Chopper trade-off

stealthChop2 is quiet but loses torque as speed rises; that is why the TMC2209
can auto-switch to spreadCycle above `TPWMTHRS`. For the slide-whistle you may
want stealthChop for the slow approach and spreadCycle for the fast traverse —
the tester exposes both so you can hear and measure the difference rather than
assume.
