import unittest

from tools.substitution import choose_substitute


class SubstituteSelectorTests(unittest.TestCase):
    def test_available_original_item_is_kept(self):
        result = choose_substitute({
            "item": "milk", "category": "dairy", "out_of_stock": False,
            "budget_remaining": 3, "candidates": [],
        })
        self.assertEqual(result["status"], "original_item")

    def test_cheapest_same_category_within_budget(self):
        result = choose_substitute({
            "item": "milk", "category": "dairy", "out_of_stock": True,
            "budget_remaining": "3.00",
            "candidates": [
                {"id": "soy", "category": "dairy", "price": 2.2},
                {"id": "oat", "category": "dairy", "price": 2.0},
                {"id": "juice", "category": "drink", "price": 0.5},
                {"id": "too_expensive", "category": "dairy", "price": 3.1},
            ],
        })
        self.assertEqual(result["status"], "substitute_selected")
        self.assertEqual(result["substitute_id"], "oat")
        self.assertEqual(result["price"], "2.0")

    def test_tie_break_is_deterministic(self):
        order = {
            "item": "milk", "category": "dairy", "out_of_stock": True,
            "budget_remaining": 5,
            "candidates": [
                {"id": "soy", "category": "dairy", "price": 2},
                {"id": "oat", "category": "dairy", "price": 2},
            ],
        }
        self.assertEqual(choose_substitute(order), choose_substitute(order))
        self.assertEqual(choose_substitute(order)["substitute_id"], "oat")

    def test_no_candidate_fails_safe(self):
        result = choose_substitute({
            "item": "milk", "category": "dairy", "out_of_stock": True,
            "budget_remaining": 1,
            "candidates": [{"id": "soy", "category": "dairy", "price": 2}],
        })
        self.assertEqual(result["status"], "needs_assistance")
        self.assertEqual(result["reason"], "no_eligible_substitute")

    def test_invalid_budget_fails_safe(self):
        result = choose_substitute({
            "item": "milk", "category": "dairy", "out_of_stock": True,
            "budget_remaining": -1, "candidates": [],
        })
        self.assertEqual(result["status"], "needs_assistance")

    def test_invalid_required_fields_are_rejected(self):
        self.assertEqual(choose_substitute({})["status"], "invalid_input")


if __name__ == "__main__":
    unittest.main()
