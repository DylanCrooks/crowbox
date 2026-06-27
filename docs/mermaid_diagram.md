# CrowBox State Machine

```mermaid
stateDiagram-v2
    [*] --> IDLE

    IDLE --> AWAKE_WATCHING: PIR_TRIPPED\ngate opens if presence_opens_gate

    AWAKE_WATCHING --> DEPOSIT_DETECTED: WEIGHT_DETECTED
    AWAKE_WATCHING --> PIR_CLEARED: no retrigger > debounce window

    DEPOSIT_DETECTED --> VALIDATING: WEIGHT_SETTLED
    VALIDATING --> SIGNAL_VALID: OBJECT_VALID
    VALIDATING --> SIGNAL_INVALID: OBJECT_INVALID

    SIGNAL_INVALID --> TRAPDOOR_SEQ: deposit_pending_clear = true
    SIGNAL_VALID --> DISPENSING: REWARD_GRANTED\n(VOD or JACKPOT bundle)
    SIGNAL_VALID --> NO_REWARD: REWARD_WITHHELD

    DISPENSING --> GATE_OPENING
    GATE_OPENING --> GATE_CLOSING: reward tray baseline\n(crow took food)
    GATE_CLOSING --> TRAPDOOR_SEQ: deposit_pending_clear = true

    NO_REWARD --> TRAPDOOR_SEQ

    TRAPDOOR_SEQ --> TRAPDOOR_UNLOCKING
    TRAPDOOR_UNLOCKING --> TRAPDOOR_OPENING: 150ms delay\n(bolt fully retracted)
    TRAPDOOR_OPENING --> TRAPDOOR_CLOSING: TRAPDOOR_OPEN_COMPLETE
    TRAPDOOR_CLOSING --> TRAPDOOR_LOCKING
    TRAPDOOR_LOCKING --> AWAKE_WATCHING: deposit cleared

    PIR_CLEARED --> PRESET_RESTOCK: visit concluded
    PRESET_RESTOCK --> IDLE: LVT + object restocked\ngate safety closed

    IDLE --> ERROR: sensor fault
    AWAKE_WATCHING --> ERROR: sensor fault
    ERROR --> IDLE: manual reset
```

## Notes

Stage 1 vs Stage 2 differences are handled within states, not as separate flows:
- Stage 1: gate opens on PIR, object dispenser fires at PRESET_RESTOCK
- Stage 2: gate only opens on DISPENSING, no object dispenser

Reward bundle at DISPENSING:
- VOD → HVT (hopper 2)
- JACKPOT → LVT + HVT + EHVT (hoppers 1+2+3)

TRAPDOOR_SEQ fires after any deposit (valid or invalid) once crow has departed.

presence_opens_gate is a config bool — true for stage 1 by default, configurable for stage 2.