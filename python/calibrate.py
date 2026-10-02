#!/usr/bin/env python3
"""
ShoutDuino Calibration Script

Generates a calibration file by sweeping all potentiometer positions
and measuring the resulting voltage.

Usage:
    python calibrate.py --port /dev/ttyACM0 --samples 10 --output calibration.yaml
"""

import argparse
import json
import yaml
import time
import sys
from datetime import datetime
from emfi_device import EMFIDevice, EMFIError


def calibrate(port: str, samples: int, output: str, start_pos: int = 0, end_pos: int = 1023):
    """
    Perform calibration sweep.

    Args:
        port: Serial port
        samples: Number of measurements per position
        output: Output file path
        start_pos: Starting pot position (default 0)
        end_pos: Ending pot position (default 1023)
    """
    print(f"Starting calibration on {port}")
    print(f"Samples per position: {samples}")
    print(f"Position range: {start_pos}-{end_pos}")
    print()

    voltages = [0.0] * 1024

    with EMFIDevice(port) as device:
        device.initialize()
        print(f"Device: {device.device_info}")
        print()

        total_positions = end_pos - start_pos + 1
        start_time = time.time()

        for pos in range(start_pos, end_pos + 1):
            # Set potentiometer position
            device.set_pot_position(pos)

            # Start charging
            device.charge_start()
            time.sleep(0.5)  # Allow voltage to stabilize

            # Take measurements
            measurements = []
            for _ in range(samples):
                voltage = device.voltage_measured
                measurements.append(voltage)
                time.sleep(0.05)

            # Stop charging
            device.charge_stop()

            # Compute average
            avg_voltage = sum(measurements) / len(measurements)
            voltages[pos] = avg_voltage

            # Progress
            done = pos - start_pos + 1
            elapsed = time.time() - start_time
            eta = (elapsed / done) * (total_positions - done) if done > 0 else 0

            print(f"\rPosition {pos:4d}/{end_pos}: {avg_voltage:6.2f}V "
                  f"(min={min(measurements):.2f}, max={max(measurements):.2f}) "
                  f"ETA: {eta/60:.1f}min", end='', flush=True)

            # Brief pause between positions
            time.sleep(0.1)

        print("\n")
        device.safe_shutdown()

    # Save calibration
    calibration_data = {
        'created': datetime.now().isoformat(),
        'port': port,
        'samples_per_position': samples,
        'position_range': [start_pos, end_pos],
        'voltages': voltages
    }

    with open(output, 'w') as f:
        if output.endswith('.yaml') or output.endswith('.yml'):
            yaml.dump(calibration_data, f, default_flow_style=False)
        else:
            json.dump(calibration_data, f, indent=2)

    print(f"Calibration saved to {output}")

    # Summary
    non_zero = [v for v in voltages if v > 0]
    if non_zero:
        print(f"Voltage range: {min(non_zero):.2f}V - {max(non_zero):.2f}V")


def main():
    parser = argparse.ArgumentParser(description='ShoutDuino Calibration')
    parser.add_argument('--port', '-p', required=True, help='Serial port')
    parser.add_argument('--samples', '-n', type=int, default=10,
                        help='Measurements per position (default: 10)')
    parser.add_argument('--output', '-o', default='calibration.yaml',
                        help='Output file (default: calibration.yaml)')
    parser.add_argument('--start', type=int, default=0,
                        help='Start position (default: 0)')
    parser.add_argument('--end', type=int, default=1023,
                        help='End position (default: 1023)')

    args = parser.parse_args()

    try:
        calibrate(args.port, args.samples, args.output, args.start, args.end)
    except EMFIError as e:
        print(f"\nError: {e}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nCalibration cancelled.")
        sys.exit(1)


if __name__ == '__main__':
    main()
