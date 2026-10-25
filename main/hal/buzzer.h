#pragma once
// Passive buzzer: haptic-style clicks for touch and the knob, and chimes for the timer.
//
// Every sound is defined in BUZZ_PATTERNS below, so this is the one place to tune them.
// A pattern is a list of notes, each BZ(hz, volume, ms) or BZ_REST(ms), played `repeat` times.
//   hz      pitch. The buzzer is loudest near its resonance, about 2-3 kHz.
//   volume  0-50: PWM duty in percent. 50 is a square wave, as loud as it gets; single digits
//           are a soft tick. Loudness isn't linear: 3 vs 6 is easy to hear, 40 vs 50 isn't.
//   ms      how long the note lasts.
//
// Priorities decide what happens when a sound is asked for while another plays: the new one
// takes over if its priority is the same or higher, and is dropped if it's lower. So a tap
// can't cut off the end-of-session chime, but a new tap replaces the last tap.
//
// buzzer_play() is safe from any task and returns at once; a timer plays the notes.
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUZZ_PRIO_FEEDBACK  0    // touch and knob clicks: tiny, quiet
#define BUZZ_PRIO_PET       1    // the pet's reactions: soft and warm
#define BUZZ_PRIO_NOTICE    2    // something happened: a session started, a drift, an invite
#define BUZZ_PRIO_ALARM     3    // the timer ran out: loud, meant to be heard across the room

#define BZ(hz, vol, ms) { (hz), (vol), (ms) }
#define BZ_REST(ms)     { 0, 0, (ms) }

