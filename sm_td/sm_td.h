/* Copyright 2025 Stanislav Markin (https://github.com/stasmarkin)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Version: 0.6.5-SNAPSHOT
 * Date: 2026-06-18
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef SMTD_UNIT_TEST
#include QMK_KEYBOARD_H
#endif

#if defined(SMTD_UNIT_TEST) && defined(SMTD_TEST_DEBUG) && !defined(SMTD_DEBUG_ENABLED)
#define SMTD_DEBUG_ENABLED
#endif

/* ************************************* *
 *         GLOBAL CONFIGURATION          *
 * ************************************* */

#ifndef SMTD_GLOBAL_SIMULTANEOUS_PRESSES_DELAY_MS
#define SMTD_GLOBAL_SIMULTANEOUS_PRESSES_DELAY_MS 0
#endif

#ifndef SMTD_GLOBAL_TAP_TERM
#define SMTD_GLOBAL_TAP_TERM TAPPING_TERM
#endif

#ifndef SMTD_GLOBAL_SEQUENCE_TERM
#define SMTD_GLOBAL_SEQUENCE_TERM TAPPING_TERM / 2
#endif

#ifndef SMTD_GLOBAL_RELEASE_TERM
#define SMTD_GLOBAL_RELEASE_TERM TAPPING_TERM / 4
#endif

// Dynamic release term (issue #45). For the ambiguous sequence
// `↓mod ↓key ↑mod ↑key` the decision window for ↑key is derived from the
// typing rhythm instead of the fixed SMTD_TIMEOUT_RELEASE: hold is chosen
// only when both releases come much faster than the presses did, i.e.
//   release_term = min(p1, p2) * SMTD_GLOBAL_RELEASE_PERCENT / 100
// where p1 is the pause between the presses and p2 is the overlap between
// ↓key and ↑mod. The result is clamped to [1ms .. SMTD_TIMEOUT_RELEASE],
// so the (possibly per-key) fixed timeout remains the upper bound.
//
// SMTD_GLOBAL_RELEASE_PERCENT is the configuration knob: it defaults to 30 and
// can be set anywhere in [0 .. 100+] for single-percent granularity. Set it to 0
// to disable the dynamic window and fall back to the fixed SMTD_TIMEOUT_RELEASE.
//
// SMTD_GLOBAL_RELEASE_RATIO is a deprecated, undocumented back-compat alias for
// configs that predate PERCENT: when it (and only it) is set, PERCENT is derived
// as 100 / ratio (ratio 0 -> PERCENT 0 -> disabled). New configs should use PERCENT.
#if defined(SMTD_GLOBAL_RELEASE_RATIO) && !defined(SMTD_GLOBAL_RELEASE_PERCENT)
#if SMTD_GLOBAL_RELEASE_RATIO > 0
#define SMTD_GLOBAL_RELEASE_PERCENT (100 / SMTD_GLOBAL_RELEASE_RATIO)
#else
#define SMTD_GLOBAL_RELEASE_PERCENT 0
#endif
#endif

#ifndef SMTD_GLOBAL_RELEASE_PERCENT
#define SMTD_GLOBAL_RELEASE_PERCENT 30
#endif

#ifndef SMTD_GLOBAL_AGGREGATE_TAPS
#define SMTD_GLOBAL_AGGREGATE_TAPS false
#endif

// When enabled, sm_td sends resolved tap keys through the full QMK pipeline
// (process_record) instead of raw tap_code16/register_code16 calls, so QMK
// features like Caps Word, Auto Shift or Key Overrides can see them.
// This only works when the key sent is the same as the keycode in the keymap
// at the pressed position; derived keycodes (e.g. alternate multi-tap keys)
// are still sent directly with a manual Caps Word pass.
// Can be overridden per key via SMTD_FEATURE_PIPELINE_TAPS in smtd_feature_enabled.
#ifndef SMTD_GLOBAL_PIPELINE_TAPS
#define SMTD_GLOBAL_PIPELINE_TAPS true
#endif

