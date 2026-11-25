#pragma once

// selftest_run
// Power-on self-test for the main junction box. Checks every module it can
// measure (I2C devices, MCP23017 registers and output latches, both HX711s,
// PIR, IR beams, vision link, Notecard modem/signal/supply voltage, NVS),
// exercises the outputs it can't measure (LEDs, buzzer, COB strip, solenoid,
// servos, gate) so you can watch/listen, then sends a summary note to Notehub
// (file "selftest.qo") and prints a report on serial.
// Ends with the green LED on (all passed) or red LED on (something failed).
// Returns when done. Call it from app_main() before (or instead of) normal startup.
void selftest_run(void);
