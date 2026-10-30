"""Dependency-free design gate for the Group II OMER solution.

Checks arithmetic and the scripted time budget only. It does not certify
rule compliance, mechanical safety, cost-accounting treatment, or score awards.
"""
BUDGET_LIMIT = 165
BUDGET_LINES = {
    "frame": 12, "wheels": 12, "axle_bearings": 10,
    "crank_chain_sprockets": 15, "steering": 6, "brakes": 8,
    "seat_footrest": 5, "fasteners_guards": 6, "shopping_cart": 8,
    "tow_coupler": 3, "elastic_return": 8, "return_visual_speed_control": 4,
    "unloader_frame": 10, "winch_pulleys_rope": 8, "three_items": 5,
    "costumes_set": 3, "music_props": 2, "contingency": 10,
}
SCORE_TARGETS = {
    "overall_creativity": 16, "overall_quality": 12,
    "propulsion_creativity": 20, "vehicle_engineering": 19,
    "shopping_checkout": 10, "all_three_items": 10,
    "substitution": 5, "shopping_list_creativity": 8,
    "assistant_list": 5, "assistant_accuracy": 5,
    "assistant_substitution": 5, "assistant_performance": 12,
    "unload_all_three": 12, "unload_creativity": 10,
    "cart_return_success": 10, "cart_return_creativity": 12,
}
MAX_SCORE = {
    "overall_creativity": 20, "overall_quality": 15,
    "propulsion_creativity": 25, "vehicle_engineering": 25,
    "shopping_checkout": 10, "all_three_items": 10,
    "substitution": 5, "shopping_list_creativity": 10,
    "assistant_list": 5, "assistant_accuracy": 5,
    "assistant_substitution": 5, "assistant_performance": 15,
    "unload_all_three": 12, "unload_creativity": 13,
    "cart_return_success": 10, "cart_return_creativity": 15,
}
SHOW_SEGMENTS_SECONDS = {
    "setup": 25, "story_and_order": 30, "attach_and_start": 20,
    "shopping": 70, "substitution": 40, "checkout_travel": 45,
    "stop_and_position": 20, "unloading": 65, "assistant_check": 30,
    "cart_return": 65, "ending": 30,
}


def budget_total(lines=BUDGET_LINES):
    return sum(lines.values())


def score_target_total(targets=SCORE_TARGETS):
    return sum(targets.values())


def score_target_is_valid():
    return all(0 <= SCORE_TARGETS[key] <= MAX_SCORE[key] for key in SCORE_TARGETS)


def show_duration_seconds(segments=SHOW_SEGMENTS_SECONDS):
    return sum(segments.values())


def validate():
    errors = []
    total = budget_total()
    if total > BUDGET_LIMIT:
        errors.append("Budget total {} exceeds limit {}.".format(total, BUDGET_LIMIT))
    if total != 135:
        errors.append("Budget baseline changed: expected 135, got {}.".format(total))
    if not score_target_is_valid():
        errors.append("One or more score targets exceed official category maxima.")
    score = score_target_total()
    if score != 171:
        errors.append("Score target arithmetic changed: expected 171, got {}.".format(score))
    duration = show_duration_seconds()
    if duration > 480:
        errors.append("Script duration {}s exceeds 480s.".format(duration))
    if duration > 440:
        errors.append("Script duration {}s leaves less than 40s reserve.".format(duration))
    return errors


if __name__ == "__main__":
    problems = validate()
    print("Budget USD: {} / {}".format(budget_total(), BUDGET_LIMIT))
    print("Problem-solving score target: {} / 200".format(score_target_total()))
    print("Script duration: {}s / 480s".format(show_duration_seconds()))
    if problems:
        for problem in problems:
            print("FAIL: {}".format(problem))
        raise SystemExit(1)
    print("PASS: arithmetic gates only; not a safety or rules certification.")
