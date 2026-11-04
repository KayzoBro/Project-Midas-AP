"""Ids and names shared with port/include/ap_ids.h and port/src/ap_client.c."""

from collections import namedtuple

MISSIONS = [
    "Dam",
    "Facility",
    "Runway",
    "Surface",
    "Bunker",
    "Silo",
    "Frigate",
    "Surface 2",
    "Bunker 2",
    "Statue",
    "Archives",
    "Streets",
    "Depot",
    "Train",
    "Jungle",
    "Control",
    "Caverns",
    "Cradle",
    "Aztec",
    "Egyptian",
]

DIFFICULTIES = ["Agent", "Secret Agent", "00 Agent"]

WEAPONS = [
    "PP7",
    "PP7 (silenced)",
    "DD44 Dostovei",
    "Klobb",
    "KF7 Soviet",
    "ZMG (9mm)",
    "D5K Deutsche",
    "D5K (silenced)",
    "Phantom",
    "AR33 Assault Rifle",
    "RC-P90",
    "Shotgun",
    "Automatic Shotgun",
    "Sniper Rifle",
    "Cougar Magnum",
    "Golden Gun",
    "Moonraker Laser",
    "Grenade Launcher",
    "Rocket Launcher",
    "Hand Grenade",
    "Timed Mine",
    "Proximity Mine",
    "Remote Mine",
    "Throwing Knife",
]

# A mission cannot be cleared on explosives alone: they run out, and a launcher
# is not a gun you can fight a level with. Loose copies stay on the floor, so
# the multiworld has to place a firearm in an early sphere. It is not added to
# the starting inventory.
FIREARMS = WEAPONS[:17]
EXPLOSIVES = (
    "Grenade Launcher",
    "Rocket Launcher",
    "Hand Grenade",
    "Timed Mine",
    "Proximity Mine",
    "Remote Mine",
)

TRAPS = [
    "Camera Alert",
    "Jammed Clip",
    "Slipped on the Ice",
    "Baron Samedi",
    "Greasy Hands",
    "Restart Mission",
    "Tiny Bond",
    "The Living Daylights",
    "I am invisible",
    "DK Mode",
    "Paintball",
    "Single Bullet",
]

ITEM_ARMOR = 640201
ITEM_AMMO = 640202
ITEM_CLIP = 640203
ITEM_ALERT = 640301
ITEM_JAM = 640302
ITEM_SLIP = 640303
ITEM_BARON = 640304
ITEM_GREASY = 640305
ITEM_RESTART = 640306
ITEM_TINY = 640307
ITEM_FLAG = 640308
ITEM_BORIS = 640309
ITEM_DK = 640310
ITEM_PAINT = 640311
ITEM_BULLET = 640312

TOOLS = [
    ("Watch Magnet", 640401),
    ("Watch Laser", 640402),
    ("Key Analyser", 640403),
    ("Camera", 640404),
    ("Datathief", 640405),
    ("Plastique", 640406),
    ("Bomb Defuser", 640407),
    ("Tracker Bug", 640408),
    ("Explosive Floppy", 640409),
]

CAMPAIGN_COUNT = 18
GOAL_CRADLE = 0
GOAL_AZTEC = 1
GOAL_EGYPTIAN = 2
GOAL_CAMPAIGN = 3
CHECKS_ALL = 0
CHECKS_AGENT = 1
CHECKS_SECRET = 2
CHECKS_00 = 3
GOAL_DIFF_ALL = 3
DIFF_DOSSIER_BASE = 640501


def dossier_name(index):
    return f"{MISSIONS[index]} Dossier"


def dossier_id(index):
    return 640001 + index


def dossier_diff_name(mission, difficulty):
    return f"{MISSIONS[mission]} Dossier ({DIFFICULTIES[difficulty]})"


def dossier_diff_id(mission, difficulty):
    return DIFF_DOSSIER_BASE + mission * 3 + difficulty


def check_diffs(mode):
    """Difficulties that send a mission check. 0 all, 1 agent, 2 secret, 3 00."""
    return {0: (0, 1, 2), 1: (0,), 2: (1,), 3: (2,)}[int(mode)]


