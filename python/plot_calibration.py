#!/usr/bin/env python3
"""Plot calibration data."""
import yaml
import matplotlib.pyplot as plt

with open('calibration.yaml') as f:
    data = yaml.safe_load(f)

voltages = data['voltages']
positions = list(range(len(voltages)))

plt.figure(figsize=(10, 6))
plt.plot(positions, voltages)
plt.xlabel('Pot Position')
plt.ylabel('Voltage (V)')
plt.title('Pot Position vs Voltage')
plt.grid(True)
plt.savefig('calibration_plot.png', dpi=150)
plt.show()
