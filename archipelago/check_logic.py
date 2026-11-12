"""Every generated Project Midas seed has to be finishable.

The generator places a required item only on a location that can already be
reached without that item. This walks the same rules as the world and rejects
a setting only when that placement cannot reach every location.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "projectmidas"))

import data  # noqa: E402


def run_one(goal, goal_difficulty, checks_mode, bonus, objectives, cheats, split, gating, start, golden):
    try:
        diffs = data.check_diffs(checks_mode)
        data.goal_diffs(goal_difficulty, diffs)
    except ValueError:
        return "rejected"
    mission_count = 20 if bonus or goal in (data.GOAL_AZTEC, data.GOAL_EGYPTIAN) else data.CAMPAIGN_COUNT
    rule_diffs = data.logic_diffs(diffs, cheats, split)
    tool_map = data.tool_rules(rule_diffs, mission_count, objectives) if gating else {}
    if golden:
        tool_map = data.with_golden_gun(tool_map, rule_diffs, mission_count)
    locs = data.build_locations(mission_count, diffs, split, objectives, cheats, tool_map)
    owned_start = data.ensure_open_start(list(start), diffs, tool_map)
    start_items = data.starting_items(owned_start, diffs, split)
    pool = data.progression_pool(locs, start_items)
    if len(pool) > len(locs):
        return "rejected"
    if data.early_fill(start_items, pool, locs):
        return "ok"
    if golden and "Golden Gun" in pool:
        start_items = list(start_items) + ["Golden Gun"]
        pool = data.progression_pool(locs, start_items)
        if len(pool) > len(locs):
            return "rejected"
        if data.early_fill(start_items, pool, locs):
            return "ok"
    return "stuck"


def main():
    failures = []
    rejected = 0
    ok = 0
    for goal in range(4):
        for goal_difficulty in range(4):
            for checks_mode in range(4):
                for bonus in (False, True):
                    for objectives in (False, True):
                        for cheats in (False, True):
                            for split in (False, True):
                                for gating in (False, True):
                                    for golden in (False, True):
                                        for start in ([0], [1], [4]):
                                            result = run_one(
                                                goal, goal_difficulty, checks_mode, bonus,
                                                objectives, cheats, split, gating, start, golden,
                                            )
                                            if result == "rejected":
                                                rejected += 1
                                            elif result == "stuck":
                                                failures.append(
                                                    (goal, goal_difficulty, checks_mode, bonus,
                                                     objectives, cheats, split, gating, golden, start)
                                                )
                                            else:
                                                ok += 1

    tool_map = data.tool_rules((0, 1, 2), 20, False)
    locs = data.build_locations(20, (0, 1, 2), False, False, False, tool_map)
    raw = data.starting_items([4], (0, 1, 2), False)
    if data.early_fill(raw, data.progression_pool(locs, raw), locs):
        raise SystemExit("a Bunker-only start with mission tools off should not fill")
    fixed = data.ensure_open_start([4], (0, 1, 2), tool_map)
    fixed_items = data.starting_items(fixed, (0, 1, 2), False)
    if not data.early_fill(fixed_items, data.progression_pool(locs, fixed_items), locs):
        raise SystemExit("replacing a tool-locked start did not make the seed finishable")

    gated = [4, 5, 6, 8, 10, 13]
    diffs = (2,)
    tool_map = data.tool_rules(diffs, 20, False)
    locs = data.build_locations(20, diffs, True, False, True, tool_map)
    raw = data.starting_items(gated, diffs, True)
    if data.early_fill(raw, data.progression_pool(locs, raw), locs):
        raise SystemExit("six tool-locked starts should not fill")
    fixed = data.ensure_open_start(list(gated), diffs, tool_map)
    fixed_items = data.starting_items(fixed, diffs, True)
    if not data.early_fill(fixed_items, data.progression_pool(locs, fixed_items), locs):
        raise SystemExit("replacing one of six tool-locked starts did not make the seed finishable")

    if failures:
        print(f"{len(failures)} settings cannot be finished")
        for row in failures[:20]:
            print(" ", row)
        raise SystemExit(1)
    print(f"check_logic ok ({ok} finishable, {rejected} rejected)")


if __name__ == "__main__":
    main()