def logic_diffs(checks, cheat_checks, split):
    """Difficulties whose tool requirements can appear on a location.

    Split dossiers lock every difficulty that is not a check, so a cheat on
    another difficulty is not in the seed. One shared dossier leaves every
    difficulty playable, and a cheat target on one of them needs its tools.
    """
    checks = tuple(checks)
    if cheat_checks and not split:
        return (0, 1, 2)
    return checks


def goal_diffs(goal_difficulty, checks):
    """Goal difficulties. 3 means every check difficulty.

    A single difficulty that is not a check cannot be the goal: that location
    does not exist, so the seed could not be finished.
    """
    checks = tuple(checks)
    goal_difficulty = int(goal_difficulty)
    if goal_difficulty == GOAL_DIFF_ALL:
        return checks
    if goal_difficulty not in checks:
        have = ", ".join(DIFFICULTIES[d] for d in checks)
        raise ValueError(
            f"Goal difficulty {DIFFICULTIES[goal_difficulty]} is not a check. "
            f"This seed only checks {have}. Choose one of those, or All."
        )
    return (goal_difficulty,)


def weapon_id(index):
    return 640101 + index


def location_name(mission, difficulty):
    return f"{MISSIONS[mission]} ({DIFFICULTIES[difficulty]})"


def location_id(mission, difficulty):
    return 641000 + mission * 10 + difficulty


def objective_location_id(mission, diff, slot):
    return 642000 + mission * 40 + diff * 10 + slot


def objective_location_name(mission, text, diff=None):
    base = f"{MISSIONS[mission]} - {text}"
    if diff is None:
        return base
    return f"{base} ({DIFFICULTIES[diff]})"


# Menu names, in cheat-id order. 0-19 are the target-time cheats, one per mission.
# 20-22 are the completion cheats (Magnum, Laser, Golden Gun).
CHEATS = [
    "Dam - Paintball Mode",
    "Facility - Invincible",
    "Runway - DK Mode",
    "Surface - 2x Grenade Launcher",
    "Bunker - 2x Rocket Launcher",
    "Silo - Turbo Mode",
    "Frigate - No Radar",
    "Surface 2 - Tiny Bond",
    "Bunker 2 - 2x Throwing Knife",
    "Statue - Fast Animation",
    "Archives - Bond Invisible",
    "Streets - Enemy Rockets",
    "Depot - Slow Animation",
    "Train - Silver PP7",
    "Jungle - 2x Hunting Knife",
    "Control - Infinite Ammo",
    "Caverns - 2x RC-P90",
    "Cradle - Gold PP7",
    "Aztec - 2x Laser",
    "Egyptian - All Guns",
    "Cradle - Magnum",
    "Aztec - Laser",
    "Egyptian - Golden Gun",
]

# Which mission the cheat belongs to. The last three share a mission with a time cheat.
CHEAT_MISSION = list(range(20)) + [17, 18, 19]

# Target-time difficulty for cheats 0-19, from solo_target_time_array.
# 0 Agent, 1 Secret Agent, 2 00 Agent. The pattern in the cart is Secret, 00, Agent, repeating.
CHEAT_DIFF = [1, 2, 0] * 6 + [1, 2]


def cheat_location_id(index):
    return 643000 + index


def cheat_playable(index, diffs):
    """True when the cheat can be earned on one of these difficulties."""
    diffs = set(diffs)
    if index < 20:
        return CHEAT_DIFF[index] in diffs
    if index == 20:
        return bool(diffs)
    if index == 21:
        return bool(diffs & {1, 2})
    if index == 22:
        return 2 in diffs
    return False


def cheat_rows(mission_count, diffs=None, split=False):
    rows = []
    for index, name in enumerate(CHEATS):
        mission = CHEAT_MISSION[index]
        if mission >= mission_count:
            continue
        if split and diffs is not None and not cheat_playable(index, diffs):
            continue
        rows.append((mission, index, name))
    return rows


