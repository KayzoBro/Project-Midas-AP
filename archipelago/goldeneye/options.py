from dataclasses import dataclass

from Options import Choice, DefaultOnToggle, PerGameCommonOptions, Range, Toggle


class Goal(Choice):
    """What finishes your game. Cradle is the campaign finale. Aztec and Egyptian
    are the bonus missions. Campaign means every mission from Dam through Cradle
    on the goal difficulty."""

    display_name = "Goal"
    option_cradle = 0
    option_aztec = 1
    option_egyptian = 2
    option_campaign = 3
    default = 0


class GoalDifficulty(Choice):
    """Difficulty a mission must be cleared on before it counts as the goal.
    All means every difficulty that Difficulty Checks includes. The goal
    difficulty has to be one of those checks, or All. 007 mode counts as
    00 Agent."""

    display_name = "Goal Difficulty"
    option_agent = 0
    option_secret_agent = 1
    option_00_agent = 2
    option_all = 3
    default = 0


class DifficultyChecks(Choice):
    """Which clears send a check.

    All: Agent, Secret Agent and 00 Agent are each a check.
    Agent only, Secret Agent or 00 Agent: only that difficulty sends the check.
    007 mode counts as 00 Agent."""

    display_name = "Difficulty Checks"
    option_all = 0
    option_agent_only = 1
    option_secret_agent = 2
    option_00_agent = 3
    default = 0


class DifficultyDossiers(Toggle):
    """Hard. Off, one dossier unlocks every difficulty of that mission.

    On, each difficulty is its own dossier: Dam Dossier (Agent), Dam Dossier
    (Secret Agent), Dam Dossier (00 Agent). Only difficulties that send a
    check get a dossier, and that difficulty stays locked until it arrives.
    This adds more progression items and makes the seed longer. A seed with
    very few locations cannot turn it on."""

    display_name = "Difficulty Dossiers"


class StartingMissions(Range):
    """How many campaign missions are already in the folder. They are chosen
    at random, so the free start is not always Dam. Those dossiers are not
    in the item pool. Aztec and Egyptian are never part of this set."""

    display_name = "Starting Missions"
    range_start = 1
    range_end = 6
    default = 1


class IncludeBonus(DefaultOnToggle):
    """Aztec and Egyptian are checks, and their dossiers are in the pool.
    Turn this off for the 18 campaign missions only. A goal of Aztec or Egyptian
    turns it back on."""

    display_name = "Include Bonus Missions"


class ObjectiveChecks(Toggle):
    """Completing an objective sends that objective's check, as well as the
    mission clear. The tracker uses the objective's own text. The A/B/C letter
    on screen changes with the difficulty, so it is not the location name."""

    display_name = "Objective Checks"


class CheatChecks(Toggle):
    """Beating a mission's cheat target time sends that cheat's check.
    The time and the difficulty are the cart's: Dam on Secret Agent in 2:40
    is Paintball Mode, Facility on 00 Agent in 2:05 is Invincible, and so on.
    Magnum is any Cradle clear, Laser is Aztec on Secret Agent or 00 Agent,
    and Golden Gun is Egyptian on 00 Agent. A run with a cheat turned on does
    not count, and 007 does not count. Aztec and Egyptian cheats follow
    Include Bonus Missions."""

    display_name = "Cheat Checks"


class GlobalWeapons(Toggle):
    """Guns on the ground and guns dropped by guards give ammunition only.
    A weapon is added to your inventory when its item arrives, and you keep it
    for every mission. The seed starts you with your fists and one random gun.
    Each mission starts every firearm at 100 rounds, the amount the cart gives
    the gun you begin with, or at that type's maximum when it is lower.
    Grenades, rockets and mines start with one magazine. Baron Samedi only
    dies to the Golden Gun, and the one in the temple does not stay with you,
    so Egyptian cannot be cleared until that item arrives. Silver PP7 and
    Gold PP7 are the Train and Cradle cheats. A guard who drops one does
    not add it to your inventory."""

    display_name = "Global Weapon Inventory"


