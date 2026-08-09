# Adding and Maintaining Tests

SM_TD uses two complementary test layers:

1. **Unit tests** in `tests/unit/` compile the engine with small QMK mocks. They are fast and precise.
2. **Integration tests** in `tests/integration/` compile against QMK's native test harness. They verify real pipeline behavior.

Use a unit test for state-machine behavior. Add an integration test when a change depends on QMK features such as Caps Word, VIA, Leader, layers, or the `process_record` pipeline.

## Run tests

```sh
python3 tests/run_tests.py                         # all unit tests
python3 -m unittest tests.unit.qmk_taphold.test   # one unit suite
SMTD_DEBUG=1 python3 -m unittest tests.unit.qmk_taphold.test  # engine trace
just test qmk full                                # one QMK integration suite
just test qmk                                     # every QMK integration suite
```

The unit-test command exits nonzero on a failure, import error, or C compilation error. The compiler is selected from `CC`, then `clang`, `cc`, or `gcc`.

## Unit suite layout

Create a focused directory under `tests/unit/<feature>/`:

```text
tests/unit/<feature>/
  __init__.py
  layout.c       Minimal keymap, configuration, and hooks
  test.py        Scenarios and expected behavior
```

Keep each suite about one feature. Prefer descriptive scenario names such as `test_same_hand_roll_resolves_as_taps` over implementation names.

### Minimal `layout.c`

```c
#define SMTD_UNIT_TEST
#define MATRIX_ROWS 1
#define MATRIX_COLS 3
#define TAPPING_TERM 200

#include "../sm_td_bindings.c"

enum { L0 = 0 };
enum { K_A = 100, K_B, K_C };

uint16_t const keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [L0] = {{K_A, K_B, K_C}},
};

smtd_resolution on_smtd_action(uint16_t keycode, smtd_action action,
                               uint8_t tap_count) {
    switch (keycode) {
        SMTD_MT(K_A, KC_LEFT_CTRL)
    }
    return SMTD_RESOLUTION_UNHANDLED;
}

uint32_t get_smtd_timeout(uint16_t keycode, smtd_timeout timeout) {
    return get_smtd_timeout_default(timeout);
}

bool smtd_feature_enabled(uint16_t keycode, smtd_feature feature) {
    return smtd_feature_enabled_default(keycode, feature);
}

char *smtd_keycode_to_str_user(uint16_t keycode) {
    static char buffer[16];
    TEST_snprintf(buffer, sizeof(buffer), "KC_%d", keycode);
    return buffer;
}

void post_register_code16(uint16_t keycode) {}
void post_unregister_code16(uint16_t keycode) {}
void post_process_record(keyrecord_t *record) {}
```

### Minimal `test.py`

```python
import unittest

from tests.unit.sm_td_assertions import EmulatePress, EmulateRelease, SmTdAssertions
from tests.unit.sm_td_bindings import Key, Keycode, load_smtd_lib

smtd = load_smtd_lib("tests/unit/<feature>/layout.c")
K_A_CODE = Keycode(smtd, 100, 0, 0, 0)
K_B_CODE = Keycode(smtd, 101, 0, 1, 0)
K_A = Key(smtd, "K_A", 0, 0, "Ctrl when held", [K_A_CODE])
K_B = Key(smtd, "K_B", 0, 1, "plain key", [K_B_CODE])


class TestFeature(SmTdAssertions):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.smtd = smtd

    def setUp(self):
        super().setUp()
        for keycode in (K_A_CODE, K_B_CODE):
            keycode.reset()
        for key in (K_A, K_B):
            key.reset()
        smtd.reset()

    def test_plain_key_passes_through(self):
        self.assertFalse(K_B.press())
        self.assertFalse(K_B.release())
        self.assertHistory(EmulatePress(K_B), EmulateRelease(K_B))


if __name__ == "__main__":
    unittest.main()
```

## Write readable scenarios

Use arrange, act, assert ordering inside each test:

```python
def test_release_after_dynamic_window_resolves_as_taps(self):
    # Arrange and act
    MT_CTRL.press()
    smtd.wait(30)
    LETTER.press()
    MT_CTRL.release()
    smtd.wait(25)
    LETTER.release()

    # Assert
    self.assertHistory(...)
    self.assertEqual(smtd.get_mods(), 0)
```

Guidelines:

- Assert observable behavior, not private state, unless the private state is the subject of a focused invariant test.
- Include the reason in assertion messages when the expected result is not obvious.
- Use `smtd.wait(ms)` for timing behavior. The virtual clock advances only when `wait()` is called.
- Use `prolong()` only when the scenario specifically needs to fire the current deferred callback immediately.
- Reset every `Key`, `Keycode`, and the SM_TD runtime in `setUp()`.
- Let `SmTdAssertions.tearDown()` verify that modifiers, layers, and deferred callbacks do not leak.
- Avoid random timing unless the test uses a fixed seed and also covers boundary values directly.
- Add a regression test that fails before the fix and passes after it.

## Return-value contract

`process_smtd()` follows the QMK convention:

- `false` means SM_TD handled the event and QMK should stop processing it.
- `true` means QMK should continue processing the event.

Capacity and allocation tests should verify this contract explicitly. In particular, an event that SM_TD cannot track must fail open instead of disappearing.

## Integration suites

Integration suites live in `tests/integration/suites/smtd_<feature>/`. Each contains:

- `test.mk` for QMK build flags and sources
- `config.h` for suite-specific configuration
- `smtd_hooks.c` for SM_TD hooks
- `test_*.cpp` for QMK googletest scenarios

Every keyboard test must keep a `TestDriver driver;` alive for the full test body, and every fixture must call `smtd_reset()` from `SetUp()`. See [`tests/integration/README.md`](../tests/integration/README.md) for the complete harness description.

## Before opening a pull request

```sh
python3 tests/run_tests.py
python3 -m compileall -q tests
just test qmk <affected-suite>
```

Run all QMK suites for changes to shared pipeline, state-management, or timing code. State exactly which commands and QMK version you tested in the pull request.