# Briefing order. The slot is the row in that mission's objective list.
OBJECTIVES = [
    (0, 0, "Neutralize all alarms"),
    (0, 1, "Install covert modem"),
    (0, 2, "Intercept data backup"),
    (0, 3, "Bungee jump from platform"),
    (1, 0, "Gain entry to lab area"),
    (1, 1, "Contact double agent"),
    (1, 2, "Rendezvous with 006"),
    (1, 3, "Destroy all tanks in bottling room"),
    (1, 4, "Minimize scientist casualties"),
    (2, 0, "Find plane ignition key"),
    (2, 1, "Destroy heavy gun emplacements"),
    (2, 2, "Destroy missile battery"),
    (2, 3, "Escape in plane"),
    (3, 0, "Power down communications dish"),
    (3, 1, "Obtain safe key"),
    (3, 2, "Steal building plans"),
    (3, 3, "Enter base via ventilation tower"),
    (4, 0, "Disrupt all surveillance equipment"),
    (4, 1, "Copy Goldeneye key and leave original"),
    (4, 2, "Get personnel to activate computer"),
    (4, 3, "Download data from computer"),
    (4, 4, "Photograph main video screen"),
    (5, 0, "Plant bombs in fuel rooms"),
    (5, 1, "Photograph satellite"),
    (5, 2, "Obtain telemetric data"),
    (5, 3, "Retrieve satellite circuitry"),
    (5, 4, "Minimize scientist casualties"),
    (6, 0, "Rescue Hostages"),
    (6, 1, "Disarm bridge bomb"),
    (6, 2, "Disarm engine room bomb"),
    (6, 3, "Plant tracking bug on helicopter"),
    (7, 0, "Disrupt all surveillance equipment"),
    (7, 1, "Break communications link to bunker"),
    (7, 2, "Disable Spetznaz support aircraft"),
    (7, 3, "Gain entry to bunker"),
    (8, 0, "Compare staff / casualty lists"),
    (8, 1, "Recover CCTV tape"),
    (8, 2, "Disable all security cameras"),
    (8, 3, "Recover Goldeneye operations manual"),
    (8, 4, "Escape with Natalya"),
    (9, 0, "Contact Valentin"),
    (9, 1, "Confront and unmask Janus"),
    (9, 2, "Locate helicopter"),
    (9, 3, "Rescue Natalya"),
    (9, 4, "Find flight recorder"),
    (10, 0, "Escape from interrogation room"),
    (10, 1, "Find Natalya"),
    (10, 2, "Recover helicopter black box"),
    (10, 3, "Escape with Natalya"),
    (11, 0, "Contact Valentin"),
    (11, 1, "Pursue Ourumov and Natalya"),
    (11, 2, "Minimize civilian casualties"),
    (12, 0, "Destroy illegal arms cache"),
    (12, 1, "Destroy computer network"),
    (12, 2, "Obtain safe key"),
    (12, 3, "Recover helicopter blueprints"),
    (12, 4, "Locate Trevelyan's train"),
    (13, 0, "Destroy brake units"),
    (13, 1, "Rescue Natalya"),
    (13, 2, "Locate Janus secret base"),
    (13, 3, "Crack Boris' password"),
    (13, 4, "Escape to safety"),
    (14, 0, "Destroy drone guns"),
    (14, 1, "Eliminate Xenia"),
    (14, 2, "Blow up ammo dump"),
    (14, 3, "Escort Natalya to Janus Base"),
    (15, 0, "Protect Natalya"),
    (15, 1, "Disable Goldeneye satellite"),
    (15, 2, "Destroy armored mainframes"),
    (16, 0, "Destroy inlet pump controls"),
    (16, 1, "Destroy outlet pump controls"),
    (16, 2, "Destroy master control console"),
    (16, 3, "Use radio to contact Jack Wade"),
    (16, 4, "Minimize scientist casualties"),
    (17, 0, "Destroy control console"),
    (17, 1, "Settle the score with Trevelyan"),
    (18, 0, "Reprogram shuttle guidance"),
    (18, 1, "Launch shuttle"),
    (19, 0, "Recover the golden gun"),
    (19, 1, "Defeat Baron Samedi"),
]

