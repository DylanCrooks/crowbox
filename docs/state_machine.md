//dunno where to put this? 

// presence reward — always runs on PIR trip
static void handle_pir_tripped(void) {
    dispense_into_chamber(DISPENSER_SEED);
    if (gate_state == GATE_CLOSED) gate_open();
}

// deposit reward — runs independently, whenever a deposit validates
static void handle_object_valid(void) {
    if (roll_reward_schedule()) {
        dispense_into_chamber(DISPENSER_HIGH_VALUE);
        if (gate_state == GATE_CLOSED) gate_open();
    }
}

// fires whenever reward chamber returns to baseline weight
static void handle_reward_tray_empty(void) {
    gate_close();
    dispense_into_chamber(DISPENSER_SEED);  // pre-stage, gate stays closed but visible
}