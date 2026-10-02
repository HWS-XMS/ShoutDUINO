#!/usr/bin/env python3
"""
Example EM-FI experiment combining DelayUnit and ShoutDuino.
"""

import sys
sys.path.insert(0, '../DelayUnit/python')

from decimal import Decimal
from delay_unit import DelayUnit, EdgeType
from emfi_device import EMFIDevice


def target_operation() -> bool:
    """
    Perform target operation and check for fault.
    Returns True if fault was induced successfully.
    """
    # TODO: Implement target communication
    # Example: send command, check for unexpected response
    return False


def run_experiment(
    emfi: EMFIDevice,
    delay: DelayUnit,
    pot_range: range,
    delay_range_ns: range,
    on_glitch: callable = None
):
    """
    Parameter sweep over voltage (pot position) and delay.

    Args:
        emfi: EMFIDevice instance
        delay: DelayUnit instance
        pot_range: Range of pot positions to test
        delay_range_ns: Range of delays in nanoseconds
        on_glitch: Callback(pot, delay_ns) called on successful fault
    """
    for pot in pot_range:
        emfi.pot_position = pot
        emfi.charging = True

        for delay_ns in delay_range_ns:
            delay.delay = Decimal(delay_ns) / Decimal(1_000_000_000)
            delay.arm()

            # Trigger and check result
            if target_operation():
                if on_glitch:
                    on_glitch(pot, delay_ns)

        emfi.charging = False


def main():
    with EMFIDevice('/dev/ttyACM0') as emfi, DelayUnit() as delay:
        emfi.initialize()

        # Configure delay unit
        delay.edge = EdgeType.RISING
        delay.width = Decimal('0.000000100')  # 100ns output pulse

        # Run parameter sweep
        run_experiment(
            emfi=emfi,
            delay=delay,
            pot_range=range(100, 200, 10),
            delay_range_ns=range(0, 1000, 50),
            on_glitch=lambda pot, ns: print(f"Glitch at pot={pot}, delay={ns}ns")
        )

        emfi.safe_shutdown()


if __name__ == '__main__':
    main()