# Minimum difficulty for an objective: 0 Agent, 1 Secret Agent, 2 00 Agent.
# Missing entries are available from Agent.
OBJECTIVE_RANK = {
    (0, "Neutralize all alarms"): 1,
    (0, "Install covert modem"): 2,
    (0, "Intercept data backup"): 2,
    (1, "Contact double agent"): 1,
    (2, "Destroy heavy gun emplacements"): 2,
    (2, "Destroy missile battery"): 1,
    (3, "Obtain safe key"): 1,
    (3, "Steal building plans"): 1,
    (4, "Disrupt all surveillance equipment"): 1,
    (4, "Get personnel to activate computer"): 2,
    (4, "Download data from computer"): 2,
    (5, "Plant bombs in fuel rooms"): 2,
    (5, "Obtain telemetric data"): 1,
    (5, "Retrieve satellite circuitry"): 1,
    (6, "Disarm bridge bomb"): 1,
    (6, "Disarm engine room bomb"): 1,
    (7, "Disrupt all surveillance equipment"): 2,
    (7, "Disable Spetznaz support aircraft"): 1,
    (8, "Compare staff / casualty lists"): 1,
    (8, "Disable all security cameras"): 1,
    (8, "Recover Goldeneye operations manual"): 2,
    (10, "Recover helicopter black box"): 1,
    (11, "Contact Valentin"): 1,
    (12, "Destroy illegal arms cache"): 2,
    (12, "Destroy computer network"): 1,
    (12, "Obtain safe key"): 1,
    (12, "Recover helicopter blueprints"): 1,
    (13, "Locate Janus secret base"): 1,
    (13, "Crack Boris' password"): 2,
    (14, "Blow up ammo dump"): 1,
    (16, "Destroy inlet pump controls"): 1,
    (16, "Destroy outlet pump controls"): 1,
    (16, "Destroy master control console"): 1,
    (16, "Use radio to contact Jack Wade"): 2,
}


def item_name_to_id():
    ids = {dossier_name(i): dossier_id(i) for i in range(len(MISSIONS))}
    ids.update({
        dossier_diff_name(mission, diff): dossier_diff_id(mission, diff)
        for mission in range(len(MISSIONS))
        for diff in range(len(DIFFICULTIES))
    })
    ids.update({WEAPONS[i]: weapon_id(i) for i in range(len(WEAPONS))})
    ids["Body Armor"] = ITEM_ARMOR
    ids["Ammo Cache"] = ITEM_AMMO
    ids["Ammo Clip"] = ITEM_CLIP
    ids["Camera Alert"] = ITEM_ALERT
    ids["Jammed Clip"] = ITEM_JAM
    ids["Slipped on the Ice"] = ITEM_SLIP
    ids["Baron Samedi"] = ITEM_BARON
    ids["Greasy Hands"] = ITEM_GREASY
    ids["Restart Mission"] = ITEM_RESTART
    ids["Tiny Bond"] = ITEM_TINY
    ids["The Living Daylights"] = ITEM_FLAG
    ids["I am invisible"] = ITEM_BORIS
    ids["DK Mode"] = ITEM_DK
    ids["Paintball"] = ITEM_PAINT
    ids["Single Bullet"] = ITEM_BULLET
    ids.update({name: item_id for name, item_id in TOOLS})
    return ids


def objective_rows(diff_count, mission_count):
    """One row per (mission, difficulty, slot) the player can actually clear.

    diff_count 1 is agent-only: a single check, stored in the spare difficulty
    band, completed on whichever difficulty they played.
    """
    per_diff = diff_count > 1
    rows = []
    for mission, slot, text in OBJECTIVES:
        if mission >= mission_count:
            continue
        if not per_diff:
            rows.append((mission, 3, slot, objective_location_name(mission, text)))
            continue
        minimum = OBJECTIVE_RANK.get((mission, text), 0)
        for diff in range(minimum, diff_count):
            rows.append((mission, diff, slot, objective_location_name(mission, text, diff)))
    return rows


def objective_rows_for(diffs, mission_count):
    """Objectives that exist on one of these difficulties."""
    rows = []
    for mission, slot, text in OBJECTIVES:
        if mission >= mission_count:
            continue
        minimum = OBJECTIVE_RANK.get((mission, text), 0)
        for diff in diffs:
            if diff >= minimum:
                rows.append((mission, diff, slot, objective_location_name(mission, text, diff)))
    return rows


