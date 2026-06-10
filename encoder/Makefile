
default: upload

.PHONY: compile upload
.PHONY: compile_uno upload_uno

compile:
	# arduino-cli compile -b arduino:avr:uno 
	arduino-cli compile -b esp32:esp32:esp32

upload:
	arduino-cli compile -b esp32:esp32:esp32 -p /dev/ttyUSB0 --upload


compile_uno:
	arduino-cli compile -b arduino:avr:uno

upload_uno:
	arduino-cli compile -b arduino:avr:uno -p /dev/ttyACM0 --upload
