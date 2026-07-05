#pragma once

// gate_open
// Extend both linear actuators to raise the gate sleeve.
// Set MCP_BIT_ACT_A_IN1=1, MCP_BIT_ACT_A_IN2=0 (actuator A forward)
// Set MCP_BIT_ACT_B_IN1=1, MCP_BIT_ACT_B_IN2=0 (actuator B forward)
// vTaskDelay for travel time (placeholder — tune on hardware), then gate_stop().
void gate_open(void);

// gate_close
// Retract both linear actuators to lower the gate sleeve.
// Set IN1=0, IN2=1 for both actuators (reverse direction).
// vTaskDelay for travel time, then gate_stop().
void gate_close(void);

// gate_stop
// Set all 4 direction pins LOW. Cuts power to both actuators.
// Always call this after travel — don't leave motor stalled against end stop.
void gate_stop(void);
