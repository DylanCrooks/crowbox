# CrowBox Parts List

Open this file with Ctrl+Shift+V (markdown preview) to get clickable checkboxes. Clicking a box in preview toggles it in the actual file. Two checkboxes per part: Ordered and Received.

Last big order: 2026-10-17 (frame, fasteners, enclosures, electronics, Blues, tools). Items not ordered yet are grouped at the end.

---

## Phase 0/1 — Core breadboard kit

### ESP32 DevKitC (x3)
38-pin HiLetgo boards (ESP-WROOM-32).
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

## Frame (2020 aluminum extrusion)

### Misumi HFS5-2020 extrusion, cut to length — 35 pcs
1000mm ×4, 490mm ×5, 450mm ×5, 380mm ×8, 170mm ×4, 150mm ×9.
- [x] Ordered
- [ ] Received

### 2028 corner bracket kits (BLCCLOY 20-set) ×4 — 80 brackets
Each bracket comes with 2 T-nuts, 2 M5×10 screws and 2 washers.
- [x] Ordered
- [x] Received

### M5 drop-in T-nuts for 6mm-slot 2020 — 3 packs, 180 pcs
- [x] Ordered
- [x] Received

---

## Fasteners (Albany County Fasteners, A2 stainless, metric)

Spacers: the McMaster spacers are dropped. Front panel standoffs = a stack of 5 M5 washers each. Load cell spacers = 6 M4 washers (5mm) and 3 M4 hex nuts + 3 M4 washers (11mm).

### M5 button head socket cap screws
×8mm (10), ×10mm (100), ×12mm (100), ×16mm (30), ×20mm (5), ×25mm (6), ×50mm (15).
- [x] Ordered
- [ ] Received

### M4 button head socket cap screws
×16mm (6), ×20mm (10).
- [x] Ordered
- [ ] Received

### M5 flat washers, DIN 125A, 5.3×10×1.0 (225)
- [x] Ordered
- [ ] Received

### M5 fender washers, 5.3×15×1.2 (25)
- [x] Ordered
- [ ] Received

### M4 flat washers, 4.3×9×0.8 (43)
- [x] Ordered
- [ ] Received

### M5 nylon lock nuts (10)
- [x] Ordered
- [ ] Received

### M4 nylon lock nuts (10)
- [x] Ordered
- [ ] Received

### M5 hex nuts (10)
- [x] Ordered
- [ ] Received

### M4 hex nuts (need at least 6)
- [x] Ordered
- [ ] Received

### M5×75 hex bolts ×4 (skirt)
Not in the Albany order — buying at the hardware store.
- [ ] Ordered
- [ ] Received

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

### ~~SG90 servos (x4 — hopper dosers)~~
Replaced by MG996R — SG90s are too weak to turn the doser rotor. Already bought; keep as bench spares.

### MG996R servo (2-pack — 1 for trapdoor lifter, 1 spare)
- [x] Ordered
- [x] Received

### MG996R servos, 4-pack (hopper dosers, replacing the SG90s)
Each drives a notched rotor under an inverted Gatorade-bottle hopper. Comes with its own 25T horns — SG90 horns don't fit. With the 2-pack that makes 6 total: 3 dosers + 1 trapdoor + 2 spares (one for hopper 4 later).
- [x] Ordered
- [x] Received

### Mini 12V linear actuator, 50mm stroke (x2 — gate lift) 
- [x] Ordered
- [x] Received

### L298N dual H-bridge driver module (for 2x linear actuators)
- [x] Ordered
- [x] Received

### AOD4184 MOSFET modules, optocoupler-isolated, 5-pack (replaces the IRF520 modules)
For the solenoid and the LED strip. Opto-isolated version: wire the module's input GND to the MCP23017's GND, and confirm on the bench that a 3.3V signal switches it reliably.
- [x] Ordered
- [x] Received

---

## Trapdoor mechanism

### 12V solenoid lock (Bonsicoky, deadbolt for trapdoor)
- [x] Ordered
- [x] Received

### MOSFET driver module , 2-pack (1 for solenoid, 1 for LED strip switching)
IRF520 modules — being replaced by the AOD4184 modules (Actuators / drivers). Keep as spares.
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

### ESP32 38-pin screw-terminal breakout board
Fits both 0.9" and 1.0" pin-row spacing (DevKitC-style). Gets the ESP32 off the breadboard.
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

### XL4015 5A buck converter with voltmeter (HiLetgo)
For the servo rail, if an LM2596 isn't enough.
- [x] Ordered
- [x] Received

### MP1584EN mini buck converters (HiLetgo pack)
One goes in the vision box (12V → 5V for the P4 board and the PIR).
- [x] Ordered
- [x] Received

### Electrolytic capacitor assortment kit (ALLECIN)
Includes the bulk capacitors for the servo rail and the Notecarrier.
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

### WAGO 221 lever-nut assortment (brings the total to 12× 5-port 221-415)
- [x] Ordered
- [x] Received