def location_name_to_id():
    ids = {
        location_name(mission, difficulty): location_id(mission, difficulty)
        for mission in range(len(MISSIONS))
        for difficulty in range(len(DIFFICULTIES))
    }
    for mission, diff, slot, name in objective_rows(len(DIFFICULTIES), len(MISSIONS)):
        ids[name] = objective_location_id(mission, diff, slot)
    for mission, diff, slot, name in objective_rows(1, len(MISSIONS)):
        ids[name] = objective_location_id(mission, diff, slot)
    for _mission, index, name in cheat_rows(len(MISSIONS)):
        ids[name] = cheat_location_id(index)
    return ids


def item_kind(name):
    if " Dossier" in name or name in {tool for tool, _id in TOOLS}:
        return "progression"
    if name in WEAPONS:
        return "useful"
    # Spare supplies the player spends on purpose. Ammo Clip stays filler:
    # it only tops up the gun already in hand.
    if name in ("Body Armor", "Ammo Cache"):
        return "useful"
    if name in TRAPS:
        return "trap"
    return "filler"


def tool_rules(diffs, mission_count, objective_checks):
    """Locations that need a tool when the seed does not hand mission kit out.

    A mission clear needs every tool its objectives on that difficulty need.
    Timed Mine is the existing weapon item. The other names are TOOLS.
    `diffs` is every difficulty a location in this seed can ask for.
    """
    diffs = tuple(diffs)
    reqs = {}

    def add(name, items):
        have = list(reqs.get(name, ()))
        for item in items:
            if item not in have:
                have.append(item)
        reqs[name] = tuple(have)

    def clear(mission, diff, items):
        if mission < mission_count and diff in diffs:
            add(location_name(mission, diff), items)

    def objective(mission, text, items):
        if not objective_checks or mission >= mission_count:
            return
        minimum = OBJECTIVE_RANK.get((mission, text), 0)
        for diff in diffs:
            if diff >= minimum:
                add(objective_location_name(mission, text, diff), items)

    def every_clear(mission, items):
        for diff in diffs:
            clear(mission, diff, items)

    def every_objective(mission, items):
        for mission_i, _slot, text in OBJECTIVES:
            if mission_i == mission:
                objective(mission, text, items)

    key, camera, thief = "Key Analyser", "Camera", "Datathief"
    objective(4, "Copy Goldeneye key and leave original", (key,))
    objective(4, "Photograph main video screen", (camera,))
    objective(4, "Download data from computer", (thief,))
    clear(4, 0, (key, camera))
    clear(4, 1, (key, camera))
    clear(4, 2, (key, camera, thief))

    plastique = "Plastique"
    objective(5, "Photograph satellite", (camera,))
    objective(5, "Plant bombs in fuel rooms", (plastique,))
    clear(5, 0, (camera,))
    clear(5, 1, (camera,))
    clear(5, 2, (camera, plastique))

    bug, defuser = "Tracker Bug", "Bomb Defuser"
    objective(6, "Plant tracking bug on helicopter", (bug,))
    objective(6, "Disarm bridge bomb", (defuser,))
    objective(6, "Disarm engine room bomb", (defuser,))
    clear(6, 0, (bug,))
    clear(6, 1, (bug, defuser))
    clear(6, 2, (bug, defuser))

    magnet = "Watch Magnet"
    every_clear(8, (magnet,))
    every_objective(8, (magnet,))
    objective(10, "Recover helicopter black box", (magnet,))
    clear(10, 1, (magnet,))
    clear(10, 2, (magnet,))

    laser = "Watch Laser"
    every_clear(13, (laser,))
    every_objective(13, (laser,))

    mines = ("Timed Mine",)
    objective(16, "Destroy inlet pump controls", mines)
    objective(16, "Destroy outlet pump controls", mines)
    objective(16, "Destroy master control console", mines)
    clear(16, 1, mines)
    clear(16, 2, mines)

    floppy = ("Explosive Floppy",)
    every_clear(18, floppy)
    every_objective(18, floppy)
    return reqs


