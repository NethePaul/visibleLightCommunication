# Polarization communication
This is a proof of concept for a steganographic communication channel using polarization of light


## Encoder
### Hardware
The code is compatible with any arduino compatible microcontroller like the arduino uno or the eps32.
### Flash
The provided Makefile uses arduino-cli to compile and upload the code.
You need to install arduino-cli and run `arduino-cli core install esp32:esp32` or `arduino-cli core install arduino:avr`
Then, run `make upload` to compile and upload the code to an esp32 or `make upload_uno` to upload the code to an arduino uno.
You can also flash the code using the Ardunio IDE.
### Webserver
If the microcontroller has a WiFi module, uncomment line 3, and set the SSID and password of your router on line 84 and 85. This will enable a Webserver on the encoder, which allows dynamically changing the message via a `HTTP POST $ip:80/set?data=$new_message` request.
The IP address of the microcontroller can be found using the administration page of the connected router or by using the arduino monitor with baud 9600 via `arduino-cli monitor -p /dev/ttyUSB0`.
The number of times, that the message was sent completely can be queried with `HTTP GET $ip:80/status`, wich returns the count in plaintext.
Otherwise the message can only be changed by reflashing the microcontroller with a different hardcoded message.

### Sending
The microcontroller will continuously send the message in a loop while it is powered.

## Decoder
### Hardware
The code runs on a linux device with a camera descriptor on `/dev/video0`.
### Configuration
The polarized camera is configured via `make camera_config` which uses `cameractrls` as a dependency.
### Compilation
Run `make`, which requires opencv4 as a dependency and a c++ compiler.
### Decoding
The output contains debug information which can be filtered out via `grep 'received string:'`.
#### Live decoding
Run `bin/decoder /dev/video0` after compilation or `make live`.
#### Decoding from a video file
Run `bin/decoder video.mp4` after compilation.
#### Adjusting the minimum Threshold
Run `bin/decoder $video_source $minimum_threshold`.
