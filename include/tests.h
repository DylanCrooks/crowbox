#pragma once

// Standalone hardware test functions — each runs an infinite loop.
// Call exactly one from app_main() after all inits, then flash and observe.

void test_mcp23017_leds(void);     // blinks green/red/blue LEDs on MCP23017 port A
void test_weight_sensors(void);    // logs raw + grams for both HX711s every second
void test_solenoid_and_servo(void); // solenoid on/off every 10s, MG996R sweep every 5s
void test_gate(void);              // open/close linear actuators, log travel time
