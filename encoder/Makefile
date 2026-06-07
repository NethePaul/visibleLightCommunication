
default: upload

.PHONY: compile upload

compile:
	# arduino-cli compile -b arduino:avr:uno 
	arduino-cli compile -b esp32:esp32:esp32

upload:
	arduino-cli compile -b esp32:esp32:esp32 -p /dev/ttyUSB0 --upload
