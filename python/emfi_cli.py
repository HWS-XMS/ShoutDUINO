#!/usr/bin/env python3
"""
ShoutDuino EM-FI Command Line Interface

Interactive CLI for controlling the ShoutDuino EM-FI device.
Commands are case-insensitive. Use 'keyword' to read, 'keyword value' to write.

Usage:
    python emfi_cli.py /dev/ttyACM0
    python emfi_cli.py COM3 --auto-connect
"""

import cmd
import sys
import argparse
import time
from typing import Optional

from emfi_device import EMFIDevice, EMFIError, EMFICommError, EMFITemperatureInitError


class EMFICmd(cmd.Cmd):
    """Interactive command-line interface for EM-FI device."""

    intro = """
ShoutDuino EM-FI CLI
Type 'help' for commands. Keywords read values, 'keyword value' sets them.
"""
    prompt = 'EMFI> '

    def __init__(self, port: str, baudrate: int = 115200):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.device: Optional[EMFIDevice] = None
        self.connected = False
        self.charging = False
        self.armed = False

    def precmd(self, line):
        """Make commands case-insensitive."""
        parts = line.split(maxsplit=1)
        if parts:
            parts[0] = parts[0].lower()
            return ' '.join(parts)
        return line

    def _require_connection(self):
        if not self.connected or not self.device:
            print("Not connected. Use 'connect' first.")
            return False
        return True

    # === Connection ===

    def do_connect(self, args):
        """Connect to device."""
        try:
            self.device = EMFIDevice(self.port, self.baudrate)
            self.device.connect()
            self.connected = True
            print(f"Connected to {self.port}")
        except EMFIError as e:
            print(f"Connection failed: {e}")

    def do_disconnect(self, args):
        """Disconnect from device."""
        if self.device:
            self.device.disconnect()
            self.connected = False
            print("Disconnected")

    def do_init(self, args):
        """Initialize device."""
        if not self._require_connection():
            return
        try:
            self.device.initialize()
            print("Initialized")
        except EMFITemperatureInitError as e:
            print(f"Temperature sensor error: {e}")
            print(f"  Sensor: {e.sensor_name}")
            print(f"  Issue: {e.failure_type}")
        except EMFIError as e:
            print(f"Init failed: {e}")

    # === Read/Write Commands ===

    def do_pot(self, args):
        """Potentiometer position (0-1023). Usage: pot [value]"""
        if not self._require_connection():
            return
        try:
            if args.strip():
                pos = int(args)
                self.device.set_pot_position(pos)
                print(f"POT = {pos}")
            else:
                pos = self.device.get_pot_position()
                print(f"POT = {pos}")
        except ValueError:
            print("Invalid value")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_pulse(self, args):
        """Pulse width in ns (10-200). Usage: pulse [value]"""
        if not self._require_connection():
            return
        try:
            if args.strip():
                width = int(args)
                self.device.pulse_width_ns = width
                print(f"PULSE = {width}ns")
            else:
                width = self.device.pulse_width_ns
                print(f"PULSE = {width}ns")
        except ValueError:
            print("Invalid value")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_gate(self, args):
        """Gate driver enable (0/1). Usage: gate [0|1]"""
        if not self._require_connection():
            return
        try:
            if args.strip():
                val = int(args)
                self.device.gate_driver_enabled = bool(val)
                print(f"GATE = {1 if val else 0}")
            else:
                val = self.device.gate_driver_enabled
                print(f"GATE = {1 if val else 0}")
        except ValueError:
            print("Invalid value (use 0 or 1)")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_trigger(self, args):
        """Trigger enable (0/1). Usage: trigger [0|1]"""
        if not self._require_connection():
            return
        try:
            if args.strip():
                val = int(args)
                self.device.trigger_enabled = bool(val)
                print(f"TRIGGER = {1 if val else 0}")
            else:
                val = self.device.trigger_enabled
                print(f"TRIGGER = {1 if val else 0}")
        except ValueError:
            print("Invalid value (use 0 or 1)")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_charge(self, args):
        """Charging enable (0/1). Usage: charge [0|1]"""
        if not self._require_connection():
            return
        try:
            if args.strip():
                val = int(args)
                if val:
                    self.device.charge_start()
                    self.charging = True
                else:
                    self.device.charge_stop()
                    self.charging = False
                print(f"CHARGE = {1 if self.charging else 0}")
            else:
                print(f"CHARGE = {1 if self.charging else 0}")
        except ValueError:
            print("Invalid value (use 0 or 1)")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_loadcal(self, args):
        """Load pot-to-voltage calibration file. Usage: loadcal <filepath>"""
        if not self._require_connection():
            return
        if not args.strip():
            print("Usage: loadcal <filepath>")
            return
        try:
            self.device.load_calibration(args.strip())
            print(f"Calibration loaded from {args.strip()}")
        except Exception as e:
            print(f"Error loading calibration: {e}")

    def do_loadadccal(self, args):
        """Load ADC error correction calibration. Usage: loadadccal <filepath>"""
        if not self._require_connection():
            return
        if not args.strip():
            print("Usage: loadadccal <filepath>")
            return
        try:
            self.device.load_adc_calibration(args.strip())
            print(f"ADC calibration loaded from {args.strip()}")
        except Exception as e:
            print(f"Error loading ADC calibration: {e}")

    # === Read-Only Commands ===

    def do_voltage(self, args):
        """Voltage read/set. Usage: voltage [value]. Set requires loaded calibration."""
        if not self._require_connection():
            return
        try:
            if args.strip():
                if not self.device.calibration_is_loaded():
                    print("Error: Load calibration first (use 'loadcal <file>')")
                    return
                voltage = float(args)
                self.device.set_voltage_calibrated(voltage)
                print(f"VOLTAGE = {voltage:.2f}V (target)")
            else:
                v = self.device.voltage_measured
                print(f"VOLTAGE = {v:.2f}V")
        except ValueError:
            print("Invalid voltage value")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_temp(self, args):
        """Read temperatures."""
        if not self._require_connection():
            return
        try:
            temps = self.device.temperatures
            names = ["NMOS1", "GATE", "TRAFO", "NMOS2"]
            for name, t in zip(names, temps):
                print(f"TEMP_{name} = {t:.1f}C")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_info(self, args):
        """Read device info."""
        if not self._require_connection():
            return
        try:
            info = self.device.device_info
            print(f"INFO = {info}")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_adcdebug(self, args):
        """Read raw ADC bytes for SPI debugging."""
        if not self._require_connection():
            return
        try:
            debug = self.device.get_adc_debug()
            hi, lo = debug['hi'], debug['lo']
            raw = debug['raw']

            # Calculate ADC input voltage (before divider scaling)
            vref = 3.279
            adc_input_v = (raw / 4096.0) * vref

            # Calculate HV (after divider scaling)
            divider_ratio = (5000000 + 25000) / 25000  # 201
            hv = adc_input_v * divider_ratio

            print(f"hi=0x{hi:02X} lo=0x{lo:02X} raw={raw}")
            print(f"ADC input: {adc_input_v:.4f}V  (compare with multimeter at ADC pin)")
            print(f"HV calculated: {hv:.2f}V")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_adcsw(self, args):
        """Read ADC using software SPI (bit-banged) for timing comparison."""
        if not self._require_connection():
            return
        try:
            sw = self.device.get_adc_sw()
            hw = self.device.get_adc_debug()

            vref = 3.279
            sw_adc_in = (sw['raw'] / 4096.0) * vref
            hw_adc_in = (hw['raw'] / 4096.0) * vref

            print("Mode      | raw  | ADC_in  | HV")
            print("----------|------|---------|--------")
            print(f"HW SPI    | {hw['raw']:4d} | {hw_adc_in:.4f}V | {hw['voltage']:6.2f}V")
            print(f"SW SPI    | {sw['raw']:4d} | {sw_adc_in:.4f}V | {sw['voltage']:6.2f}V")
            print(f"Diff      | {sw['raw'] - hw['raw']:+4d} | {sw_adc_in - hw_adc_in:+.4f}V |")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_status(self, args):
        """Read all status values."""
        if not self._require_connection():
            return
        try:
            status = self.device.get_status()
            print(f"CHARGING = {1 if status.charging else 0}")
            print(f"FAULT = {1 if status.fault_detected else 0}")
            print(f"GATE = {1 if status.gate_driver_enabled else 0}")
            print(f"TRIGGER = {1 if status.trigger_enabled else 0}")
            print(f"VOLTAGE = {status.voltage_measured:.2f}V")
            print(f"PULSE = {status.pulse_width_ns}ns")
            names = ["NMOS1", "GATE", "TRAFO", "NMOS2"]
            for name, t in zip(names, status.temperatures):
                print(f"TEMP_{name} = {t:.1f}C")
        except EMFIError as e:
            print(f"Error: {e}")

    # === Actions ===

    def do_fire(self, args):
        """Fire pulse."""
        if not self._require_connection():
            return
        try:
            self.device.fire()
            print("FIRED")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_arm(self, args):
        """Arm state (0/1). Usage: arm [0|1]"""
        if not self._require_connection():
            return
        try:
            if args.strip():
                val = int(args)
                if val:
                    self.device.arm()
                    self.armed = True
                else:
                    self.device.disarm()
                    self.armed = False
                print(f"ARM = {1 if self.armed else 0}")
            else:
                print(f"ARM = {1 if self.armed else 0}")
        except ValueError:
            print("Invalid value (use 0 or 1)")
        except EMFIError as e:
            print(f"Error: {e}")

    def do_stop(self, args):
        """Emergency stop - stop charging and disarm."""
        if self.device:
            try:
                self.device.safe_shutdown()
                self.charging = False
                self.armed = False
                print("STOPPED")
            except Exception as e:
                print(f"Error: {e}")

    def do_test(self, args):
        """Test pot position with charging. Usage: test <pos> [samples=5]"""
        if not self._require_connection():
            return
        try:
            parts = args.split()
            if not parts:
                print("Usage: test <position> [samples]")
                return

            pos = int(parts[0])
            samples = int(parts[1]) if len(parts) > 1 else 5

            print(f"Testing POT={pos}, samples={samples}")
            self.device.set_pot_position(pos)
            self.device.charge_start()
            time.sleep(0.5)

            measurements = []
            for i in range(samples):
                v = self.device.voltage_measured
                measurements.append(v)
                print(f"  [{i+1}] {v:.2f}V")
                time.sleep(0.1)

            self.device.charge_stop()
            avg = sum(measurements) / len(measurements)
            print(f"AVG = {avg:.2f}V (min={min(measurements):.2f}, max={max(measurements):.2f})")

        except ValueError:
            print("Invalid parameters")
        except EMFIError as e:
            print(f"Error: {e}")
            try:
                self.device.charge_stop()
            except:
                pass

    # === Exit ===

    def do_quit(self, args):
        """Quit CLI."""
        if self.connected:
            try:
                self.device.safe_shutdown()
                self.device.disconnect()
            except:
                pass
        print("Bye")
        return True

    def do_exit(self, args):
        """Exit CLI."""
        return self.do_quit(args)

    def do_EOF(self, args):
        """Handle Ctrl+D."""
        print()
        return self.do_quit(args)

    def emptyline(self):
        pass

    def default(self, line):
        print(f"Unknown: {line}")


def main():
    parser = argparse.ArgumentParser(description='ShoutDuino EM-FI CLI')
    parser.add_argument('port', help='Serial port')
    parser.add_argument('--baudrate', '-b', type=int, default=115200)
    parser.add_argument('--auto-connect', '-c', action='store_true')

    args = parser.parse_args()
    cli = EMFICmd(args.port, args.baudrate)

    if args.auto_connect:
        cli.do_connect("")

    try:
        cli.cmdloop()
    except KeyboardInterrupt:
        print("\nInterrupted")
        cli.do_quit("")


if __name__ == '__main__':
    main()
