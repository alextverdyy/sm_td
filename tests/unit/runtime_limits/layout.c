/* Runtime-capacity tests for fail-open behavior. */
#define SMTD_UNIT_TEST
#define MATRIX_ROWS 1
#define MATRIX_COLS 11
#define TAPPING_TERM 200

#include "../sm_td_bindings.c"

enum { L0 = 0 };

enum {
    K00 = 100, K01, K02, K03, K04, K05,
    K06, K07, K08, K09, K10,
};

uint16_t const keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [L0] = {{K00, K01, K02, K03, K04, K05, K06, K07, K08, K09, K10}},
};

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
    TEST_snprintf(buffer, sizeof(buffer), "KC_%d", keycode);
    return buffer;
}

void post_register_code16(uint16_t keycode) {}
void post_unregister_code16(uint16_t keycode) {}
void post_process_record(keyrecord_t *record) {}