//  name            priority            repeat  notes
#define BUZZ_PATTERNS(X) \
    /* ---- Feedback: felt more than heard ---- */ \
    /* a tap or knob press on a button or row: one crisp tick */ \
    X(TAP,           BUZZ_PRIO_FEEDBACK, 1, BZ(2700, 30, 9)) \
    /* each detent of the knob: a fainter, higher tick, like a ratchet */ \
    X(DETENT,        BUZZ_PRIO_FEEDBACK, 1, BZ(3800, 15, 4)) \
    /* the - and + buttons (also while held and repeating) */ \
    X(STEP,          BUZZ_PRIO_FEEDBACK, 1, BZ(3200, 25, 6)) \
    /* Back: two ticks falling away */ \
    X(BACK,          BUZZ_PRIO_FEEDBACK, 1, BZ(2600, 30, 8), BZ_REST(28), BZ(1900, 30, 10)) \
    \
    /* ---- The pet ---- */ \
    /* petting: three soft rising coos, a purr more than a click */ \
    X(PET,           BUZZ_PRIO_PET,      1, BZ(1150, 25, 45), BZ_REST(35), BZ(1400, 30, 45), \
                                            BZ_REST(35), BZ(1750, 35, 80)) \
    /* tapping it during focus: one low, quiet "shh" */ \
    X(SHH,           BUZZ_PRIO_PET,      1, BZ(880, 20, 70)) \
    /* stroking it: a low, soft trill, like a purr */ \
    X(PURR,          BUZZ_PRIO_PET,      1, BZ(520, 18, 35), BZ_REST(10), BZ(600, 18, 35), \
                                            BZ_REST(10), BZ(520, 18, 45)) \
    /* the Playground: one click of the wind-up key, a ratchet click-clack */ \
    X(WIND,          BUZZ_PRIO_FEEDBACK, 1, BZ(3400, 25, 6), BZ_REST(14), BZ(2900, 20, 5)) \
    /* fully wound: a little music-box tune while it dances */ \
    X(DANCE,         BUZZ_PRIO_PET,      1, BZ(2093, 30, 110), BZ(2637, 30, 110), BZ(3136, 30, 110), \
                                            BZ(2637, 30, 110), BZ(2349, 30, 110), BZ(2793, 30, 110), \
                                            BZ(3520, 30, 110), BZ(2793, 30, 110), BZ(3136, 30, 160), \
                                            BZ_REST(40), BZ(4186, 30, 220)) \
    /* a gear tossed: a quick rising swish */ \
    X(TOSS,          BUZZ_PRIO_FEEDBACK, 1, BZ(1800, 12, 20), BZ(2300, 12, 20), BZ(2900, 12, 25)) \
    /* ...and caught: a crunchy chomp-chomp */ \
    X(CRUNCH,        BUZZ_PRIO_PET,      1, BZ(420, 40, 25), BZ_REST(20), BZ(360, 40, 25), \
                                            BZ_REST(60), BZ(440, 35, 25), BZ_REST(20), BZ(380, 35, 30)) \
    \
    /* ---- Notices ---- */ \
    /* start-up chirp */ \
    X(BOOT,          BUZZ_PRIO_NOTICE,   1, BZ(1568, 35, 45), BZ(2093, 35, 45), BZ(2637, 35, 70)) \
    /* a study session starts: the clockwork winds up, three quick rising notes */ \
    X(SESSION_START, BUZZ_PRIO_NOTICE,   1, BZ(2093, 50, 55), BZ_REST(20), BZ(2637, 50, 55), \
                                            BZ_REST(20), BZ(3136, 50, 110)) \
    /* the session or set was ended by hand: two soft falling notes */ \
    X(END,           BUZZ_PRIO_NOTICE,   1, BZ(2093, 45, 80), BZ_REST(30), BZ(1568, 45, 150)) \
    /* a drift began: a low double nudge */ \
    X(DRIFT,         BUZZ_PRIO_NOTICE,   1, BZ(1000, 50, 90), BZ_REST(70), BZ(1000, 50, 90)) \
    /* the laptop went silent and the session paused: a slow fall */ \
    X(LINK_LOST,     BUZZ_PRIO_NOTICE,   1, BZ(1568, 50, 140), BZ_REST(40), BZ(1175, 50, 140), \
                                            BZ_REST(40), BZ(880, 50, 220)) \
    /* the laptop is back and the session runs again: a quick rise */ \
    X(LINK_BACK,     BUZZ_PRIO_NOTICE,   1, BZ(1568, 45, 60), BZ(2093, 45, 60), BZ(2637, 45, 90)) \
    /* a friend invited us to a study circle: ding-dong, twice */ \
    X(INVITE,        BUZZ_PRIO_NOTICE,   2, BZ(2637, 50, 150), BZ_REST(40), BZ(2093, 50, 260), \
                                            BZ_REST(250)) \
    \
    /* ---- Alarms ---- */ \
    /* study time is up, the break starts: a bright rising chime, four times */ \
    X(BREAK_START,   BUZZ_PRIO_ALARM,    4, BZ(2093, 50, 160), BZ(2637, 50, 160), BZ(3136, 50, 160), \
                                            BZ(3520, 50, 450), BZ_REST(250)) \
    /* the 4th session is up, the long rest starts: the chime climbs, then a flourish */ \
    X(LONG_REST,     BUZZ_PRIO_ALARM,    3, BZ(2093, 50, 160), BZ(2637, 50, 160), BZ(3136, 50, 160), \
                                            BZ(3520, 50, 300), BZ_REST(60), BZ(3136, 50, 130), \
                                            BZ(3520, 50, 500), BZ_REST(280)) \
    /* the break is over: an alarm-clock "beep-beep", six times */ \
    X(BREAK_OVER,    BUZZ_PRIO_ALARM,    6, BZ(2793, 50, 140), BZ_REST(70), BZ(2793, 50, 140), \
                                            BZ_REST(380)) \
    /* the long rest is over and the set is done: the chime falling back down, twice */ \
    X(SET_DONE,      BUZZ_PRIO_ALARM,    3, BZ(3520, 50, 160), BZ(3136, 50, 160), BZ(2637, 50, 160), \
                                            BZ(2093, 50, 450), BZ_REST(280))

typedef struct {
    uint16_t hz;    // 0 = rest
    uint8_t vol;    // duty in percent, 0-50
    uint16_t ms;
} buzz_note_t;

#define BUZZ_ENUM_ITEM(name, prio, repeat, ...) BUZZ_##name,
typedef enum { BUZZ_PATTERNS(BUZZ_ENUM_ITEM) BUZZ_COUNT } buzz_t;
#undef BUZZ_ENUM_ITEM

// Set up the PWM on board()->buzzer (after board_detect and the display, which owns LEDC
// timer 0 for the backlight). Without a buzzer pin buzzer_play() does nothing.
esp_err_t buzzer_init(void);

// Play a sound, unless one with a higher priority is playing
void buzzer_play(buzz_t sound);

#ifdef __cplusplus
}
#endif