### 1N5819 Schottky diodes (already owned)
- [x] Received

### ~~DC barrel jack to screw terminal adapter~~
Not needed — using bare wire adapter instead. Keep this line only as a fallback if bare wire ends up being impractical to terminate cleanly; if so, switch to a 5.5x2.1mm barrel adapter and revisit this part.

### ~~MB102 breadboard power supply module~~
Likely unnecessary now that the adjustable buck converter can feed the breadboard directly. Skip unless you want a simple standalone testing supply before the full power chain is wired.

---

## Enclosures

### Main junction box — hinged IP65 ABS, clear PC lid, 290×190×140mm
Interior 264×166mm. Includes a perforated ABS mounting board (245×148×4mm) and 4 mounting brackets. Clear lid: keep it shaded, since sun heats a clear box.
- [x] Ordered
- [ ] Received

(Cable glands are in the wiring order — see "Not ordered yet".)

---

## Vision box — ESP32-P4 board, camera

Assembly notes: [crowbox_vision_box.txt](crowbox_vision_box.txt). The ESP32-P4 board, camera and microSD are listed under Phase 6 below. Switched from a Raspberry Pi Zero 2 WH + Camera Module 3 + Coral on 2026-10-17, because the Zero 2 W was unobtainable.

### Vision box — SENRISE IP65 ABS box, clear lid, 115×90×55mm
- [x] Ordered
- [x] Received

### ~~Right-angle USB-C to right-angle micro-USB OTG cable~~
Not needed: the Coral is dropped with the switch to the ESP32-P4.

