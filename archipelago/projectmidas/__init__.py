"""
Project Midas world for Archipelago 0.6.

Mission clears are locations. A dossier unlocks that mission in the folder.
Difficulty dossiers can lock each difficulty on its own. Tools gate a mission
when Mission Tools is off.
"""

from BaseClasses import Item, ItemClassification, Location, Region, Tutorial
from Options import OptionError
from worlds.AutoWorld import WebWorld, World

from .data import (
    CAMPAIGN_COUNT,
    CHEATS,
    DIFFICULTIES,
    GOAL_AZTEC,
    GOAL_CAMPAIGN,
    GOAL_CRADLE,
    GOAL_EGYPTIAN,
    MISSIONS,
    WEAPONS,
    build_locations,
    check_diffs,
    dossier_diff_name,
    dossier_name,
    early_fill,
    ensure_open_start,
    FIREARMS,
    with_golden_gun,
    goal_diffs,
    item_kind,
    item_name_to_id,
    location_name,
    location_name_to_id,
    logic_diffs,
    progression_pool,
    required_gear,
    starting_items,
    tool_rules,
)
from .options import GEOptions

FILLER_SUPPLIES = ("Body Armor", "Ammo Cache", "Ammo Clip")

ITEM_CLASS = {
    "progression": ItemClassification.progression,
    "useful": ItemClassification.useful,
    "filler": ItemClassification.filler,
    "trap": ItemClassification.trap,
}


class GEItem(Item):
    game = "Project Midas - GE 007"


class GELocation(Location):
    game = "Project Midas - GE 007"


class GEWebWorld(WebWorld):
    theme = "stone"
    tutorials = [Tutorial(
        "Setup Guide",
        "Generate a seed and connect Project Midas to the room.",
        "English",
        "setup_en.md",
        "setup/en",
        ["Project Midas"],
    )]
    options_presets = {
        "Standard": {
            "goal": "cradle",
            "goal_difficulty": "agent",
            "difficulty_checks": "all",
            "starting_missions": 1,
            "include_bonus": True,
        },
        "Quick": {
            "goal": "cradle",
            "goal_difficulty": "agent",
            "difficulty_checks": "agent_only",
            "starting_missions": 3,
            "include_bonus": False,
        },
    }


