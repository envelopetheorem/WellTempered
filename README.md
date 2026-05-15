# WellTempered

A MIDI to CV and gate converter that converts to well temperament

## Hardware overview

Microcontroller: Teensy 4.0
Two MCP4921s generate the CV. The output from these are amplified in op amps and calibrated (through a trim pot) so that the final CV output spans 0-5V
Gate signals are passed through NPN transistors, and so the gate signal from the Teensy is inverted. 