### Nylon standoff + screw + nut assortment, M2 / M3 / M4, male-female (Raogoodcx, 360 pcs)
Camera: 4× M2 standoffs + M2 screws (length = the lens's height above the board + ~1.5mm, so measure the camera first). P4-NANO: check its hole size. The kit has no M2.5; if the holes are M2.5, M2 screws fit through them.
- [x] Ordered
- [x] Received

### 5-minute epoxy (NOT superglue — its fumes fog the clear lid)
Hardware store.
- [ ] Ordered
- [ ] Received

### ~~3M VHB double-sided tape (holds the Coral)~~
Not needed for the vision box any more (it only held the Coral). VHB for the pit lining and signal pod is under "Not ordered yet".

### Black adhesive foam strip (ring around the lens, blocks reflections)
Hardware store (black foam weatherstrip).
- [ ] Ordered
- [ ] Received

### ~~4-conductor cable, 22 AWG (5V / GND / TX / RX to the main box)~~
Replaced by the Cat6 link (Excel Wiring tab, W08), which also carries the PIR and HX711 lines. Cat6 is in the wiring order.

### ~~M4 bolts ×4 + nylon-insert lock nuts + washers (box ears to reward chamber floor)~~
Replaced: the model hangs the vision box from the two reward crossbars with M5 screws into T-nuts (Excel Fasteners tab), covered by the frame and fastener orders.

---

## Phase 5 — Connectivity

### Blues Notecard Cellular — MBNAN (LTE Cat-1 bis, North America), $49
Includes 10 years of service and 500MB of data. The LTE-M (narrowband) version originally planned is now quote-only with a 250-unit minimum. Connects to the main ESP32 (I2C), not the vision board.
- [x] Ordered
- [ ] Received

### Notecarrier A v2.3 ($25)
68×75mm, onboard cellular and GPS antennas, LiPo/solar connectors. Power its V+ pin from the 5V rail (2.5–5.5V; plan for brief ~2A bursts).
- [x] Ordered
- [ ] Received

### Blues flexible LTE/Wi-Fi/GNSS antenna ($3.25)
Backup for the Notecarrier's onboard antenna if signal is weak inside the junction box. Use only the LTE lead.
- [x] Ordered
- [ ] Received

---

## Phase 6 — Computer vision

### Waveshare ESP32-P4-NANO — basic kit (with speaker)
Vision coprocessor. 50×50mm, ESP32-P4 with 32MB PSRAM, standard 15-pin Pi camera connector, microSD slot, USB-C. Replaces the Pi Zero 2 WH, Coral, Camera Module 3, Zero camera cable and OTG cable.
- [x] Ordered
- [ ] Received

### OV5647 5MP camera — 3.6mm adjustable-focus lens, 25×24mm board
Comes with 15-to-15-pin and 15-to-22-pin cables; use the 15-to-15 one for the P4-NANO. Check it has an IR filter (not a "NoIR" / night-vision version).
- [x] Ordered
- [x] Received

### SanDisk High Endurance 32GB microSD (SDSQQNR-032G)
For saving training photos on the P4-NANO's microSD slot.
- [x] Ordered
- [ ] Received

### USB 2.0 keychain microSD card reader (PiShop)
- [x] Ordered
- [ ] Received

### ~~Raspberry Pi Zero 2 WH~~
Replaced by the ESP32-P4-NANO (2026-10-17). The Zero 2 W was unobtainable.

### ~~Pi Camera Module v3~~
Replaced by the OV5647 camera. Camera Module 3 only works with Raspberry Pi boards.

### ~~Pi Zero camera cable, 22-pin to 15-pin~~
Not needed: the P4-NANO has the standard 15-pin connector, so the camera's own cable fits.

### ~~Panel mount kit for Pi Camera Module v3~~
Not needed with the vision box design — the camera mounts on standoffs inside the clear lid and looks through it, so nothing goes through the enclosure wall. See [crowbox_vision_box.txt](crowbox_vision_box.txt).

### ~~USB OTG micro adapter~~
Not needed: the Pi and Coral are both gone.

---

## ~~Phase 7 — Coral~~

Dropped on 2026-10-17. The classifier runs on the ESP32-P4 itself (int8 model through ESP-DL), so no accelerator is needed.

### ~~Google Coral USB Accelerator~~
Not needed.

### ~~P-channel MOSFET (Coral power switching)~~
Not needed for the Coral. Revisit only if the P4-NANO's measured idle draw turns out too high and its power needs switching (see the vision box notes).

---

## Tools and soldering

### Pinecil V2 soldering iron (BB2, Amazon)
- [x] Ordered
- [x] Received

### 65W USB-C PD power supply (Handheld Legend, 20V / 3.25A) — powers the Pinecil
- [x] Ordered
- [ ] Received

### Anker 643 USB-C to USB-C cable, 100W, silicone, 3ft
- [x] Ordered
- [x] Received

### Rosin-core solder, 0.6mm, 100g (MAIYUM)
- [x] Ordered
- [x] Received

### Hakko 633-01 iron holder + tip cleaner
- [x] Ordered
- [x] Received

### Desoldering wick (TOWOT)
- [x] Ordered
- [x] Received

### Flux pen
- [x] Ordered
- [ ] Received

### Digital calipers
- [x] Ordered
- [x] Received

### Metric hex key set
- [x] Ordered
- [x] Received

### AMTOVL tool set
- [x] Ordered
- [ ] Received

### Drill bit set, 19-piece, 1–10mm
- [x] Ordered
- [x] Received

### Step drill bit set (titanium coated) — for gland holes
- [x] Ordered
- [x] Received

### Wire strippers (already owned)
- [x] Received

### Heat gun
Hardware store.
- [ ] Ordered
- [ ] Received

---

## Not ordered yet

**Hardware store**

### Plywood (panels)
- [ ] Ordered
- [ ] Received

### Wood screws
- [ ] Ordered
- [ ] Received

### 3M VHB tape (pit lining, signal pod)
- [ ] Ordered
- [ ] Received

(Also from the hardware store, listed above: M5×75 hex bolts ×4, 5-minute epoxy, black foam weatherstrip, heat gun.)

**Wiring order (later)**

### Cat6 stranded patch cable (vision box link, plugs cut off)
- [ ] Ordered
- [ ] Received

### Field cables (jacketed multi-core runs per the Excel Wiring tab)
- [ ] Ordered
- [ ] Received

### Cable glands — PG7 / PG9 assortment + PG16 with multi-hole inserts
See the Excel's Glands tab for counts and positions. Servo plugs are about 7.6mm wide and need PG9 or larger to pass through.
- [ ] Ordered
- [ ] Received

### Ferrules
- [ ] Ordered
- [ ] Received

### Heat-shrink (plain + adhesive-lined)
- [ ] Ordered
- [ ] Received

### Dupont jumpers / female housings (incl. the ESP32-P4 board's header)
- [ ] Ordered
- [ ] Received

### Hookup wire
- [ ] Ordered
- [ ] Received

### Split loom
- [ ] Ordered
- [ ] Received

### COB LED strip
- [ ] Ordered
- [ ] Received

### Silica gel packs (condensation in the sealed boxes)
- [ ] Ordered
- [ ] Received

### Main power switch, sealed
- [ ] Ordered
- [ ] Received

**Phase 9 — Solar (not needed for bench or first deployment)**

### Solar panel, 20W 12V (~470×345×20mm)
- [ ] Ordered
- [ ] Received

### Small-panel tilt mount kit
- [ ] Ordered
- [ ] Received

### Victron SmartSolar MPPT 75/10 charge controller (LiFePO4 profile, load output)
- [ ] Ordered
- [ ] Received

### 2-core 18 AWG UV-rated outdoor cable (panel to controller)
- [ ] Ordered
- [ ] Received

**Fabrication**

### SendCutSend — reward tray
- [ ] Ordered
- [ ] Received

### SendCutSend — bottom plate (0.187")
- [ ] Ordered
- [ ] Received

### SendCutSend — gate (0.177" polycarbonate)
- [ ] Ordered
- [ ] Received

### ISPACE — 3D prints and laser-cut parts
- [ ] Ordered
- [ ] Received