class ProjectMidasWorld(World):
    game = "Project Midas - GE 007"
    web = GEWebWorld()
    options_dataclass = GEOptions
    options: GEOptions
    topology_present = True

    item_name_to_id = item_name_to_id()
    location_name_to_id = location_name_to_id()
    item_name_groups = {
        "Dossiers": {dossier_name(i) for i in range(len(MISSIONS))} | {
            dossier_diff_name(i, d) for i in range(len(MISSIONS)) for d in range(len(DIFFICULTIES))
        },
        "Weapons": set(WEAPONS),
    }
    location_name_groups = {
        **{
            DIFFICULTIES[d]: {location_name(m, d) for m in range(len(MISSIONS))}
            for d in range(len(DIFFICULTIES))
        },
        "Cheats": set(CHEATS),
    }

    def generate_early(self):
        goal = int(self.options.goal)
        bonus = bool(self.options.include_bonus) or goal in (GOAL_AZTEC, GOAL_EGYPTIAN)
        self.bonus = bonus
        self.mission_count = len(MISSIONS) if bonus else CAMPAIGN_COUNT
        self.check_diffs = check_diffs(int(self.options.difficulty_checks))
        try:
            self.goal_diffs = goal_diffs(int(self.options.goal_difficulty), self.check_diffs)
        except ValueError as exc:
            raise OptionError(str(exc)) from exc
        self.split = bool(self.options.difficulty_dossiers)
        gating = bool(self.options.global_weapons) and not bool(self.options.mission_tools)
        rule_diffs = logic_diffs(self.check_diffs, bool(self.options.cheat_checks), self.split)
        self.tool_map = tool_rules(
            rule_diffs, self.mission_count, bool(self.options.objective_checks)
        ) if gating else {}
        if bool(self.options.global_weapons):
            self.tool_map = with_golden_gun(self.tool_map, rule_diffs, self.mission_count)
        self.rules = build_locations(
            self.mission_count,
            self.check_diffs,
            self.split,
            bool(self.options.objective_checks),
            bool(self.options.cheat_checks),
            self.tool_map,
            # Loose guns stay on the floor, so a clear is only real once a
            # firearm has been handed over. Launchers and mines do not count.
            need_firearm=bool(self.options.global_weapons),
        )

    def create_item(self, name: str) -> GEItem:
        kind = item_kind(name)
        if name == "Golden Gun" and bool(self.options.global_weapons) and self.mission_count > 19:
            kind = "progression"
        return GEItem(name, ITEM_CLASS[kind], self.item_name_to_id[name], self.player)

    def create_regions(self):
        menu = Region("Menu", self.player, self.multiworld)
        self.multiworld.regions.append(menu)
        for mission in range(self.mission_count):
            region = Region(MISSIONS[mission], self.player, self.multiworld)
            locations = {
                name: loc.id for name, loc in self.rules.items() if loc.mission == mission
            }
            region.add_locations(locations, GELocation)
            self.multiworld.regions.append(region)
            if self.split:
                needed = tuple(dossier_diff_name(mission, diff) for diff in self.check_diffs)
                menu.connect(
                    region,
                    rule=lambda state, need=needed: any(state.has(name, self.player) for name in need),
                )
            else:
                dossier = dossier_name(mission)
                menu.connect(region, rule=lambda state, needed=dossier: state.has(needed, self.player))

    def create_items(self):
        start_n = min(int(self.options.starting_missions), CAMPAIGN_COUNT)
        starting = self.random.sample(range(CAMPAIGN_COUNT), start_n)
        starting = ensure_open_start(starting, self.check_diffs, self.tool_map)
        start_names = starting_items(starting, self.check_diffs, self.split)
        for name in start_names:
            self.multiworld.push_precollected(self.create_item(name))

        owned = set(starting)
        if self.split:
            dossier_pool = [
                dossier_diff_name(mission, diff)
                for mission in range(self.mission_count)
                for diff in self.check_diffs
                if mission not in owned
            ]
        else:
            dossier_pool = [
                dossier_name(mission)
                for mission in range(self.mission_count)
                if mission not in owned
            ]
        items = [self.create_item(name) for name in dossier_pool]
        rest = len(self.rules) - len(items)
        if rest < 0:
            raise OptionError(
                "Project Midas - GE 007 has more dossiers than locations. Difficulty "
                "Dossiers adds one dossier per check difficulty and makes the "
                "seed longer. Turn it off, or add locations with all difficulty "
                "checks, objective checks, cheat checks, or more starting missions."
            )

        tool_names = required_gear(self.rules)
        if len(tool_names) > rest:
            raise OptionError(
                "Mission Tools off is a difficulty setting: those tools join the "
                f"multiworld ({len(tool_names)} items), but this seed only has {rest} "
                "open slots. Turn Mission Tools on, or add locations with all "
                "difficulty checks, objective checks, cheat checks, difficulty "
                "dossiers off, or more starting missions."
            )

        catalog = list(WEAPONS)
        owned_logic = list(start_names)
        if self.options.global_weapons:
            # One held weapon, which may be a launcher. A second gun is not
            # given. When the roll is not a firearm, one firearm stays in the
            # pool and is marked early so the fill puts it on a sphere-1 check.
            start_pool = list(WEAPONS[:19])
            if self.multiworld.players < 2:
                # Solo has no other world's first checks to hold that gun.
                start_pool = [name for name in start_pool if name in FIREARMS]
            start_gun = self.random.choice(start_pool)
            self.multiworld.push_precollected(self.create_item(start_gun))
            owned_logic.append(start_gun)
            if start_gun in catalog:
                catalog.remove(start_gun)
            if start_gun in tool_names:
                tool_names = [kept for kept in tool_names if kept != start_gun]
            if start_gun not in FIREARMS:
                # Golden Gun stays a normal progression item for Egyptian.
                firearm = self.random.choice([name for name in FIREARMS if name != "Golden Gun"])
                early = self.multiworld.early_items.setdefault(self.player, {})
                early[firearm] = early.get(firearm, 0) + 1
                # Our own reachability check runs before the multiworld fill.
                # It treats the sphere-1 firearm as already found, without
                # putting it in the starting inventory.
                owned_logic.append(firearm)
        for name in list(tool_names):
            if name in catalog:
                catalog.remove(name)
        if not early_fill(owned_logic, progression_pool(self.rules, owned_logic), self.rules):
            # One difficulty and no extra checks leaves a single Egyptian location,
            # which cannot hold both its dossier and the Golden Gun. Starting with
            # the gun keeps that seed finishable.
            if (
                "Golden Gun" not in owned_logic
                and "Golden Gun" in progression_pool(self.rules, owned_logic)
            ):
                self.multiworld.push_precollected(self.create_item("Golden Gun"))
                owned_logic.append("Golden Gun")
                tool_names = [name for name in tool_names if name != "Golden Gun"]
                if "Golden Gun" in catalog:
                    catalog.remove("Golden Gun")
            if not early_fill(owned_logic, progression_pool(self.rules, owned_logic), self.rules):
                raise OptionError(
                    "Project Midas - GE 007 cannot order these items so every location stays "
                    "reachable. Turn Mission Tools on, turn Difficulty Dossiers off, "
                    "or add locations with more difficulty checks, objective checks, "
                    "or cheat checks. With a shared weapon inventory, Egyptian also "
                    "needs the Golden Gun before it can be cleared."
                )
        for _ in range(5):
            catalog.extend(FILLER_SUPPLIES)
        catalog += ["Camera Alert"] * 6
        catalog += ["Jammed Clip"] * 4
        catalog += ["Slipped on the Ice"] * 4
        catalog += ["Baron Samedi"] * 4
        catalog += ["Greasy Hands"] * 4
        catalog += ["Restart Mission"] * 4
        catalog += ["Tiny Bond"] * 4
        # Not placed. In solo the flag sits on Bond's body, which the player
        # does not see, so the trap only takes the gun away and reads as a bug.
        # The item id and the AP Debug page still fire it.
        # catalog += ["The Living Daylights"] * 2
        catalog += ["I am invisible"] * 2
        catalog += ["DK Mode"] * 2
        catalog += ["Paintball"] * 2
        catalog += ["Single Bullet"] * 2
        filler_n = rest - len(tool_names)
        pad = 0
        while len(catalog) < filler_n:
            catalog.append(FILLER_SUPPLIES[pad % 3])
            pad += 1
        items += [self.create_item(name) for name in tool_names]
        items += [self.create_item(name) for name in catalog[:filler_n]]

        if len(items) != len(self.rules):
            raise RuntimeError(f"Project Midas - GE 007 built {len(items)} items for {len(self.rules)} locations")
        self.multiworld.itempool += items

    def set_rules(self):
        for name, loc in self.rules.items():
            all_of = tuple(loc.all_of)
            any_of = tuple(tuple(group) for group in loc.any_of)
            if not all_of and not any_of and not loc.need_gun:
                continue
            guns = FIREARMS if loc.need_gun else ()
            self.get_location(name).access_rule = lambda state, need=all_of, alts=any_of, guns=guns: (
                all(state.has(item, self.player) for item in need)
                and (not alts or any(all(state.has(item, self.player) for item in group) for group in alts))
                and (not guns or any(state.has(gun, self.player) for gun in guns))
            )
        goal = int(self.options.goal)
        if goal == GOAL_CAMPAIGN:
            missions = range(CAMPAIGN_COUNT)
        else:
            missions = ({GOAL_CRADLE: 17, GOAL_AZTEC: 18, GOAL_EGYPTIAN: 19}[goal],)
        names = tuple(
            f"{MISSIONS[mission]} ({DIFFICULTIES[diff]})"
            for mission in missions
            for diff in self.goal_diffs
        )
        self.multiworld.completion_condition[self.player] = (
            lambda state, needed=names: all(state.can_reach(name, "Location", self.player) for name in needed)
        )

    def fill_slot_data(self):
        return {
            "goal": int(self.options.goal),
            "goal_difficulty": int(self.options.goal_difficulty),
            "difficulty_checks": int(self.options.difficulty_checks),
            "difficulty_dossiers": 1 if self.split else 0,
            "include_bonus": 1 if self.bonus else 0,
            "objective_checks": 1 if self.options.objective_checks else 0,
            "cheat_checks": 1 if self.options.cheat_checks else 0,
            "global_weapons": 1 if self.options.global_weapons else 0,
            "mission_tools": 1 if self.options.mission_tools else 0,
            "enemy_skins": int(self.options.enemy_skins),
            "enemy_heads": int(self.options.enemy_heads),
            "enemy_weapons": int(self.options.enemy_weapons),
            "allow_lethal_weapons": 1 if self.options.allow_lethal_weapons else 0,
            "enemy_accuracy": int(self.options.enemy_accuracy),
            "mission_music": 1 if self.options.mission_music else 0,
            "death_link": 1 if self.options.death_link else 0,
        }

    def get_filler_item_name(self) -> str:
        return self.random.choice(FILLER_SUPPLIES)