def with_golden_gun(tool_map, diffs, mission_count):
    """Egyptian stays locked until the Golden Gun arrives.

    Baron Samedi only dies to that gun. Recovering the one in the temple does
    not require already owning it, and does not put it in a shared inventory.
    """
    if mission_count <= 19:
        return tool_map
    tool_map = dict(tool_map or {})

    def extend(name):
        have = list(tool_map.get(name, ()))
        if "Golden Gun" not in have:
            have.append("Golden Gun")
        tool_map[name] = tuple(have)

    for diff in diffs:
        extend(location_name(19, diff))
        extend(objective_location_name(19, "Defeat Baron Samedi", diff))
    return tool_map


Loc = namedtuple("Loc", "id mission all_of any_of need_gun")
# Existing call sites pass four fields. need_gun is only set for a shared
# weapon inventory, where a clear is not real without a firearm.
Loc.__new__.__defaults__ = (False,)


def _satisfied(all_of, any_of, owned, need_gun=False):
    if need_gun and not any(name in owned for name in FIREARMS):
        return False
    if not set(all_of) <= owned:
        return False
    if not any_of:
        return True
    return any(set(group) <= owned for group in any_of)


def build_locations(mission_count, diffs, split, objective_checks, cheat_checks, tool_map, need_firearm=False):
    """Every location and the items required to reach it.

    `tool_map` is empty when missions still start with their tools.
    When dossiers are split, a difficulty stays locked until its own dossier
    arrives. Otherwise one dossier opens every difficulty.
    """
    diffs = tuple(diffs)
    tool_map = tool_map or {}
    locs = {}

    def dossier_for(mission, diff):
        if split:
            return dossier_diff_name(mission, diff)
        return dossier_name(mission)

    def add(name, loc_id, mission, all_of, any_of=()):
        locs[name] = Loc(
            loc_id, mission, frozenset(all_of), tuple(frozenset(g) for g in any_of), need_firearm
        )

    for mission in range(mission_count):
        for diff in diffs:
            name = location_name(mission, diff)
            tools = tool_map.get(name, ())
            add(name, location_id(mission, diff), mission, (dossier_for(mission, diff), *tools))

    if objective_checks:
        for mission, diff, slot, name in objective_rows_for(diffs, mission_count):
            tools = tool_map.get(name, ())
            add(name, objective_location_id(mission, diff, slot), mission, (dossier_for(mission, diff), *tools))

    if cheat_checks:
        for mission, index, name in cheat_rows(mission_count, diffs, split):
            if not split:
                tools = ()
                if index < 20:
                    tools = tool_map.get(location_name(mission, CHEAT_DIFF[index]), ())
                elif index == 21:
                    tools = tool_map.get(location_name(mission, 1), ())
                elif index == 22:
                    tools = tool_map.get(location_name(mission, 2), ())
                add(name, cheat_location_id(index), mission, (dossier_name(mission), *tools))
                continue
            if index < 20:
                diff = CHEAT_DIFF[index]
                tools = tool_map.get(location_name(mission, diff), ())
                add(name, cheat_location_id(index), mission, (dossier_diff_name(mission, diff), *tools))
            elif index == 20:
                groups = [(dossier_diff_name(mission, diff),) for diff in diffs]
                add(name, cheat_location_id(index), mission, (), groups)
            elif index == 21:
                groups = []
                for diff in diffs:
                    if diff in (1, 2):
                        tools = tool_map.get(location_name(mission, diff), ())
                        groups.append((dossier_diff_name(mission, diff), *tools))
                add(name, cheat_location_id(index), mission, (), groups)
            elif index == 22:
                tools = tool_map.get(location_name(mission, 2), ())
                add(name, cheat_location_id(index), mission, (dossier_diff_name(mission, 2), *tools))
    return locs


def starting_items(starting, diffs, split):
    items = []
    for mission in starting:
        if split:
            items.extend(dossier_diff_name(mission, diff) for diff in diffs)
        else:
            items.append(dossier_name(mission))
    return items