// Enable automatic handling for standard QMK MT() / LT() keycodes.
// Set to 1 in your config to use MT()/LT() in keymaps without SMTD_MT/SMTD_LT.
#ifndef SMTD_ENABLE_QMK_TAPHOLD
#define SMTD_ENABLE_QMK_TAPHOLD 0
#endif

// Apply Caps Word shift to MT()/LT() taps when CAPS_WORD_ENABLE is on.
#ifndef SMTD_QMK_TAPHOLD_USE_CAPS_WORD
#define SMTD_QMK_TAPHOLD_USE_CAPS_WORD true
#endif

// QMK-style "chordal hold" (opposite-hands rule). When 1, a tap-hold settles as
// HOLD only if a key on the opposite hand is involved; same-hand rolls stay taps.
// Disabled by default so it compiles out entirely (zero code/RAM when off).
#ifndef SMTD_CHORDAL_HOLD
#define SMTD_CHORDAL_HOLD 0
#endif

/* ************************************* *
 *         BASE DEFINITIONS              *
 * ************************************* */

typedef enum {
    SMTD_ACTION_TOUCH,
    SMTD_ACTION_TAP,
    SMTD_ACTION_HOLD,
    SMTD_ACTION_RELEASE,
} smtd_action;

typedef enum {
    SMTD_RESOLUTION_UNCERTAIN,
    SMTD_RESOLUTION_UNHANDLED,
    SMTD_RESOLUTION_DETERMINED,
} smtd_resolution;

typedef enum {
    SMTD_TIMEOUT_TAP,
    SMTD_TIMEOUT_SEQUENCE,
    SMTD_TIMEOUT_RELEASE,
} smtd_timeout;

typedef enum {
    SMTD_FEATURE_AGGREGATE_TAPS,
    SMTD_FEATURE_PIPELINE_TAPS,
} smtd_feature;


#ifndef SMTD_POOL_SIZE
#define SMTD_POOL_SIZE 10
#endif

/* ************************************* *
 *           PUBLIC FUNCTIONS            *
 * ************************************* */

bool process_smtd(uint16_t keycode, keyrecord_t *record);

/* Clears all sm_td runtime state: the state pool, the active-state list, any
 * pending timeout deferred-execs, and the executing/bypass flags. Intended for
 * test harnesses that reuse one process across scenarios (the QMK test fixture
 * resets QMK state but not sm_td's). Harmless but normally unused in firmware. */
void smtd_reset(void);

smtd_resolution on_smtd_action(uint16_t keycode, smtd_action action, uint8_t tap_count);

__attribute__((weak)) uint32_t get_smtd_timeout(uint16_t keycode, smtd_timeout timeout);

__attribute__((weak)) bool smtd_feature_enabled(uint16_t keycode, smtd_feature feature);

#if SMTD_CHORDAL_HOLD
// Per-key handedness used by the chordal-hold rule. Returns 'L' (left), 'R'
// (right) or '*' (neutral, e.g. thumbs). The default reads the user-supplied
// chordal_hold_layout from PROGMEM (same 'L'/'R'/'*' convention as QMK); it is
// weak so a keymap can override it to compute handedness without the array.
__attribute__((weak)) char smtd_chordal_handedness(keypos_t key);

// Layout marking each matrix position's hand. Required when the default
// smtd_chordal_handedness() is used (i.e. not overridden by the keymap).
extern const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS];
#endif

/* ************************************* *
 *         CUSTOMIZATION HELPERS         *
 * ************************************* */

/* Default implementations for custom timeout and feature hooks. */
uint32_t get_smtd_timeout_default(smtd_timeout timeout);
bool smtd_feature_enabled_default(uint16_t keycode, smtd_feature feature);

/* Output helpers used by the customization macros below. */
void smtd_tap_code16(bool use_cl, uint16_t key);
void smtd_register_code16(bool use_cl, uint16_t key);
void smtd_unregister_code16(bool use_cl, uint16_t key);

#ifdef SMTD_DEBUG_ENABLED
/* Optional readable keycode names for debug output. */
__attribute__((weak)) char *smtd_keycode_to_str_user(uint16_t keycode);
#endif

