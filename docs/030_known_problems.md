# Known problems and current limitations

This page describes behavior that is known, intentional for now, or still on the development backlog. For setup failures, see [Debugging](040_debugging.md).

## Combo support is partial

SM_TD supports combo interactions only when the resolved tap can travel through the regular QMK processing pipeline. Custom or derived tap keycodes can use the direct-output fallback, which does not provide a complete synthetic combo record.

QMK's supported action-combo API is `COMBO_ACTION` with `process_combo_event()`. Simple `COMBO()` behavior is not guaranteed for every SM_TD output path. Better combo support is tracked upstream in [issue 41](https://github.com/stasmarkin/sm_td/issues/41).

## Pipeline taps replay part of QMK processing

SM_TD initially receives a physical press or release in `process_record_user()`. When the action resolves later, a native keymap tap can re-enter `process_record()` so Caps Word, Leader, Auto Shift, Key Overrides, and similar features can observe it.

As a result, code before `process_record_user()` can observe the original physical event and the later emulated event. Code with side effects should check its keycode, event position, or SM_TD integration point carefully.

Pipeline replay is enabled by `SMTD_GLOBAL_PIPELINE_TAPS`. It can be disabled for a specific key with `SMTD_FEATURE_PIPELINE_TAPS` when duplicate processing causes an unwanted side effect.

## Derived keycodes have a reduced fallback path

When the tap keycode differs from the keycode at the pressed matrix position, SM_TD cannot safely replay that key as a normal matrix event. It sends the derived keycode directly and applies feature-specific compatibility where available.

Current consequences:

- Caps Word receives a compatibility pass.
- Leader taps are added to the Leader sequence.
- Combos, Repeat Key, and other features that depend on a complete matrix record may not see identical behavior.

The production marker near `smtd_emulate_key()` tracks a possible combo-style record for row and column `(0, 0)` events.

## Caps Word and held modifiers can differ from native QMK

A held SM_TD mod-tap registers its modifier directly. QMK Caps Word may therefore not observe the same tap-hold transition it observes for a native QMK mod-tap. In particular, a held non-shift modifier can remain invisible to Caps Word in some configurations.

Use the Caps Word integration tests as the compatibility baseline and test custom `caps_word_press_user()` behavior on the target keymap.

## Leader captures taps, not holds

Native keymap taps reach Leader through the regular pipeline. Derived taps use SM_TD's Leader compatibility path. Holding a custom mod-tap during a Leader sequence is not added to the Leader buffer because Leader is a tap-sequence feature.

## Development backlog markers

The core currently contains four production follow-ups:

- Cleanup of a state removed while iterating the active-state stack
- Ordering or replacement of a repeated key already in `SMTD_STAGE_SEQUENCE`
- Explicit coverage for a new press while another key is in `SMTD_STAGE_TOUCH_RELEASE`
- Richer emulation records for keys without a real matrix position

These are design or coverage tasks, not confirmed security vulnerabilities. Changes in these areas need focused unit tests and, when the QMK pipeline is involved, a native integration test.

## Open upstream feature requests

The upstream issue tracker currently includes requests for OSL, Layer Lock, tri-layer behavior, JSON-to-C support, easier configuration, examples, a documentation site, VIA/Vial improvements, and better combo support. Review the current issue and discussion before implementing one:

<https://github.com/stasmarkin/sm_td/issues>
