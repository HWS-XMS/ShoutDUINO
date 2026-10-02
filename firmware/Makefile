# shoutduino EM-FI Shield Firmware Makefile

PROJECT_NAME 		= shoutduino
SKETCH_FILE 		= firmware.ino
ARDUINO_CLI 		= arduino-cli
BOARD_UNO 			= arduino:avr:uno
BOARD_UNO_R4_WIFI 	= arduino:renesas_uno:unor4wifi
BOARD_UNO_R4_MINIMA = arduino:renesas_uno:minima

# Auto-detect connected Arduino board
ifndef BOARD
	# Extract FQBN directly from arduino-cli board list (it's in the 10th field)
	DETECTED_FQBN = $(shell $(ARDUINO_CLI) board list | grep -i arduino | grep -v "Unknown" | head -1 | awk '{print $$10}')
	ifneq ($(DETECTED_FQBN),)
		BOARD = $(DETECTED_FQBN)
	else
		# No Arduino detected, default to classic Uno
		BOARD = $(BOARD_UNO)
	endif
endif

# Auto-detect port
ifndef PORT
	PORT = $(shell $(ARDUINO_CLI) board list | grep -i arduino | head -1 | awk '{print $$1}')
endif

SRC_DIR 	= src
BUILD_DIR 	= build
SKETCH_DIR 	= .

# Default Target
all: compile

# Check if arduino-cli is installed
check-arduino-cli:
	@which $(ARDUINO_CLI) > /dev/null || (echo "Error: arduino-cli not found. Install from https://arduino.github.io/arduino-cli/" && exit 1)

# Install required cores
install-cores: check-arduino-cli
	@echo "Installing Arduino cores..."
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install arduino:avr
	$(ARDUINO_CLI) core install arduino:renesas_uno

# Compile sketch
compile: check-arduino-cli $(SKETCH_FILE)
	@echo "Compiling for $(BOARD)..."
	$(ARDUINO_CLI) compile --fqbn $(BOARD) $(BUILD_FLAGS) $(SKETCH_DIR)

# Upload to Arduino
upload: check-arduino-cli
	@if [ -z "$(PORT)" ]; then \
		echo "Auto-uploading to $(BOARD)..."; \
		$(ARDUINO_CLI) upload --fqbn $(BOARD) $(SKETCH_DIR); \
	else \
		echo "Uploading to $(BOARD) on $(PORT)..."; \
		$(ARDUINO_CLI) upload --fqbn $(BOARD) --port $(PORT) $(SKETCH_DIR); \
	fi

# Compile and upload
flash: compile upload

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -rf $(SKETCH_DIR)/build

# Mock mode targets
mock: 
	$(MAKE) compile MOCK=1

# Board-specific targets
uno: 
	$(MAKE) compile BOARD=$(BOARD_UNO)

uno-r4-wifi:
	$(MAKE) compile BOARD=$(BOARD_UNO_R4_WIFI)

uno-r4-minima:
	$(MAKE) compile BOARD=$(BOARD_UNO_R4_MINIMA)

# Test compilation for all boards
test-all: uno uno-r4-wifi uno-r4-minima
	@echo "Successfully compiled for all supported boards!"

# Detect connected Arduino
detect:
	@echo "Scanning for connected Arduino boards..."
	@$(ARDUINO_CLI) board list
	@echo ""
	@echo "Auto-detected board: $(BOARD)"
	@if [ -n "$(PORT)" ]; then echo "Auto-detected port: $(PORT)"; else echo "No Arduino detected on any port"; fi

# Print board info
board-info:
	@echo "Available boards:"
	@echo "  make uno           - Arduino Uno (ATmega328P)"
	@echo "  make uno-r4-wifi   - Arduino Uno R4 WiFi (RA4M1)"
	@echo "  make uno-r4-minima - Arduino Uno R4 Minima (RA4M1)"
	@echo ""
	@echo "Debug modes:"
	@echo "  make mock          - Compile with mock hardware mode"
	@echo ""
	@echo "Current board: $(BOARD)"
	@if [ -n "$(PORT)" ]; then echo "Current port: $(PORT)"; fi

# Generate Documentation
docs:
	@echo "Generating documentation..."
	@cd docs && doxygen Doxyfile

# Print Variables (for debugging Makefile)
print-%:
	@echo '$*=$($*)'

.PHONY: all compile upload flash clean uno uno-r4-wifi uno-r4-minima test-all board-info docs install-cores check-arduino-cli print-%