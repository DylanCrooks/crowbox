#pragma once

// buzzer_init — configure LEDC_TIMER_1 (10-bit, 2000Hz) and buzzer channel on PIN_BUZZER.
void buzzer_init(void);

// buzzer_play_reject — short low-frequency tone: invalid deposit feedback.
void buzzer_play_reject(void);

// buzzer_play_accept — short high-frequency tone: valid deposit feedback.
void buzzer_play_accept(void);

// buzzer_play_jackpot — ascending sweep: jackpot reward feedback.
void buzzer_play_jackpot(void);
