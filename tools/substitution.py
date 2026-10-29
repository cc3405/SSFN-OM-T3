#!/usr/bin/env python3
"""Deterministic, hardware-independent OMER substitute selector.

Input JSON example:
{"item":"milk","category":"dairy","out_of_stock":true,"budget_remaining":4.0,
 "candidates":[{"id":"soy_milk","category":"dairy","price":2.5},
               {"id":"juice","category":"drink","price":1.2}]}

This tool only demonstrates a deterministic decision rule. It does not claim
to sense shelf inventory, control a robot, or establish competition compliance.
"""
from __future__ import annotations

import argparse
import json
import sys
from decimal import Decimal, InvalidOperation
from typing import Any


def _money(value: Any) -> Decimal | None:
    try:
        amount = Decimal(str(value))
    except (InvalidOperation, ValueError, TypeError):
        return None
    if not amount.is_finite() or amount < 0:
        return None
    return amount


def choose_substitute(order: dict[str, Any]) -> dict[str, Any]:
    """Choose the lowest-priced in-category candidate within remaining budget.

    Ties are broken by candidate ID in lexical order. If the requested item is
    available, no substitute is selected. Invalid prices and candidates without
    an ID/category are ignored. Missing/invalid budget yields a safe no-choice.
    """
    item = order.get("item")
    category = order.get("category")
    if not isinstance(item, str) or not item.strip():
        return {"status": "invalid_input", "reason": "item_required"}
    if not isinstance(category, str) or not category.strip():
        return {"status": "invalid_input", "reason": "category_required"}
    if order.get("out_of_stock") is not True:
        return {"status": "original_item", "item": item, "reason": "item_available"}

    budget = _money(order.get("budget_remaining"))
    if budget is None:
        return {"status": "needs_assistance", "reason": "invalid_or_missing_budget"}

    candidates = order.get("candidates")
    if not isinstance(candidates, list):
        return {"status": "needs_assistance", "reason": "candidate_list_required"}

    eligible = []
    for candidate in candidates:
        if not isinstance(candidate, dict):
            continue
        candidate_id = candidate.get("id")
        candidate_category = candidate.get("category")
        price = _money(candidate.get("price"))
        if not isinstance(candidate_id, str) or not candidate_id.strip():
            continue
        if candidate_category != category or price is None or price > budget:
            continue
        eligible.append((price, candidate_id, candidate))

    if not eligible:
        return {
            "status": "needs_assistance",
            "reason": "no_eligible_substitute",
            "original_item": item,
        }

    price, candidate_id, candidate = sorted(eligible, key=lambda row: (row[0], row[1]))[0]
    return {
        "status": "substitute_selected",
        "original_item": item,
        "substitute_id": candidate_id,
        "price": str(price),
        "reason": "same_category_lowest_price_within_budget",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", help="JSON input file; stdin when omitted")
    args = parser.parse_args()
    try:
        if args.input:
            with open(args.input, encoding="utf-8") as handle:
                order = json.load(handle)
        else:
            order = json.load(sys.stdin)
        if not isinstance(order, dict):
            raise ValueError("top-level JSON value must be an object")
        result = choose_substitute(order)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(json.dumps({"status": "invalid_input", "reason": str(exc)}, ensure_ascii=False))
        return 2
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))
    return 0 if result["status"] != "invalid_input" else 2


if __name__ == "__main__":
    raise SystemExit(main())
