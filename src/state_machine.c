/*


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


static void handle_pir_tripped(void) {
    last_pir_trigger_ms = now_ms();
    if (gate_state == GATE_CLOSED && config.presence_opens_gate) {
        gate_open();
    }
}

static void check_for_departure(void) {  // polled ~1Hz while awake
    if (now_ms() - last_pir_trigger_ms > DEPARTURE_DEBOUNCE_MS) {
        post_event(EVENT_PIR_CLEARED, ...);
    }
}

static void handle_pir_cleared(void) {
    if (weight_read_grams(SENSOR_REWARD_CHAMBER) <= BASELINE_THRESHOLD) {
        dispense(DISPENSER_LVT, DISPENSE_REASON_PRESET);
    }
    if (config.stage == STAGE_1_PRESENCE_TRIGGER) {
        dispense(DISPENSER_OBJECT, DISPENSE_REASON_PRESET);  // no pileup tracking needed, confirmed
    }
    if (gate_state == GATE_OPEN) gate_close();
    transition_to(deposit_pending_clear ? STATE_TRAPDOOR_UNLOCKING : STATE_IDLE);
}

static void handle_object_valid(void) {
    if (!roll_reward_schedule()) { post_event(EVENT_REWARD_WITHHELD, ...); return; }
    bool jackpot = roll_jackpot(get_current_jackpot_chance());
    dispense_reason_t reason = jackpot ? DISPENSE_REASON_JACKPOT : DISPENSE_REASON_VOD;
    reward_bundle_t bundle = get_reward_bundle(reason);
    for (int i = 0; i < bundle.item_count; i++) dispense(bundle.items[i].dispenser_id, reason);
    if (gate_state == GATE_CLOSED) gate_open();
    play_tone(jackpot ? TONE_JACKPOT : TONE_ACCEPT);
    set_light(jackpot ? LIGHT_JACKPOT : LIGHT_ACCEPT);
    deposit_pending_clear = true;
    post_event(EVENT_REWARD_GRANTED, .reason = reason, ...);
}

static void handle_object_invalid(void) {
    play_tone(TONE_REJECT);
    set_light(LIGHT_REJECT);
    deposit_pending_clear = true;  // still needs clearing even if rejected
}

static reward_bundle_t get_reward_bundle(dispense_reason_t reason) {
    switch (reason) {
        case DISPENSE_REASON_PRESET:  return (reward_bundle_t){{{DISPENSER_LVT,1}}, 1};
        case DISPENSE_REASON_VOD:     return (reward_bundle_t){{{DISPENSER_HVT,1}}, 1};
        case DISPENSE_REASON_JACKPOT: return (reward_bundle_t){{{DISPENSER_LVT,1},{DISPENSER_HVT,1},{DISPENSER_EHVT,1}}, 3};
    }
}

*/