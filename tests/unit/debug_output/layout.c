/* Focused debug-formatter regression tests. */
#define SMTD_UNIT_TEST
#define SMTD_TEST_DEBUG
#define MATRIX_ROWS 1
#define MATRIX_COLS 1
#define TAPPING_TERM 200

#include "../sm_td_bindings.c"

uint16_t const keymaps[][MATRIX_ROWS][MATRIX_COLS] = {{{100}}};

smtd_resolution on_smtd_action(uint16_t keycode, smtd_action action, uint8_t tap_count) {
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
    TEST_snprintf(buffer, sizeof(buffer), "CUSTOM_%d", keycode);
    return buffer;
}

const char *TEST_format_keycode_uncertain(uint16_t keycode) {
    return smtd_keycode_to_str_uncertain(keycode, true);
}

const char *TEST_format_state(uint16_t pressed_keycode, uint16_t desired_keycode) {
    smtd_state state = EMPTY_STATE;
    state.pressed_keycode = pressed_keycode;
    state.desired_keycode = desired_keycode;
    return smtd_state_to_str(&state);
}

void post_register_code16(uint16_t keycode) {}
void post_unregister_code16(uint16_t keycode) {}
void post_process_record(keyrecord_t *record) {}
