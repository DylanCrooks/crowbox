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


unsure what the enums and structs are actually for, in general. i need to create that mermaid graph - how? also how to create wiring diagrams? i'll also probably work on getting a CAD design of the physical project tomorrow, at least a simple one. couldn't find the datasheet for the solenoid and actuators but fuck it, they can't be like that. also don't the actuators step down voltage by 2V anyway? idk, if the 12.8V fucks up the solenoid or actuators i'll buy more. also i'm doing bare wire 12V 2A wall adapter. can you verify gemini saying i'll need 5 wago connectors? "For your exact system layout, you will need four 5-wire WAGO connectors in total.

Since they are rarely sold individually, you should just buy a standard 10-pack or 25-pack of WAGO 221 (5-conductor) connectors. Having extras is always helpful for prototyping when things change.

The Master Planning Breakdown

Here is why your count sits exactly at four blocks, divided by your power lines:

1. The 12V Power Line (Requires 1 WAGO)

This block splits the incoming 12V wall power to the components that need high voltage.



* Slot 1: Incoming 12V from the wall adapter

* Slot 2: Out to the L298N driver (for your 2 linear actuators)

* Slot 3: Out to the Latch Solenoid MOSFET

* Slot 4: Out to the Buck Converter input (to make 5V)

* Slot 5: Empty spare

2. The 5V Power Line (Requires 1 WAGO)

This block takes the safe 5V coming out of your buck converter and distributes it to your logic and low-voltage motors.



* Slot 1: Incoming 5V from the Buck Converter output

* Slot 2: Out to the ESP32 `VIN` pin to power the brain

* Slot 3: Out to the heavy-duty MG996R trapdoor servo

* Slot 4: Out to the breadboard power rail (for your 4 smaller SG90 hopper servos)

* Slot 5: Out to the 5V COB LED strip lighting array

3. The Ground / Negative Line (Requires 2 WAGOs)

Because every single component in your entire machine must share a common ground connection to prevent electrical noise and glitching, your ground line has the most wires.

A single 5-wire WAGO isn't big enough to hold 8 separate ground wires, so you must hook two WAGO blocks together in a chain (using a short jumper wire to link them), giving you a total of 8 available connection slots:



* Slot 1: Wall adapter negative (-)

* Slot 2: L298N ground

* Slot 3: Solenoid MOSFET ground

* Slot 4: LED strip MOSFET ground

* Slot 5: Buck converter input ground

* Slot 6: Buck converter output ground

* Slot 7: ESP32 ground pin

* Slot 8: Servo array ground rail"