import ctypes
import unittest

from tests.unit.sm_td_bindings import load_smtd_lib


smtd = load_smtd_lib("tests/unit/debug_output/layout.c")
smtd.lib.TEST_format_keycode_uncertain.argtypes = [ctypes.c_uint16]
smtd.lib.TEST_format_keycode_uncertain.restype = ctypes.c_char_p
smtd.lib.TEST_format_state.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
smtd.lib.TEST_format_state.restype = ctypes.c_char_p


class TestDebugOutput(unittest.TestCase):
    def test_uncertain_custom_keycode_keeps_prefix(self):
        result = smtd.lib.TEST_format_keycode_uncertain(100).decode()
        self.assertEqual(result, "?CUSTOM_100")

    def test_state_uses_distinct_pressed_and_desired_names(self):
        result = smtd.lib.TEST_format_state(100, 101).decode()
        self.assertIn("CUSTOM_100->CUSTOM_101", result)


if __name__ == "__main__":
    unittest.main()
