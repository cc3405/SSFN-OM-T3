import unittest

from redesign.tools.design_gate import (
    BUDGET_LIMIT,
    budget_total,
    score_target_is_valid,
    score_target_total,
    show_duration_seconds,
    validate,
)


class DesignGateTests(unittest.TestCase):
    def test_budget_is_below_limit(self):
        self.assertEqual(budget_total(), 135)
        self.assertLessEqual(budget_total(), BUDGET_LIMIT)

    def test_score_target_is_within_category_maxima(self):
        self.assertTrue(score_target_is_valid())
        self.assertEqual(score_target_total(), 171)

    def test_show_script_has_time_reserve(self):
        self.assertEqual(show_duration_seconds(), 440)
        self.assertLessEqual(show_duration_seconds(), 440)

    def test_all_arithmetic_gates_pass(self):
        self.assertEqual(validate(), [])


if __name__ == "__main__":
    unittest.main()
