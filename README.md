# ArduinoMqttDmx

Project for having an MQTT to DMX bridge.
This project is tested with the home assistant MQTT light.

This repository exists out of 2 parts: software implementation in Arduino and PCB drawing in KiCad

## Software
The software is written in Arduino. The serial registers are written directly.
The sofware can be found in the folder 'Arduino/Dmx'
Needed library:
 - pubsubclient

Before uploading this software, a serial number need to be uploaded in EEPROM. This way of programming gives every arduino a fixed number, without the constant need of changing the software. 
 Fill in the ip, username and password of your broker in the code. 

This bridge supports up to 50 DMX channels. Enlarging the buffer size is possible, but I had some unstabilities. 
The max485 is connected to pin D1. 

The mqtt topic the arduino connects to is 'D0000/X/command' where D0000 will be overwritten with the serial number, found in the EEPROM and x the channel that need to be changed.

The payload of the MQTT messages should have the following structure:

{
  "state": ON,
  "brightness": 127,
  "transition": 5
}

with:
  state is ON or OFF,
  the brighness is a value between 0 and 255
  the transition is a value in seconds