class EnemySkins(Choice):
    """Cosmetic. Off leaves the cart's uniforms. Grouped puts one combat
    uniform on every enemy in the mission. Chaotic rolls each enemy,
    including guards who are not placed in the level and show up later.
    Scientists, civilians, allies with a unique head, and named characters
    stay as they are on the cart."""

    display_name = "Random Enemy Skins"
    option_off = 0
    option_grouped = 1
    option_chaotic = 2
    default = 0


class EnemyHeads(Choice):
    """Cosmetic. Off leaves the cart's heads. Grouped uses one male head
    and one female head for the mission. Chaotic rolls each enemy,
    including guards who are not placed in the level and show up later.
    Named heads stay put."""

    display_name = "Random Enemy Heads"
    option_off = 0
    option_grouped = 1
    option_chaotic = 2
    default = 0


class EnemyWeapons(Choice):
    """Hard. Enemies can be given any weapon a generic enemy can
    already hold: guns, grenades, and launchers. Bond's gadgets, knives,
    and mines stay out. A guard does not know what to do with a knife or a
    mine, so he just runs. Remote mines stay out for a second reason: a guard
    who throws one can spoil the Surface 2 helicopter objective.

    Off leaves the cart's weapons. Grouped gives every enemy in
    the mission the same weapon. Chaotic rolls each enemy on their own,
    including guards who are not placed in the level and show up later.
    The Golden Gun, the Silver PP7 and the Gold PP7 stay out unless
    Allow Lethal Weapons is on. A roll can make a seed unfair, and some
    seeds will be unreasonable."""

    display_name = "Random Enemy Weapons"
    option_off = 0
    option_grouped = 1
    option_chaotic = 2
    default = 0


class AllowLethalWeapons(Toggle):
    """Hard. Only matters when Random Enemy Weapons is not Off.

    Off, the mix leaves out the Golden Gun, the Silver PP7 and the Gold PP7.
    On, those three join the mix. One shot is enough. Are you crazy enough?!

    Random Enemy Weapons Off still leaves every guard with the cart's gun."""

    display_name = "Allow Lethal Weapons"


class EnemyAccuracy(Choice):
    """Hard. Replaces the aim of enemies. The roll runs from
    almost never hitting to very lethal, so a mission can become trivial
    or unreasonable. Named characters keep the cart's accuracy.

    Off leaves the cart. Grouped uses one roll for the whole mission.
    Chaotic rolls each enemy, including guards who are not placed in the
    level and show up later."""

    display_name = "Random Enemy Accuracy"
    option_off = 0
    option_grouped = 1
    option_chaotic = 2
    default = 0


class DeathLink(Toggle):
    """When Bond dies, everyone with Death Link dies too. Natalya dying sends
    one as well. A death link you receive kills Bond. Off by default."""

    display_name = "Death Link"


class MissionMusic(Toggle):
    """Cosmetic. The mission's main music is another track from the cart,
    chosen by the seed. Action stingers stay with the mission."""

    display_name = "Random Mission Music"


class MissionTools(DefaultOnToggle):
    """Difficulty setting. Turning this off makes the seed harder.

    On: each mission still starts with the tools it needs, such as the watch
    magnet, the watch laser, the camera, the datathief, the key analyser,
    plastique, the bomb defuser, the tracker bug, and the mines a stage hands
    you at the door. Holdable guns still follow Global Weapon Inventory.

    Off: those tools are items in the multiworld instead of being given at the
    door. The logic will not require a mission that needs one until you have
    it. Timed mines use the Timed Mine weapon. This only applies when Global
    Weapon Inventory is on. A seed with very few item slots cannot use Off."""

    display_name = "Mission Tools"


@dataclass
class GEOptions(PerGameCommonOptions):
    goal: Goal
    goal_difficulty: GoalDifficulty
    difficulty_checks: DifficultyChecks
    difficulty_dossiers: DifficultyDossiers
    starting_missions: StartingMissions
    include_bonus: IncludeBonus
    objective_checks: ObjectiveChecks
    cheat_checks: CheatChecks
    global_weapons: GlobalWeapons
    mission_tools: MissionTools
    enemy_skins: EnemySkins
    enemy_heads: EnemyHeads
    enemy_weapons: EnemyWeapons
    allow_lethal_weapons: AllowLethalWeapons
    enemy_accuracy: EnemyAccuracy
    mission_music: MissionMusic
    death_link: DeathLink
