# Universal Bike Controller

A fork of [nategreco/uBike](https://github.com/nategreco/uBike), an open-source drop-in replacement for the NordicTrack S-series studio cycle consoles.

**What this fork adds:** a physics-based power estimate. The stock firmware reports only ~1 W across resistance levels 2–8, so its watts at other levels are suspect too. This fork replaces that calculation with one calibrated from the bike's own mechanics.

## How the power model is calibrated

The model is derived from **coast-downs**: spin the flywheel up, stop pedaling, and record the RPM decay. Because the flywheel's inertia is known from its mass and radius, the deceleration gives the braking torque at each speed and resistance level:

```
I · (dω/dt) = −T_brake(ω, level)
```

Power at a given cadence is then `T_brake · ω`. No power meter is required — only the tachometer the firmware already provides. And because a magnetic brake's torque depends on flywheel **speed**, not on whether a rider is pedaling, the coast-down curve is the same resistance you feel under load.

A power meter (e.g. Favero Assioma pedals) is optional but raises accuracy from a good physics-based estimate to reference grade. In testing, pedal data showed the stock firmware reading 1.3–2.3× low.

> **Note:** the constants in this repo are calibrated to one bike. Magnet strength, belt tension, and wear vary between units, so for best results recalibrate against your own coast-downs. The scripts in [`misc-scripts/`](misc-scripts/) handle logging and curve-fitting.

## Bike measurements

| Parameter | Value |
|---|---|
| Flywheel diameter | 455.65 mm |
| Flywheel pulley diameter | 58.3 mm |
| Crank pulley diameter | 310 mm |
| Crank → flywheel ratio | ~5.317 : 1 |
| Flywheel mass | 14.515 kg (32 lb, advertised) |
| Mechanical efficiency | 0.95 (estimated) |

## Building & flashing

This fork changes only the power calculation. For hardware, wiring, and build/flash instructions, see the upstream project: [nategreco/uBike](https://github.com/nategreco/uBike).
