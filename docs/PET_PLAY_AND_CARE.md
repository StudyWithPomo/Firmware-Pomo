# Pet play and care: spec and log

For the dashboard ([Frontend-Pomo](https://github.com/StudyWithPomo/Frontend-Pomo)) to recreate what the device does with the pet, with every number, string and drawing it uses. The device is the source of truth for the rules in Part 2: the dashboard should show what the device reports rather than run its own copy.

- **Part 1, Milestone D: play.** Petting, the Playground, the dance and feeding. Built and pushed (`8c93a56`).
- **Part 2, Milestone E: gears and hunger.** Built; checked in the PC preview and rules tests, not yet flashed. Studying earns gears; feeding spends them; the pet gets hungry over time.
- **Log.** Dated entries, newest first. Every change to anything in this file gets one.

Coordinates are on the pet's 16×16 grid (the same grid as `pet.js`): `[x, y, w, h]`, top-left origin. Palette slots (`body`, `line`, `cheek`, `shade`, `hi`, `rivet`, `mono`) are `pet.js`'s.

---

## Part 1: Milestone D, play (built)

### Petting (every pet outside Focus)

The pet reacts the same wherever it appears: Home, the Pet hub, Status, Wardrobe, Boot, Invite, Intermission, Ready and the Playground. **Focus is different:** a tap there only hushes the pet (the `shh` face for 2 s).

The touch target is the whole porthole around the pet, not just the sprite.

| Gesture | What happens |
|---|---|
| **Tap** (on press, not release) | The pet is pleased: the `love` face for **2 s** (outside a session). The porthole **hops** up 5 px (110 ms up, 140 ms down). **Three hearts** rise. The `PET` sound. |
| **Stroke** (drag across it) | Every **30 px** of finger travel (sum of \|dx\| + \|dy\|), at most once per **250 ms**: the pet **squishes** sideways to 115% width and back (110 ms out, 170 ms back, ease-out), **one heart** rises from a random spot (±20 px of centre, 0–10 px below the porthole's top), the `love` face for 2 s, and the `PURR` sound. The drag keeps counting even if the finger leaves the pet. |

**Hearts:** a 7×6 pixel heart drawn at 2× in copper `#B5462A`, rising **38 px** over **900 ms** (ease-out), fully opaque for the first half then fading out. Tap hearts start at x offsets **−22, +2, +18** px from the porthole's centre and y offsets **6, 0, 10** px below its top, **120 ms** apart. They float above everything, so they finish even if the screen changes.

```
heart (7x6):    rects
.XX.XX.         [1,0,2,1] [4,0,2,1]
XXXXXXX         [0,1,7,2]
XXXXXXX
.XXXXX.         [1,3,5,1]
..XXX..         [2,4,3,1]
...X...         [3,5,1,1]
```

Petting earns nothing in Milestone D. (Part 2 changes how strongly a hungry pet reacts, never whether it does.)

### The Playground (Pet hub → Playground)

A big porthole (112 px) with the pet at 96 px, a speech bubble, a **"Wound up"** gauge, and two buttons: **Wind** and **Feed a gear**.

**Every body wears a wind-up key here.** The bot always has one; in the Playground the cat and dog get one too, in the same place.

#### Winding

| | |
|---|---|
| One click | Knob: any detent, either direction. Touch: tap **Wind**, or hold it (repeats). |
| Full | **12** clicks |
| Each click | The key turns one frame, the gauge fills by 1/12, the `WIND` sound |
| Words | 1–5 clicks: "Keep winding..." · 6–11: "Almost there!" |
| Left alone | After **3 s** with no play it runs down: one click every **0.6 s**, the key turning back a frame each time. At 0: "Wind my key, or toss me a gear!" |

**Key frames** (the key turning, seen from the front). Frame order while winding: 0, 1, 2, 3, 0, ...

| Frame | `shade` rects | `line` rects |
|---|---|---|
| 0 full bow (as `pet.js` draws the bot's key) | `[5,0,2,2]` `[9,0,2,2]` | `[7,0,2,3]` |
| 1 three-quarter | `[6,0,1,2]` `[9,0,1,2]` | `[7,0,2,3]` |
| 2 edge-on | none | `[7,0,2,3]` |
| 3 three-quarter | same as 1 | `[7,0,2,3]` |

Draw order with a key: base, metal, body layer (cat/dog parts; for the bot the key frame *replaces* its body layer), key frame, mood, accessories, monocle.

#### The dance (12 clicks)

**3 s** long. "Wheee!", three hearts at the start, the `DANCE` tune, the `love` face throughout.

- **Spin:** 3 full turns. At time `t` ms, `angle = (t × 360 × 3 / 3000) mod 360` degrees. The pet is drawn at `|cos(angle)|` of its width about its centre, **mirrored** while `cos(angle) < 0`. (No rotation: the pet's rectangles are just drawn narrower, so it looks like it turns.)
- **Hop:** the porthole rises `|sin(2 × angle)| × 6` px: a hop every half turn.
- **Key:** spins fast, frame `floor(t / 60)`.
- **Gauge:** drains from 12 to 0 over the 3 s.
- **After:** back to normal width, three more hearts, "Again! Again!", the `love` face for **1.5 s**.

Winding and feeding are ignored while it dances, and so is petting.

#### Feeding (Milestone D: free and unlimited; Part 2 makes it cost a gear)

| | |
|---|---|
| Toss | Knob: a press. Touch: **Feed a gear**. One gear in the air at a time. |
| Flight | **650 ms** from the top-centre of the Feed button to 5/8 of the way down the pet (its mouth), in a straight line plus an arc `45 × sin(180° × t)` px upward. The gear tumbles: it swaps between its two frames every 90/1000 of the flight. |
| Words | "Ooh, a gear!" when tossed |
| Caught | The squish (as a stroke), "Crunchy! Thank you.", the `love` face for 1.5 s, the `CRUNCH` sound |

The gear is 7×7 at 3×, in brass highlight `#F4DDA0`:

```
frame A (square)   rects                              frame B (diagonal)   rects
...X...            [3,0,1,1]                          .X...X.              [1,0,1,1] [5,0,1,1]
.XXXXX.            [1,1,5,1]                          ..XXX..              [2,1,3,1]
.XX.XX.            [1,2,2,1] [4,2,2,1]                .XX.XX.              [1,2,2,1] [4,2,2,1]
XX...XX            [0,3,2,1] [5,3,2,1]                .X...X.              [1,3,1,1] [5,3,1,1]
.XX.XX.            [1,4,2,1] [4,4,2,1]                .XX.XX.              [1,4,2,1] [4,4,2,1]
.XXXXX.            [1,5,5,1]                          ..XXX..              [2,5,3,1]
...X...            [3,6,1,1]                          .X...X.              [1,6,1,1] [5,6,1,1]
```

#### Sleep

**20 s** with nobody playing (and nothing wound, no gear in the air): the `sleep` face and "Zzz...". Any play (a click, a gear, a tap or stroke) wakes it: "Oh! Hello again."

#### Moods in the Playground (highest wins)

1. Dancing, or within 1.5 s of a dance or a catch, or petted in the last 2 s: `love`
2. Asleep: `sleep`
3. Otherwise: `happy`

### Sounds (passive buzzer)

`hz` is the pitch, `vol` the PWM duty 0–50 (50 is loudest), `ms` the length; `rest` is silence. For WebAudio, a square wave at the same pitch and length is close; volume 50 ≈ full gain, and loudness isn't linear (25 is still clearly audible).

| Sound | When | Notes |
|---|---|---|
| `PET` | tap | 1150 Hz 25 45 ms · rest 35 · 1400 Hz 30 45 ms · rest 35 · 1750 Hz 35 80 ms |
| `PURR` | stroke | 520 Hz 18 35 ms · rest 10 · 600 Hz 18 35 ms · rest 10 · 520 Hz 18 45 ms |
| `WIND` | each wind click | 3400 Hz 25 6 ms · rest 14 · 2900 Hz 20 5 ms |
| `DANCE` | the dance | 2093, 2637, 3136, 2637, 2349, 2793, 3520, 2793 Hz at 30 for 110 ms each · 3136 Hz 30 160 ms · rest 40 · 4186 Hz 30 220 ms |
| `TOSS` | a gear tossed | 1800, 2300 Hz at 12 for 20 ms · 2900 Hz 12 25 ms |
| `CRUNCH` | a gear caught | 420 Hz 40 25 ms · rest 20 · 360 Hz 40 25 ms · rest 60 · 440 Hz 35 25 ms · rest 20 · 380 Hz 35 30 ms |
| `SHH` | tap during Focus | 880 Hz 20 70 ms |

---

## Part 2: Milestone E, gears and hunger (built, not yet flashed)

### The idea

Play had no purpose beyond fun. Rather than paying for play with XP (which would make playing the most efficient way to "study"), **studying earns the treats you spoil the pet with**:

- Finishing study sessions earns **gears**.
- **Feeding** the pet costs a gear. Petting, winding and the dance stay free.
- A **recently fed** pet is happy and says so. One **left unfed** for a long time gets **peckish**, then **hungry**: quieter, a hungry face, asking for a gear.

Guardrails (decided with the user, 2026-10-25):
- **Hunger is a mood, not a health bar.** It stops at "hungry": the pet is never sick, sad, punished or lost, and hunger never touches XP. (The brief cut health and pet death.)
- **Hungry is quieter, never broken.** Every tap and stroke still gets a reaction, just a smaller one.
- **No hunger during a session.** The session's own faces (focus, drift, laptop lost) always win while studying.
- **Hunger only moves when the clock is known**, so a power cycle can't starve the pet.

### Rules (device: pure C++ in `main/app/pet_care.*`, with tests)

#### Constants

| Name | Value |
|---|---|
| `MAX_GEARS` | **12** (the bar holds 12) |
| `GEARS_PER_SESSION` | **1** for every study session whose timer runs out |
| `GEARS_PER_SET` | **+2** more when that session is the 4th (finishes the set): 3 in all |
| `FULL_FOR` | **30 min** after feeding |
| `PECKISH_AFTER` | **12 h** after feeding |
| `HUNGRY_AFTER` | **24 h** after feeding |
| `STUFFED_AFTER` | **3** feeds within 30 min; the 4th is refused |

Hunger is re-checked every second, so a level change shows (and is reported) within a second.

#### Earning

- A session **completed** (its study timer ran to zero) earns 1 gear, whatever the focus score. A completed 4th session earns 3.
- A session **ended early** earns nothing.
- **Test timing** sessions earn gears like any other (so the flow can be tested in minutes).
- Gears **cap at 12**; extra gears are lost. The device reports how many were actually added.
- The study circle adds nothing extra (for now).

#### Feeding

| Situation | Result | Gear spent? |
|---|---|---|
| 0 gears | **empty**: no toss, the low `SHH` hum. "Out of gears! Finish a session to earn some." | no |
| 3 feeds already in the last 30 min | **stuffed**: no toss, the `SHH` hum. "I'm stuffed! Save it for later." | no |
| otherwise | **fed**: the gear flies, "last fed" becomes now | yes |

The 30-minute "stuffed" window starts at the first of those feeds and resets when it ends. It runs on the time since power-on and isn't saved, so a restart also resets it.

#### Hunger

From the time since the pet was last fed (`now − fed_at`):

| Level | Since fed | Face when idle | Taps give | Strokes give | Dance |
|---|---|---|---|---|---|
| `full` | under 30 min | `love` | 3 hearts | a heart | 3 turns, hearts |
| `content` | 30 min to 12 h | `happy` | 3 hearts | a heart | 3 turns, hearts |
| `peckish` | 12 h to 24 h | `happy` | 2 hearts | a heart | 3 turns, hearts |
| `hungry` | 24 h or more | **`hungry`** (new) | 1 heart | a purr, no heart | **2 turns**, no hearts: "Wheee... huff...", then "Phew... a gear would help." |

Which hearts show: with 3, all three start positions; with 2, the left and right ones; with 1, the middle one.

Every gesture always gets the hop or squish and its sound.

**Clock rules** (the device only knows the time once the laptop sends it in `config`):
- **Clock unknown:** hunger reads `content`, and doesn't change. Feeding still spends a gear and makes the pet `full` for 30 min of uptime, but `fed_at` isn't saved.
- **A brand-new pet** (never fed): the first time the clock is known, `fed_at` becomes now. A new pet starts `content`, not hungry.
- **The clock went backwards** (now earlier than `fed_at`): `fed_at` becomes now.

#### Mood order (highest wins)

1. **In a study session:** laptop lost `sad` > drifting `annoyed` > hushed `shh` > `focus`. (Unchanged.)
2. **Break or long rest:** `love` ("wound up"). (Unchanged.)
3. **Idle or Ready:**
   1. petted in the last 2 s: `love`
   2. `full`: `love`
   3. `hungry`: `hungry`
   4. otherwise: `happy`

   The Playground puts two of its own on top: dancing or just caught a gear (1.5 s) is `love`; asleep is `sleep`. Petting wakes it, so a sleeping pet that's petted shows `love`.

### The new face: `hungry`

Worried brows, tired eyes, a small open mouth. All `line`:

```
rects: [5,6,1,1] [10,6,1,1]            brows
       [5,7,2,1] [9,7,2,1]             tired eyes
       [7,10,2,1] [6,11,1,1] [9,11,1,1] [7,12,2,1]   open mouth
```

Its word on Status: **"Hungry"**. (The others: Happy, Delighted, Focused, Annoyed, Hushed, Sleepy, Lonely.) `pet.js` should get the same face under the key `hungry`.

### What the pet says

| Where | full | content | peckish | hungry |
|---|---|---|---|---|
| Boot greeting, after "Good morning, Sidak." | "Mochi is full and happy." | "Mochi missed you." | "Mochi could go for a gear." | "Mochi's tummy is ticking. Got a gear?" |
| Playground, idle | "I'm stuffed! Wind me up instead?" | "Wind my key, or toss me a gear!" | "A gear would be lovely..." | "My tummy's ticking... got a gear?" |
| Playground, caught | "Crunchy! Thank you." | "Crunchy! Thank you." | "Crunchy! Thank you." | "Oh, that's better!" |

("Mochi" is the pet's name.)

Intermission's score line gains the gears: "88% focused, +66 XP, **+1 gear**.\nMochi is most proud." After the 4th session: "**+3 gears**". If the bar was already full: "**gears full**". With the penalty: "40% focused, −10 XP, +1 gear.\nMochi is disappointed, but you keep your level."

### On screen

- **Gears bar** at the top right of **Home** (replacing the XP number, which stays on Status and Peek) and of the **Playground**: a small gear icon, then **12 pips**, filled for each gear held. Pips are 5×8 px with 1 px gaps; filled in brass `#C9A15A`, empty as a dark outline `#6B5233`.
- **Feed a gear** when the bar is empty: the button looks dimmed (outline `#6B5233`, text `#B8A27C`), and a press says "Out of gears! ..." (no toss).
- **Status** gets a **"Last fed"** row: "just now" (under 2 min), "N min ago", "N h ago", "yesterday" (24–48 h), "N days ago", "never", or "–" while the clock is unknown.

### Saved on the device (NVS)

| Key | Meaning |
|---|---|
| `gears` | gears held, 0–12 |
| `fed_at` | when last fed (Unix seconds), 0 = never |

(The stuffed window and "fed this boot" aren't saved: see Feeding.)

### Protocol additions (device → laptop)

Additions only: nothing existing changes.

- **`state`** (sent every second during a session or break): adds `"gears": 4` and `"hunger": "content"`.
- **`end`**: unchanged. What a session earned comes in the `pet` message right after it (`"event": "earned"`).
- **New `pet` message**, sent right after `hello`, whenever the hunger level changes, after every scored session that ran to the end, and on every feed attempt:

```json
{"t": "pet", "gears": 4, "max_gears": 12, "hunger": "content", "fed_at": 1792790040, "event": "fed"}
{"t": "pet", "gears": 5, "max_gears": 12, "hunger": "content", "fed_at": 1792790040, "event": "earned", "earned": 1}
```

| Field | |
|---|---|
| `gears`, `max_gears` | held, and the cap (12) |
| `hunger` | `full`, `content`, `peckish` or `hungry` |
| `fed_at` | Unix seconds, or 0 if never or unknown |
| `event` | why it was sent: `hello`, `earned`, `fed`, `empty`, `stuffed`, `hunger` (the level changed) |
| `earned` | only with `earned`: gears this session added after the cap (0 when the bar was full) |

The laptop doesn't send anything new. To feed from the dashboard later we'd add a `feed` command; not planned yet.

### Test cases (the device's rules tests; the dashboard can use the same)

1. A completed session: +1 gear. A completed 4th session: +3. An ended-early session: +0.
2. 11 gears plus a completed 4th session: 12 gears, reports +1.
3. Feed with 0 gears: `empty`, gears stay 0, `fed_at` unchanged.
4. Three feeds in 10 minutes succeed; the 4th is `stuffed` and keeps its gear. 31 minutes after the first, feeding works again.
5. Hunger: fed at T. At T+29 min `full`; T+31 min `content`; T+12 h `peckish`; T+24 h `hungry`; T+10 days still `hungry`.
6. Clock unknown: always `content`. A brand-new pet when the clock becomes known: `fed_at` = now, `content`.
7. Clock moves before `fed_at`: `fed_at` resets to now.
8. A study session while `hungry`: the session faces win; the idle face goes back to `hungry` when it ends.

### Where it lives (device)

| | |
|---|---|
| Rules | `main/app/pet_care.*`, tests in `test/pet_care_test.cpp` (all the cases above) |
| Saved | `main/app/storage.*`: `gears`, `fed_at` |
| State | `AppState`: `gears` and `hunger` subjects, `feed()`, gears earned in `recordSession()`, hunger in `update()` every second, the mood order in `refreshMood()` |
| Art | `main/ui/pet/pet_data.h`: `pet_hungry_line_r`, and the gear sprite `pet_gear_a_r` / `pet_gear_b_r` |
| UI | `theme::gears_bar()` (Home, Playground), hunger-aware `pet_hearts()` / `pet_stroke()`, the Playground, Boot, Intermission, Status |
| Protocol | `main/comms/link.*`: `link_send_pet()`, the `pet` message, `gears` and `hunger` in `state` |

---

## Log

### 2026-10-25: Milestone E built (not yet flashed)
- Everything in Part 2, as specified, with these details settled while building (Part 2 now says them):
  - The stuffed window and "fed this boot" run on the time since power-on and aren't saved, so only `gears` and `fed_at` are saved.
  - `end` didn't change: a `pet` message with `"event": "earned"` and `"earned": N` follows each completed session instead (the device decides the gears after it sends `end`).
  - The `pet` message also goes out right after `hello`.
  - Empty and stuffed feeds play the low `SHH` hum.
  - A hungry dance says "Wheee... huff..." then "Phew... a gear would help."
  - Status calls the row "Last fed".
  - Hunger is re-checked every second.
- Checked: the rules tests (every case above), the PC preview of each hunger level, a 100-round soak test (flat memory), and an ESP32 compile check.

### 2026-10-25: fix, the dance tune was cut off
- The dance started the tune and then the hearts, whose coo (same priority) replaced it after one note. Now the hearts go first and the tune replaces their coo. Committed with Milestone E.

### 2026-10-25: Milestone E planned
- Gears and hunger designed with the user (Part 2). Not built yet.
- Decided: rewards for playing would make play the best way to "earn", so studying earns the treats instead. Bar of 12. Hunger is gentle and stops at "hungry"; it never touches XP and never runs during a session.

### 2026-10-25: Milestone D pushed (`8c93a56`)
- Petting on every pet outside Focus (tap and stroke), the Playground (winding, the dance, feeding gears, sleep), the squash-and-mirror spin, key frames on every body, six new sounds.
- Checked in the PC preview, a 100-round soak test and an ESP32 compile check. Not yet confirmed on hardware.
