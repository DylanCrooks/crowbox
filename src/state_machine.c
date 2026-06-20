// in state_machine.c — NOT in the header
static void handle_pir_tripped(void) {
    if (config.stage == STAGE_2_PRESENCE_TRIGGER) {
        grant_reward(DISPENSER_HIGH_VALUE);
        gate_open();
        gate_close();
        dispense_into_chamber(DISPENSER_SEED);  // pre-stage next reward
    } else {
        // STAGE_3 / STAGE_4: enter watching state, wait for deposit
        transition_to(STATE_DEPOSIT_DETECTED_WATCH);
    }
}
