# CrowBox Parts List

Open this file with Ctrl+Shift+V (markdown preview) to get clickable checkboxes. Clicking a box in preview toggles it in the actual file. Two checkboxes per part: Ordered and Received.

---

## Phase 0/1 — Core breadboard kit

### ESP32 DevKitC (x3)
- [x] Ordered
- [x] Received

### Breadboards — 830pt + 400pt kit
- [x] Ordered
- [x] Received

### Jumper wire kit (M-M / M-F / F-F)
- [x] Ordered
- [x] Received

### Resistor kit, assorted
- [x] Ordered
- [x] Received

### USB-C cable (match your board's port)
- [x] Received

### Multimeter (AstroAI DM6000B or similar)
- [x] Ordered
- [x] Received

### Logic analyzer, 8-channel (HiLetgo)
- [x] Ordered
- [x] Received

### USB power meter (UM25C)
- [x] Ordered
- [x] Received

---

## Sensors

### Load cell 1kg + HX711 — deposit chamber (kit #1)
- [x] Ordered
- [x] Received

### Load cell 1kg + HX711 — reward chamber (kit #2, identical part)
- [x] Ordered
- [x] Received

### Load cell 5kg + HX711 (unnecessary now, bought already)

### IR beam break sensor pairs (need 4 total for hoppers — 1 already owned, 3 more)
PAUSED — deciding on approach, revisit before Phase 1 sensor work wraps up
- [ ] Ordered (x3 more)
- [ ] Received

### PIR motion sensor
- [x] Ordered
- [x] Received

---

## Actuators / drivers

### SG90 servos (x4 — hopper dosers)
- [x] Ordered
- [x] Received

### MG996R servo (2-pack — 1 for trapdoor lifter, 1 spare)
- [x] Ordered
- [x] Received

### Mini 12V linear actuator, 50mm stroke (x2 — gate lift) 
- [x] Ordered
- [x] Received

### L298N dual H-bridge driver module (for 2x linear actuators)
- [x] Ordered
- [x] Received

---

## Trapdoor mechanism

### 12V solenoid lock (Bonsicoky, deadbolt for trapdoor)
- [x] Ordered
- [x] Received

### MOSFET driver module , 2-pack (1 for solenoid, 1 for LED strip switching)
- [x] Ordered
- [x] Received

### 1N4007 flyback diode (protects solenoid MOSFET from inductive kickback)
- [x] Ordered
- [x] Received

(reuses existing deposit-chamber load cell — no new weight sensor needed)

---

## Indicators / lighting

### LED pack, assorted (have — confirm in hand; includes blue for jackpot)
- [x] Received

### Passive buzzer module (have — confirm in hand)
- [x] Received

### 5V COB LED strip (deposit chamber lighting, aids future CV)
- [x] Ordered
- [x] Received

---

## Logic / I/O expansion

### MCP23017 I2C GPIO expander breakout
- [x] Ordered
- [x] Received

---

## Power

### 12V LiFePO4 battery, 8Ah
- [x] Ordered
- [x] Received

### 12V 2A wall adapter, bare wire (for bench testing before battery is wired)
- [x] Ordered
- [x] Received

### Adjustable buck converter w/ voltmeter display, 12V→5V (2-pack — 1 for main 5V rail, 1 as a voltage-capping regulator for the actuator/solenoid line, or spare)
- [x] Ordered
- [x] Received

### Inline fuse holder, waterproof (ecocstm, 2-pack)
- [x] Ordered
- [x] Received

### ATC/ATO blade fuse assortment kit (55pc, 2A–40A — use 5A for the 12V line)
- [x] Ordered
- [x] Received

### WAGO 221 lever connector assortment (2/3/5-conductor mixed pack)
- [x] Ordered
- [x] Received

### ~~DC barrel jack to screw terminal adapter~~
Not needed — using bare wire adapter instead. Keep this line only as a fallback if bare wire ends up being impractical to terminate cleanly; if so, switch to a 5.5x2.1mm barrel adapter and revisit this part.

### ~~MB102 breadboard power supply module~~
Likely unnecessary now that the adjustable buck converter can feed the breadboard directly. Skip unless you want a simple standalone testing supply before the full power chain is wired.

---

## Phase 5 — Connectivity

### Blues Wireless Notecard (LTE-M)
- [ ] Ordered
- [ ] Received

### Notecarrier-A
- [ ] Ordered
- [ ] Received

---

## Phase 6 — Computer vision

### Raspberry Pi Zero 2 WH (headers pre-soldered)
- [ ] Ordered
- [ ] Received

### Pi Camera Module v3
- [ ] Ordered
- [ ] Received

### Pi Zero camera cable (narrow connector — different from full-size Pi)
- [ ] Ordered
- [ ] Received

### 32GB microSD card
- [ ] Ordered
- [ ] Received

### USB OTG micro adapter
- [ ] Ordered
- [ ] Received

---

## Phase 7 — Coral (optional, only after Phase 6 proven)

### Google Coral USB Accelerator
- [ ] Ordered
- [ ] Received

### P-channel MOSFET (Coral power switching)
- [ ] Ordered
- [ ] Received

----

## Soldering (buy when actually needed, not before)

### Pinecil V2 soldering iron
- [ ] Ordered
- [ ] Received

### Solder, 60/40, 0.6mm
- [ ] Ordered
- [ ] Received

### Brass tip cleaner
- [ ] Ordered
- [ ] Received