/* ************************************* *
 *         CUSTOMIZATION MACROS          *
 * ************************************* */

#define SMTD_TAP_16(use_cl, key) smtd_tap_code16(use_cl, key)
#define SMTD_REGISTER_16(use_cl, key) smtd_register_code16(use_cl, key)
#define SMTD_UNREGISTER_16(use_cl, key) smtd_unregister_code16(use_cl, key)

#ifndef NOTHING
#define NOTHING
#endif
#ifndef OVERLOAD5
#define OVERLOAD5(_1, _2, _3, _4, _5, NAME, ...) NAME
#endif
#ifndef OVERLOAD4
#define OVERLOAD4(_1, _2, _3, _4, NAME, ...) NAME
#endif
#ifndef EXEC
#define EXEC(code)                                                             \
  do {                                                                         \
    code                                                                       \
  } while (0)
#endif

#define SMTD_LIMIT(limit, if_under_limit, otherwise) \
    if (tap_count < limit) { if_under_limit; } else { otherwise; }

#define SMTD_DANCE(macro_key, touch_action, tap_action, hold_action, release_action)    \
    case macro_key: {                                                                   \
        switch (action) {                                                               \
            case SMTD_ACTION_TOUCH: touch_action; return SMTD_RESOLUTION_UNCERTAIN;     \
            case SMTD_ACTION_TAP: tap_action; return SMTD_RESOLUTION_DETERMINED;        \
            case SMTD_ACTION_HOLD: hold_action; return SMTD_RESOLUTION_DETERMINED;      \
            case SMTD_ACTION_RELEASE: release_action; return SMTD_RESOLUTION_DETERMINED;\
        }                                                                               \
        break;                                                                          \
    }

#define SMTD_MT(...) OVERLOAD4(__VA_ARGS__, SMTD_MT4, SMTD_MT3, SMTD_MT2)(__VA_ARGS__)
#define SMTD_MT2(key, mod) SMTD_MT3_ON_MKEY(key, key, mod)
#define SMTD_MT3(key, mod, threshold) SMTD_MT4_ON_MKEY(key, key, mod, threshold)
#define SMTD_MT4(key, mod, threshold, use_cl) SMTD_MT5_ON_MKEY(key, key, mod, threshold, use_cl)
#define SMTD_MT_ON_MKEY(...) OVERLOAD5(__VA_ARGS__, SMTD_MT5_ON_MKEY, SMTD_MT4_ON_MKEY, SMTD_MT3_ON_MKEY)(__VA_ARGS__)
#define SMTD_MT3_ON_MKEY(...) SMTD_MT4_ON_MKEY(__VA_ARGS__, 1)
#define SMTD_MT4_ON_MKEY(...) SMTD_MT5_ON_MKEY(__VA_ARGS__, true)
#define SMTD_MT5_ON_MKEY(macro_key, tap_key, mod, threshold, use_cl) \
    SMTD_DANCE(macro_key,                                    \
        NOTHING,                                             \
        SMTD_TAP_16(use_cl, tap_key),                        \
        SMTD_LIMIT(threshold,                                \
            register_mods(MOD_BIT(mod));                     \
            send_keyboard_report(),                          \
            SMTD_REGISTER_16(use_cl, tap_key)),              \
        SMTD_LIMIT(threshold,                                \
            unregister_mods(MOD_BIT(mod));                   \
            send_keyboard_report(),                          \
            SMTD_UNREGISTER_16(use_cl, tap_key));            \
            send_keyboard_report()                           \
    )

