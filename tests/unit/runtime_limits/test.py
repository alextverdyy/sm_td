import unittest

from tests.unit.sm_td_assertions import SmTdAssertions
from tests.unit.sm_td_bindings import Keycode, load_smtd_lib


smtd = load_smtd_lib("tests/unit/runtime_limits/layout.c")


class TestRuntimeLimits(SmTdAssertions):
    """Capacity failures must pass events back to QMK instead of losing input."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.smtd = smtd

    def setUp(self):
        super().setUp()
        smtd.reset()
        self.keys = [Keycode(smtd, 100 + col, 0, col, 0) for col in range(11)]

    def test_pool_exhaustion_fails_open_for_press_and_release(self):
        for key in self.keys[:10]:
            self.assertFalse(key.press(), "tracked presses are handled by sm_td")

        overflow = self.keys[10]
        self.assertTrue(overflow.press(), "an untracked press must continue through QMK")
        self.assertTrue(overflow.release(), "its unmatched release must also continue")

    def test_deferred_executor_failure_resolves_without_stranding_state(self):
        key = self.keys[0]
        smtd.fail_next_deferred_exec()

        self.assertFalse(key.press())
        self.assertEqual(smtd.get_deferred_execs(), [])
        self.assertFalse(key.release())

        history = smtd.get_record_history()
        self.assertEqual([event["pressed"] for event in history], [True, False])

    def test_sequence_timeout_allocation_failure_cleans_up_immediately(self):
        key = self.keys[0]
        self.assertFalse(key.press())
        smtd.fail_next_deferred_exec()
        self.assertFalse(key.release())

        self.assertTrue(all(not item["active"] for item in smtd.get_deferred_execs()))
        key.reset()
        self.assertFalse(key.press(), "the released state must no longer occupy the pool")
        self.assertFalse(key.release())


if __name__ == "__main__":
    unittest.main()