def ensure_open_start(starting, diffs, tool_map):
    """Keep one mission whose clear does not need a tool.

    A start that is only tool-gated missions has nowhere to place those tools,
    so the seed cannot be finished. Dam is always open. The replacement stays
    inside the campaign and keeps the same count.
    """
    starting = list(starting)
    if not tool_map:
        return starting

    def open_mission(mission):
        return any(not tool_map.get(location_name(mission, diff)) for diff in diffs)

    if any(open_mission(mission) for mission in starting):
        return starting
    for mission in range(CAMPAIGN_COUNT):
        if mission not in starting and open_mission(mission):
            starting[0] = mission
            return starting
    return starting


def _missing(loc, owned):
    """Smallest set of items still needed before this location can be reached."""
    if not loc.any_of:
        return loc.all_of - owned
    best = None
    for group in loc.any_of:
        need = (loc.all_of | group) - owned
        if best is None or len(need) < len(best):
            best = need
    return best


def required_gear(locs):
    """Tools and weapons a location requires. Dossiers are not gear."""
    names = []
    seen = set()
    for loc in locs.values():
        for group in (loc.all_of, *loc.any_of):
            for name in sorted(group):
                if "Dossier" in name or name in seen:
                    continue
                seen.add(name)
                names.append(name)
    return names


def progression_pool(locs, start_items):
    """One of each item a location can require, except the ones already owned."""
    needed = set()
    for loc in locs.values():
        needed.update(loc.all_of)
        for group in loc.any_of:
            needed.update(group)
    owned = set(start_items)
    return sorted(needed - owned)


def early_fill(start_items, pool_items, locs):
    """Place every required item on a location that does not itself need it.

    A location that is already reachable does not require an item still in the
    pool, so it can hold any of them. Gates that do not spend the last free
    slot are opened first. A gate that costs more slots than it opens is tried
    only when nothing cheaper remains, hardest first, and another order is
    tried if that one runs out of room. If no order reaches every location,
    those settings cannot be finished.
    """
    obtained = set(start_items)
    unplaced = set(pool_items)
    used = set()
    failed = set()

    def reachable():
        return [
            name for name, loc in locs.items()
            if name not in used and _satisfied(loc.all_of, loc.any_of, obtained, loc.need_gun)
        ]

    def place(missing):
        chosen = []
        placed = []
        for item in sorted(missing):
            slots = sorted(reachable())
            if not slots:
                undo(placed, chosen)
                return None
            slot = slots[0]
            used.add(slot)
            unplaced.remove(item)
            obtained.add(item)
            chosen.append(slot)
            placed.append(item)
        return chosen

    def undo(missing, chosen):
        for item, slot in zip(sorted(missing), chosen):
            used.remove(slot)
            unplaced.add(item)
            obtained.remove(item)

    def search():
        if not unplaced:
            return all(_satisfied(loc.all_of, loc.any_of, obtained, loc.need_gun) for loc in locs.values())
        slots = reachable()
        if len(unplaced) <= len(slots):
            obtained.update(unplaced)
            unplaced.clear()
            return all(_satisfied(loc.all_of, loc.any_of, obtained, loc.need_gun) for loc in locs.values())
        if not slots:
            return False
        state = frozenset(obtained)
        if state in failed:
            return False
        groups = {}
        for name, loc in locs.items():
            if name in used or _satisfied(loc.all_of, loc.any_of, obtained, loc.need_gun):
                continue
            missing = _missing(loc, obtained)
            if not missing or len(missing) > len(slots) or not missing <= unplaced:
                continue
            groups.setdefault(frozenset(missing), []).append(name)
        if not groups:
            failed.add(state)
            return False

        def opened(missing):
            return sum(len(names) for other, names in groups.items() if other <= missing)

        def net(missing):
            return opened(missing) - len(missing)

        ordered = sorted(groups, key=lambda missing: (net(missing), len(missing)), reverse=True)
        growing = [missing for missing in ordered if net(missing) >= 0]
        # A gate that returns at least as many slots as it spends cannot be the
        # reason a later gate runs out of room, so one such choice is enough.
        choices = growing[:1] if growing else ordered
        for missing in choices:
            chosen = place(missing)
            if chosen is None:
                continue
            if search():
                return True
            undo(missing, chosen)
        failed.add(state)
        return False

    return search()