#define SMTD_MTE(...) OVERLOAD4(__VA_ARGS__, SMTD_MTE4, SMTD_MTE3, SMTD_MTE2)(__VA_ARGS__)
#define SMTD_MTE2(key, mod_key) SMTD_MTE3_ON_MKEY(key, key, mod_key)
#define SMTD_MTE3(key, mod_key, threshold) SMTD_MTE4_ON_MKEY(key, key, mod_key, threshold)
#define SMTD_MTE4(key, mod_key, threshold, use_cl) SMTD_MTE5_ON_MKEY(key, key, mod_key, threshold, use_cl)
#define SMTD_MTE_ON_MKEY(...) OVERLOAD5(__VA_ARGS__, SMTD_MTE5_ON_MKEY, SMTD_MTE4_ON_MKEY, SMTD_MTE3_ON_MKEY)(__VA_ARGS__)
#define SMTD_MTE3_ON_MKEY(...) SMTD_MTE4_ON_MKEY(__VA_ARGS__, 1)
#define SMTD_MTE4_ON_MKEY(...) SMTD_MTE5_ON_MKEY(__VA_ARGS__, true)
#define SMTD_MTE5_ON_MKEY(macro_key, tap_key, mod_key, threshold, use_cl) \
    SMTD_MBTE5_ON_MKEY(macro_key, tap_key, MOD_BIT(mod_key), threshold, use_cl)
#define SMTD_MBTE5_ON_MKEY(macro_key, tap_key, mods, threshold, use_cl) \
    SMTD_DANCE(macro_key,                                    \
        EXEC(                                                \
            register_mods(mods);                             \
            send_keyboard_report();                          \
        ),                                                   \
        EXEC(                                                \
            unregister_mods(mods);                           \
            SMTD_TAP_16(use_cl, tap_key);                    \
        ),                                                   \
        SMTD_LIMIT(threshold,                                \
            NOTHING,                                         \
            EXEC(                                            \
                unregister_mods(mods);                       \
                send_keyboard_report();                      \
                SMTD_REGISTER_16(use_cl, tap_key);           \
            )                                                \
        ),                                                   \
        SMTD_LIMIT(threshold,                                \
            EXEC(                                            \
                unregister_mods(mods);                       \
                send_keyboard_report();                      \
            ),                                               \
            SMTD_UNREGISTER_16(use_cl, tap_key)              \
        )                                                    \
    )

#define SMTD_LT(...) OVERLOAD4(__VA_ARGS__, SMTD_LT4, SMTD_LT3, SMTD_LT2)(__VA_ARGS__)
#define SMTD_LT2(key, layer) SMTD_LT3_ON_MKEY(key, key, layer)
#define SMTD_LT3(key, layer, threshold) SMTD_LT4_ON_MKEY(key, key, layer, threshold)
#define SMTD_LT4(key, layer, threshold, use_cl) SMTD_LT5_ON_MKEY(key, key, layer, threshold, use_cl)
#define SMTD_LT_ON_MKEY(...) OVERLOAD5(__VA_ARGS__, SMTD_LT5_ON_MKEY, SMTD_LT4_ON_MKEY, SMTD_LT3_ON_MKEY)(__VA_ARGS__)
#define SMTD_LT3_ON_MKEY(...) SMTD_LT4_ON_MKEY(__VA_ARGS__, 1)
#define SMTD_LT4_ON_MKEY(...) SMTD_LT5_ON_MKEY(__VA_ARGS__, true)
#define SMTD_LT5_ON_MKEY(macro_key, tap_key, layer, threshold, use_cl)\
    SMTD_DANCE(macro_key,                                     \
        NOTHING,                                              \
        SMTD_TAP_16(use_cl, tap_key),                         \
        SMTD_LIMIT(threshold,                                 \
            layer_on(layer),                                  \
            SMTD_REGISTER_16(use_cl, tap_key)),               \
        SMTD_LIMIT(threshold,                                 \
            layer_off(layer),                                 \
            SMTD_UNREGISTER_16(use_cl, tap_key));             \
    )

