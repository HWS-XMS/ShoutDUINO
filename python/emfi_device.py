"""
ShoutDuino EM-FI Tool Python API

Python interface for controlling the ShoutDuino electromagnetic fault injection
charging circuit via UART binary protocol.

Usage:
    from emfi_device import EMFIDevice

    with EMFIDevice('/dev/ttyACM0') as device:
        device.initialize()
        device.pot_position = 500        # Set voltage via potentiometer
        device.charging = True           # Start charging
        print(device.voltage_measured)   # Read actual voltage
        print(device.temperatures)       # Read temperature sensors
        device.safe_shutdown()
"""

import serial
import struct
import time
import logging
from typing import Optional, List
from enum import IntEnum
from dataclasses import dataclass


class EMFIError(Exception):
    """Base exception for EM-FI device errors."""
    pass


class EMFICommError(EMFIError):
    """Communication protocol error."""
    pass


class EMFIHardwareError(EMFIError):
    """Hardware fault or safety error."""
    pass


class EMFIValidationError(EMFIError):
    """Parameter validation error."""
    pass


class EMFITemperatureInitError(EMFIHardwareError):
    """Temperature sensor initialization error."""
    def __init__(self, error_code: int):
        self.error_code = error_code
        error_messages = {
            0x01: "Primary NMOS sensor initialization failed",
            0x02: "Primary NMOS sensor alert threshold setup failed",
            0x03: "Primary NMOS sensor alert polarity setup failed",
            0x04: "Gate driver sensor initialization failed",
            0x05: "Gate driver sensor alert threshold setup failed",
            0x06: "Gate driver sensor alert polarity setup failed",
            0x07: "Transformer sensor initialization failed",
            0x08: "Transformer sensor alert threshold setup failed",
            0x09: "Transformer sensor alert polarity setup failed",
            0x0A: "Discharge NMOS sensor initialization failed",
            0x0B: "Discharge NMOS sensor alert threshold setup failed",
            0x0C: "Discharge NMOS sensor alert polarity setup failed",
        }
        message = error_messages.get(error_code, f"Unknown temperature error code: 0x{error_code:02X}")
        super().__init__(f"Temperature initialization failed: {message} (error code: 0x{error_code:02X})")
        self.sensor_name = self._get_sensor_name(error_code)
        self.failure_type = self._get_failure_type(error_code)

    def _get_sensor_name(self, code: int) -> str:
        """Extract sensor name from error code."""
        if 0x01 <= code <= 0x03:
            return "Primary NMOS"
        elif 0x04 <= code <= 0x06:
            return "Gate Driver"
        elif 0x07 <= code <= 0x09:
            return "Transformer"
        elif 0x0A <= code <= 0x0C:
            return "Discharge NMOS"
        return "Unknown"

    def _get_failure_type(self, code: int) -> str:
        """Extract failure type from error code."""
        offset = (code - 1) % 3
        if offset == 0:
            return "initialization"
        elif offset == 1:
            return "alert_threshold"
        elif offset == 2:
            return "alert_polarity"
        return "unknown"


class UARTCommand(IntEnum):
    """UART command codes matching firmware protocol."""
    # System Commands
    INIT = 0x01
    GET_STATUS = 0x02
    GET_DEVICE_INFO = 0x03
    
    # Charging Commands
    CHARGE_START = 0x10
    CHARGE_STOP = 0x11
    SET_VOLTAGE = 0x12
    GET_VOLTAGE = 0x13
    GET_VOLTAGE_MEASURED = 0x14
    SET_POT_POSITION = 0x15
    GET_POT_POSITION = 0x16
    GET_ADC_DEBUG = 0x17
    GET_ADC_SW = 0x18

    # Gate Driver Commands
    SET_GATE_DRIVER_EN = 0x20
    GET_GATE_DRIVER_EN = 0x21
    
    # Trigger Commands
    SET_TRIGGER_EN = 0x22
    GET_TRIGGER_EN = 0x23
    SOFT_TRIGGER = 0x24
    SET_SOFT_TRIGGER_WIDTH = 0x25
    GET_SOFT_TRIGGER_WIDTH = 0x26
    
    # Temperature Commands
    READ_TEMPERATURES = 0x30
    SET_TEMP_LIMITS = 0x31


class UARTResponse(IntEnum):
    """UART response codes matching firmware protocol."""
    OK = 0x00
    ERROR = 0x01
    INVALID_CMD = 0x02
    INVALID_CRC = 0x03
    BUSY = 0x04
    FAULT = 0x05


