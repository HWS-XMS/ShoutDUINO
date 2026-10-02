# ShoutDUINO

Arduino-based, safe-ish, EMFI tool, at lower cost.

The board charges a capacitor to a programmable high voltage, then discharges it
through an injection coil on a trigger event. Trigger timing can come from the
host over UART (soft trigger) or from an external trigger input, with host-side
tooling for parameter sweeps.

## Status

**We are currently adding a unity gain buffer to the feedback, which improves
voltage readback.** The buffer isolates the high-voltage divider from the ADC
input, so loading by the converter no longer pulls the measured value down.

This change is in progress and is not yet in the schematic.

## Repository layout

| Path | Contents |
| --- | --- |
| `PCB/` | KiCad project. Root sheet `shoutduino.kicad_sch`, with `charger`, `discharger`, `leds` and `temperature` sub-sheets. |
| `firmware/` | Arduino firmware for the on-board microcontroller. |
| `python/` | Host-side CLI, calibration and experiment scripts. |
| `docs/datasheets/` | Datasheets for the main devices used on the board. |
| `PCB/shoutduino.pretty/`, `PCB/shoutduino.kicad_sym` | Project-local footprint and symbol libraries for the non-stock parts, registered via `PCB/fp-lib-table` and `PCB/sym-lib-table`. |
| `production/` | Generated fabrication output (gerbers, BOM, placement). |
| `DelayUnit/` | Submodule: FPGA-based delay unit used to place the trigger pulse. |

## Hardware

Key devices in the signal chain:

- **LT3751** — flyback controller driving the high-voltage capacitor charger.
- **AD5293** — digital potentiometer setting the charge voltage target.
- **MCP3201** — 12-bit SPI ADC for voltage readback.
- **MCP601** (`U12`) — op-amp in the charger feedback path.
- **TMP117** — temperature monitoring.

The KiCad files are saved in **KiCad 10** format.

## Firmware

Build with `arduino-cli` via the provided Makefile:

```sh
cd firmware
make uno-r4-minima     # or: uno-r4-wifi
make flash             # compile and upload
make detect            # list connected boards
```

Pulse width is constrained to **10–200 ns**. The firmware rejects any soft-trigger
width outside that window, because pulses longer than 200 ns can destroy the
discharge circuit. Note that this guard only covers the UART soft-trigger path —
an externally supplied trigger pulse is not clamped by the firmware.

## Host tooling

```sh
cd python
pip install -r requirements.txt
python emfi_cli.py /dev/ttyACM0     # interactive control
python calibrate.py                 # pot position -> voltage calibration
python experiment.py                # voltage/delay parameter sweep
```

## Cloning

The repository uses a submodule:

```sh
git clone --recurse-submodules https://github.com/HWS-XMS/ShoutDUINO.git
```

## Known issues

- `make uno` (classic AVR Uno) does not compile: `temperature_controller.cpp`
  uses `INPUT_PULLDOWN`, which the AVR core does not provide. Only the UNO R4
  targets build. `make test-all` fails for this reason.

## Safety

This board generates and stores high voltage. The capacitor can hold a dangerous
charge after power is removed. Do not handle the board while it is charged.
