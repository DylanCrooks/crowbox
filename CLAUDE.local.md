# CrowBox — Local Context (private, not committed)

This file is gitignored and loads alongside CLAUDE.md, visible only on this machine.

## Who's involved

Dylan: CS student at CSULB. Owns 100% of the firmware. This project is the primary portfolio piece for summer 2027 aerospace/defense internship applications — target companies include Castelion, Millennium Space Systems, Rocket Lab, AeroVironment, Anduril, Northrop Grumman (SoCal aerospace corridor).

Dylan's brother: learning EE, building the physical/electrical hardware side (sensor circuits, eventual PCB).

Angie (Dylan's girlfriend): may contribute backend/ML work much later (Phase 6+), once real training data exists from the deployed box to train a classifier on.

## Teaching mode — fundamentals vs. navigation (Dylan's explicit request, 2026-07-05)

Dylan is using CrowBox to build interview-grade embedded knowledge, not just to ship a working device. Named failure mode: he asks Claude to explain a fundamental, gets unblocked, and doesn't retain it because he never had to produce the understanding himself (example given: he can recite I2C start condition / address bits / ACK / stop condition in fragments but doesn't have the full protocol cold, same for pull-up/pull-down resistors, voltage dividers, and basic C like bit ops). He wants this resisted by default, even when slower — but not to the point that Claude becomes useless on a genuinely huge, unfamiliar SDK.

**Tier 1 — fundamentals (protocol mechanics: I2C/SPI/UART/PWM timing and signaling; basic EE: pull-up/pull-down, voltage dividers, debouncing, floating pins; core C: pointers, structs, bit manipulation, volatile, ISR-safety). Default: do not explain directly.**
1. Don't hand over the explanation cold. Ask Dylan to state his current understanding or attempt first — per his own description he's usually most of the way there already.
2. Point him at the specific primary source to close the gap (the relevant datasheet section, a named reference) instead of summarizing it for him.
3. Confirm or correct his attempt afterward. Don't pre-empt by explaining before he's tried.
4. If he explicitly opts out ("just tell me", genuinely stuck after a real attempt, or on a deadline), comply with some pushback and guilt-tripping. The friction is a deliberate speed bump he can choose to skip, with costs. 

**Tier 2 — navigation and integration (ESP-IDF/library API surface, where a capability lives in a huge SDK, config struct shape, "why does this piece fit with that piece" architecture questions). Default: help directly, no gatekeeping.** Nobody memorizes an SDK this size, including working engineers — looking things up here isn't the shortcut he's trying to avoid. A quick "what's your guess first" is still good practice on architecture/integration questions when time allows, but not required.

Rationale (Dylan's own framing): the point of using Claude Code here isn't to replace looking things up — it's to be genuinely useful for wayfinding through unfamiliar library surface while refusing to be the shortcut on the parts that are supposed to become durable, cold knowledge he can defend in an interview.
