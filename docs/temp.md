Structural Load Isolation (Zero-Torque Path)
Idle State: When closed, the trapdoor's Lock Tab rests on a hardened steel solenoid pin. 100% of downward slamming forces or crow pecking bypasses the servo entirely. 
Drop Mechanism: A single high-torque servo is attached via a linkage to lift the door only. Before dropping, the servo is electronically detached (limp state).
 The solenoid pulls back, gravity drops the door instantly, and the servo gears spin freely without resistance.
 Component Shopping List
 Weight Sensor & Amp: one of previously bought x711 + load cell. 
 The Lifter (Servo): MG996R 
The Deadbolt (Solenoid): 12V DC Miniature Solenoid Cabinet Lock (Bonsicoky DC 12V 0.43A Electromagnetic Solenoid Lock, Mini Slim Electric Door Latch for Cabinet Door Drawer)
Actuation Drivers: IRF520 MOSFET Driver Module (to allow 3.3V ESP32 logic to switch the 12V solenoid line).
Chamber Lighting: 5V COB LED Strip Light (for continuous, diffusion-free uniform lighting to aid CV edge-detection). Gonna strip it and attach to WAGO connector, maybe?
ESP-IDF Software RequirementsMCPWM Peripheral Control: Emulate servo.detach() behavior by using mcpwm_set_signal_low() to drop the hardware PWM generation line right before unlatching, forcing the servo into an unpowered, free-spinning state.
FreeRTOS Task Isolation: A dedicated, blocking task to handle hx711_read_average() sampling over a 2-wire serial interface (DOUT/PD_SCK), which unblocks and fires an execution notification to the camera task when a weight delta threshold is broken.