@dataclass
class DeviceStatus:
    """Device status information."""
    charging: bool
    gate_driver_enabled: bool
    trigger_enabled: bool
    fault_detected: bool
    temperatures: List[float]  # [primary_nmos, gate_driver, transformer, discharge_nmos]
    voltage_measured: float
    pot_position: int
    platform: str


class EMFIDevice:
    """
    Main interface for ShoutDuino EM-FI device.
    
    This class provides a high-level Python API with properties and methods
    for controlling electromagnetic fault injection operations.
    """
    
    # Hardware limits
    TEMP_LIMIT_MAX = 150.0  # Maximum safe temperature

    # Protocol constants
    FRAME_START = 0xAA
    FRAME_END = 0x55
    MAX_DATA_LENGTH = 64
    TIMEOUT_SECONDS = 5.0
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 5.0):
        """
        Initialize EM-FI device interface.
        
        Args:
            port: Serial port path (e.g., '/dev/ttyUSB0', 'COM3')
            baudrate: Serial communication speed (default: 115200)
            timeout: Communication timeout in seconds (default: 5.0)
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial: Optional[serial.Serial] = None

        # Cached state
        self._device_info: Optional[str] = None
        self._temperature_limits: List[float] = [80.0, 80.0, 80.0, 80.0]  # Default limits

        # Setup logging
        self.logger = logging.getLogger(f"EMFIDevice({port})")
        
    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
        
    def connect(self) -> None:
        """Establish serial connection to device."""
        try:
            self.serial = serial.Serial(
                self.port,
                self.baudrate,
                timeout=self.timeout,
                write_timeout=self.timeout
            )
            self.logger.info(f"Connected to {self.port} at {self.baudrate} baud")
            
            # Clear any pending data
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
            
        except serial.SerialException as e:
            raise EMFICommError(f"Failed to connect to {self.port}: {e}")
    
    def disconnect(self) -> None:
        """Close serial connection."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            self.logger.info("Disconnected from device")
    
    @property
    def is_connected(self) -> bool:
        """Check if device is connected."""
        return self.serial is not None and self.serial.is_open
    
    def _calculate_crc16(self, data: bytes) -> int:
        """Calculate CRC16 checksum (CCITT polynomial)."""
        crc = 0
        for byte in data:
            crc ^= byte << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc = crc << 1
                crc &= 0xFFFF
        return crc
    
    def _send_frame(self, command: UARTCommand, data: bytes = b'') -> bytes:
        """
        Send a binary frame and receive response.
        
        Args:
            command: Command code
            data: Optional payload data
            
        Returns:
            Response payload data
            
        Raises:
            EMFICommError: Communication error
            EMFIHardwareError: Hardware fault
        """
        if not self.is_connected:
            raise EMFICommError("Device not connected")
        
        if len(data) > self.MAX_DATA_LENGTH:
            raise EMFIValidationError(f"Data length {len(data)} exceeds maximum {self.MAX_DATA_LENGTH}")
        
        # Build frame
        frame_header = struct.pack('<BBB', self.FRAME_START, command, len(data))
        frame_data = frame_header + data
        crc = self._calculate_crc16(frame_data)
        frame = frame_data + struct.pack('>HB', crc, self.FRAME_END)
        
        # Send frame
        try:
            self.serial.write(frame)
            self.serial.flush()
            self.logger.debug(f"Sent command 0x{command:02X} with {len(data)} bytes data")
        except serial.SerialException:
            raise EMFICommError("Timeout sending command")
        
        # Read response
        response_data = self._read_response()
        return response_data
    
    def _read_response(self) -> bytes:
        """Read and validate response frame."""
        try:
            # Read response with proper frame assembly
            time.sleep(0.005)  # Brief delay for Arduino to start responding
            
            # Read header first
            header = self.serial.read(3)
            if len(header) < 3:
                raise EMFICommError("Failed to read response header")
            
            data_length = header[2]
            # Read exact amount: data + CRC + end
            remaining = self.serial.read(data_length + 3)
            if len(remaining) < data_length + 2:  # At least need data + CRC (missing end byte is common)
                raise EMFICommError(f"Incomplete response: expected {data_length + 3} bytes, got {len(remaining)}")
            elif len(remaining) == data_length + 2:
                # Missing end byte - append it
                remaining += bytes([self.FRAME_END])
            
            response_bytes = header + remaining
            
            if len(response_bytes) < 6:
                raise EMFICommError("Response too short")
                
            start_byte = response_bytes[0]
            if start_byte != self.FRAME_START:
                raise EMFICommError("Invalid response start byte")
                
            response_code = response_bytes[1]
            data_length = response_bytes[2]
            
            expected_len = 3 + data_length + 2 + 1  # header + data + crc + end
            # Handle case where response might be 1 byte shorter due to buffering
            if len(response_bytes) < expected_len - 1:
                raise EMFICommError(f"Response too short: got {len(response_bytes)}, expected {expected_len}")
            
            # Parse the complete frame
            data = response_bytes[3:3+data_length] if data_length > 0 else b''
            crc_bytes = response_bytes[3+data_length:5+data_length]
            end_byte_val = response_bytes[5+data_length]
            
            if end_byte_val != self.FRAME_END:
                raise EMFICommError("Invalid response end byte")
            
            # Debug frame parsing
            self.logger.debug(f"Response bytes: {response_bytes.hex()}")
            self.logger.debug(f"Data: {data.hex()}")
            self.logger.debug(f"CRC bytes: {crc_bytes.hex()}")
            
            # Verify CRC
            frame_data = bytes([self.FRAME_START, response_code, data_length]) + data
            expected_crc = self._calculate_crc16(frame_data)
            actual_crc = struct.unpack('>H', crc_bytes)[0]
            
            self.logger.debug(f"Frame for CRC: {frame_data.hex()}")
            self.logger.debug(f"Expected CRC: 0x{expected_crc:04X}, Actual: 0x{actual_crc:04X}")
            
            if actual_crc != expected_crc:
                raise EMFICommError(f"CRC mismatch: expected 0x{expected_crc:04X}, got 0x{actual_crc:04X}")
            
            # Handle response codes
            if response_code == UARTResponse.OK:
                self.logger.debug(f"Command successful, received {len(data)} bytes data")
                return data
            elif response_code == UARTResponse.ERROR:
                # Check if error data contains a temperature error code
                if len(data) == 1:
                    error_code = data[0]
                    if 0x01 <= error_code <= 0x0C:
                        raise EMFITemperatureInitError(error_code)
                raise EMFICommError(f"Command error (0x{data.hex() if data else 'no data'})")
            elif response_code == UARTResponse.FAULT:
                raise EMFIHardwareError("Hardware fault detected")
            elif response_code == UARTResponse.BUSY:
                raise EMFICommError("Device busy")
            elif response_code == UARTResponse.INVALID_CMD:
                raise EMFICommError("Invalid command")
            elif response_code == UARTResponse.INVALID_CRC:
                raise EMFICommError("Invalid CRC in command")
            else:
                raise EMFICommError(f"Command failed with code 0x{response_code:02X}")
                
        except serial.SerialException:
            raise EMFICommError("Timeout reading response")
    
    # System Commands
    
    def initialize(self) -> None:
        """Initialize device with default settings."""
        self._send_frame(UARTCommand.INIT)
        self.logger.info("Device initialized")
        
        # Clear cached values to force refresh
        self._target_voltage = None
        self._pulse_width_ns = None
        self._device_info = None
    
    @property
    def device_info(self) -> str:
        """Get device platform information."""
        if self._device_info is None:
            response = self._send_frame(UARTCommand.GET_DEVICE_INFO)
            self._device_info = response.decode('utf-8', errors='ignore')
        return self._device_info
    
    @property
    def status(self) -> DeviceStatus:
        """Get comprehensive device status."""
        response = self._send_frame(UARTCommand.GET_STATUS)

        # Parse flags byte
        flags = response[0]
        charging = bool(flags & 0x01)
        fault = bool(flags & 0x02)
        done = bool(flags & 0x04)
        gate_en = bool(flags & 0x08)
        trig_en = bool(flags & 0x10)
        temp_alert = bool(flags & 0x20)

        # Parse other fields
        pot_pos = struct.unpack('<H', response[1:3])[0]
        voltage = struct.unpack('<f', response[3:7])[0]
        pulse_ns = struct.unpack('<I', response[7:11])[0]
        temps = list(struct.unpack('<ffff', response[11:27]))

        return DeviceStatus(
            charging=charging,
            gate_driver_enabled=gate_en,
            trigger_enabled=trig_en,
            fault_detected=fault,
            temperatures=temps,
            voltage_measured=voltage,
            pot_position=pot_pos,
            platform=self._device_info or "Unknown"
        )
    
    # Charging Commands

    @property
    def charging(self) -> bool:
        """Get/set charging state."""
        return self.status.charging

    @charging.setter
    def charging(self, enabled: bool) -> None:
        """Start or stop charging."""
        if enabled:
            self._send_frame(UARTCommand.CHARGE_START)
            self.logger.info("Charging started")
        else:
            self._send_frame(UARTCommand.CHARGE_STOP)
            self.logger.info("Charging stopped")

    @property
    def voltage_measured(self) -> float:
        """Get measured high voltage in volts."""
        response = self._send_frame(UARTCommand.GET_VOLTAGE_MEASURED)
        return struct.unpack('<f', response)[0]
    
    # Gate Driver Commands
    
    @property
    def gate_driver_enabled(self) -> bool:
        """Get/set gate driver enable state."""
        response = self._send_frame(UARTCommand.GET_GATE_DRIVER_EN)
        return bool(response[0])
    
    @gate_driver_enabled.setter
    def gate_driver_enabled(self, enabled: bool) -> None:
        """Set gate driver enable state."""
        data = bytes([1 if enabled else 0])
        self._send_frame(UARTCommand.SET_GATE_DRIVER_EN, data)
        self.logger.info(f"Gate driver {'enabled' if enabled else 'disabled'}")
    
    # Trigger Commands
    
    @property
    def trigger_enabled(self) -> bool:
        """Get/set trigger enable state."""
        response = self._send_frame(UARTCommand.GET_TRIGGER_EN)
        return bool(response[0])
    
    @trigger_enabled.setter
    def trigger_enabled(self, enabled: bool) -> None:
        """Set trigger enable state."""
        data = bytes([1 if enabled else 0])
        self._send_frame(UARTCommand.SET_TRIGGER_EN, data)
        self.logger.info(f"Trigger {'enabled' if enabled else 'disabled'}")

    # Temperature Commands
    
    @property
    def temperatures(self) -> List[float]:
        """Get all temperature readings in Celsius."""
        response = self._send_frame(UARTCommand.READ_TEMPERATURES)
        temps = struct.unpack('<ffff', response)  # 4 floats
        return list(temps)

    @property
    def temperature_limits(self) -> List[float]:
        """Get/set temperature alert limits for all sensors."""
        return self._temperature_limits

    @temperature_limits.setter
    def temperature_limits(self, limits: List[float]) -> None:
        """Set temperature alert limits for all sensors."""
        if len(limits) != 4:
            raise EMFIValidationError("Must provide exactly 4 temperature limits")

        for i, limit in enumerate(limits):
            if not (0.0 <= limit <= self.TEMP_LIMIT_MAX):
                raise EMFIValidationError(
                    f"Temperature limit {i} ({limit}°C) outside range 0-{self.TEMP_LIMIT_MAX}°C"
                )

        data = struct.pack('<ffff', *limits)
        self._send_frame(UARTCommand.SET_TEMP_LIMITS, data)
        self._temperature_limits = limits
        self.logger.info(f"Temperature limits set: {limits}")

    # Potentiometer Commands

    @property
    def pot_position(self) -> int:
        """Get/set potentiometer position (0-1023)."""
        response = self._send_frame(UARTCommand.GET_POT_POSITION)
        return struct.unpack('<H', response)[0]

    @pot_position.setter
    def pot_position(self, position: int) -> None:
        """Set potentiometer position (45-1023). Values outside range are clamped."""
        if position < 45:
            self.logger.warning(f"Position {position} clamped to 45 (exceeds safe voltage limit)")
            position = 45
        if position > 1023:
            self.logger.warning(f"Position {position} clamped to 1023 (max value)")
            position = 1023
        data = struct.pack('<H', position)
        self._send_frame(UARTCommand.SET_POT_POSITION, data)
        self.logger.debug(f"Potentiometer position set to {position}")

    def get_adc_debug(self) -> dict:
        """
        Get raw ADC debug info for SPI troubleshooting.

        Returns:
            dict with 'hi', 'lo', 'raw', and 'voltage' keys
        """
        response = self._send_frame(UARTCommand.GET_ADC_DEBUG)
        hi = response[0]
        lo = response[1]
        raw = struct.unpack('<H', response[2:4])[0]

        # Calculate voltage from raw value
        vref = 3.279
        divider_ratio = (5000000 + 25000) / 25000  # 201
        voltage = (raw / 4096.0) * vref * divider_ratio

        return {
            'hi': hi,
            'lo': lo,
            'raw': raw,
            'voltage': voltage
        }

    def get_adc_sw(self) -> dict:
        """
        Get ADC value using software SPI (bit-banged).
        Use to compare with hardware SPI to diagnose timing issues.

        Returns:
            dict with 'raw' and 'voltage' keys
        """
        response = self._send_frame(UARTCommand.GET_ADC_SW)
        raw = struct.unpack('<H', response[0:2])[0]
        voltage = struct.unpack('<f', response[2:6])[0]

        return {
            'raw': raw,
            'voltage': voltage
        }

    def safe_shutdown(self) -> None:
        """Safely shutdown all operations."""
        try:
            self.trigger_enabled = False
            self.gate_driver_enabled = False
            self.charging = False
            self.logger.info("Safe shutdown completed")
        except Exception as e:
            self.logger.error(f"Error during safe shutdown: {e}")