#define SMTD_TD(...) OVERLOAD4(__VA_ARGS__, SMTD_TD4, SMTD_TD3, SMTD_TD2)(__VA_ARGS__)
#define SMTD_TD2(key, tap_key) SMTD_TD3_ON_MKEY(key, key, tap_key)
#define SMTD_TD3(key, tap_key, threshold) SMTD_TD4_ON_MKEY(key, key, tap_key, threshold)
#define SMTD_TD4(key, tap_key, threshold, use_cl) SMTD_TD5_ON_MKEY(key, key, tap_key, threshold, use_cl)
#define SMTD_TD_ON_MKEY(...) OVERLOAD5(__VA_ARGS__, SMTD_TD5_ON_MKEY, SMTD_TD4_ON_MKEY, SMTD_TD3_ON_MKEY)(__VA_ARGS__)
#define SMTD_TD3_ON_MKEY(...) SMTD_TD4_ON_MKEY(__VA_ARGS__, 1)
#define SMTD_TD4_ON_MKEY(...) SMTD_TD5_ON_MKEY(__VA_ARGS__, true)
#define SMTD_TD5_ON_MKEY(macro_key, tap_key, hold_key, threshold, use_cl)\
    SMTD_DANCE(macro_key,                                        \
        NOTHING,                                                 \
        SMTD_TAP_16(use_cl, tap_key),                            \
        SMTD_LIMIT(threshold,                                    \
            SMTD_TAP_16(use_cl, hold_key),                       \
            SMTD_TAP_16(use_cl, tap_key)),                       \
        SMTD_LIMIT(threshold,                                    \
            SMTD_UNREGISTER_16(use_cl, hold_key),                \
            SMTD_UNREGISTER_16(use_cl, tap_key))                 \
    )

// multi-tap activated key
#define SMTD_TK(...) OVERLOAD4(__VA_ARGS__, SMTD_TK4, SMTD_TK3, SMTD_TK2)(__VA_ARGS__)
#define SMTD_TK2(key, tap_key) SMTD_TK2_ON_MKEY(key, tap_key)
#define SMTD_TK3(key, tap_key, threshold) SMTD_TK3_ON_MKEY(key, tap_key, threshold)
#define SMTD_TK4(key, tap_key, threshold, use_cl) SMTD_TK4_ON_MKEY(key, tap_key, threshold, use_cl)
#define SMTD_TK_ON_MKEY(...) OVERLOAD4(__VA_ARGS__, SMTD_TK4_ON_MKEY, SMTD_TK3_ON_MKEY, SMTD_TK2_ON_MKEY)(__VA_ARGS__)
#define SMTD_TK2_ON_MKEY(...) SMTD_TK3_ON_MKEY(__VA_ARGS__, 1)
#define SMTD_TK3_ON_MKEY(...) SMTD_TK4_ON_MKEY(__VA_ARGS__, true)
#define SMTD_TK4_ON_MKEY(macro_key, tap_key, threshold, use_cl) \
    SMTD_DANCE(macro_key,                              \
        SMTD_LIMIT(threshold,                          \
            NOTHING,                                   \
            SMTD_TAP_16(use_cl, tap_key)),             \
        NOTHING,                                       \
        NOTHING,                                       \
        NOTHING                                        \
    )

// multi-tap activated layer move
#define SMTD_TTO(...) OVERLOAD4(__VA_ARGS__, SMTD_TTO4, SMTD_TTO3, SMTD_TTO2)(__VA_ARGS__)
#define SMTD_TTO2(key, layer) SMTD_TTO2_ON_MKEY(key, layer)
#define SMTD_TTO3(key, layer, threshold) SMTD_TTO3_ON_MKEY(key, layer, threshold)
#define SMTD_TTO4(key, layer, threshold, use_cl) SMTD_TTO4_ON_MKEY(key, layer, threshold, use_cl)
#define SMTD_TTO_ON_MKEY(...) OVERLOAD4(__VA_ARGS__, SMTD_TTO4_ON_MKEY, SMTD_TTO3_ON_MKEY, SMTD_TTO2_ON_MKEY)(__VA_ARGS__)
#define SMTD_TTO2_ON_MKEY(...) SMTD_TTO3_ON_MKEY(__VA_ARGS__, 1)
#define SMTD_TTO3_ON_MKEY(...) SMTD_TTO4_ON_MKEY(__VA_ARGS__, true)
#define SMTD_TTO4_ON_MKEY(macro_key, layer, threshold, use_cl) \
    SMTD_DANCE(macro_key,                              \
        SMTD_LIMIT(threshold,                          \
            NOTHING,                                   \
            layer_move(layer)),                        \
        NOTHING,                                       \
        NOTHING,                                       \
        NOTHING                                        \
    )


