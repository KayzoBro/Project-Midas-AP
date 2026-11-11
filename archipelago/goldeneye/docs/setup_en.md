# Project Midas - GE 007 Archipelago setup

Project Midas can join an Archipelago room. Clearing a solo mission sends that
check. A dossier in your inventory unlocks that mission in the folder. Weapons
you receive are added to Bond's loadout. A few items are armour, ammo, or traps.

This world is not part of the official Archipelago build, so a seed is generated
on your machine and hosted by a local server. The Windows game can also connect
with TLS to a server that already has the world, including archipelago.gg if it
is added there later.

## Install the world

Use Archipelago 0.6 or newer. Copy this folder:

`archipelago/goldeneye`

to:

`<Archipelago>/custom_worlds/goldeneye`

The copy must contain `__init__.py`.

## Player options

```yaml
name: Bond
game: Project Midas - GE 007
Project Midas - GE 007:
  goal: cradle
  goal_difficulty: agent
  difficulty_checks: all
  difficulty_dossiers: false
  starting_missions: 1
  include_bonus: true
  objective_checks: false
  cheat_checks: false
  global_weapons: false
  mission_tools: true
  enemy_skins: off
  enemy_heads: off
  enemy_weapons: off
  allow_lethal_weapons: false
  enemy_accuracy: off
  mission_music: false
  death_link: false
```

`objective_checks` sends a check when an objective is completed, in addition to
the mission clear. `cheat_checks` sends a check when a cheat is unlocked.
The target time is the cart's. A run with a cheat turned on does not count,
and 007 does not count. Aztec and Egyptian cheats follow `include_bonus`.

| Mission | Clear | Cheat |
| --- | --- | --- |
| Dam | Secret Agent 2:40 | Paintball Mode |
| Facility | 00 Agent 2:05 | Invincible |
| Runway | Agent 5:00 | DK Mode |
| Surface | Secret Agent 3:30 | 2x Grenade Launcher |
| Bunker | 00 Agent 4:00 | 2x Rocket Launcher |
| Silo | Agent 3:00 | Turbo Mode |
| Frigate | Secret Agent 4:30 | No Radar |
| Surface 2 | 00 Agent 4:15 | Tiny Bond |
| Bunker 2 | Agent 1:30 | 2x Throwing Knife |
| Statue | Secret Agent 3:15 | Fast Animation |
| Archives | 00 Agent 1:20 | Bond Invisible |
| Streets | Agent 1:45 | Enemy Rockets |
| Depot | Secret Agent 1:40 | Slow Animation |
| Train | 00 Agent 5:25 | Silver PP7 |
| Jungle | Agent 3:45 | 2x Hunting Knife |
| Control | Secret Agent 10:00 | Infinite Ammo |
| Caverns | 00 Agent 9:30 | 2x RC-P90 |
| Cradle | Agent 2:15 | Gold PP7 |
| Aztec | Secret Agent 9:00 | 2x Laser |
| Egyptian | 00 Agent 6:00 | All Guns |

Magnum is any Cradle clear. Laser is Aztec on Secret Agent or 00 Agent.
Golden Gun is Egyptian on 00 Agent.

`global_weapons` turns guns you find into ammunition. Weapons
from the multiworld stay in your inventory for every mission, and the seed
starts you with your fists and one random gun. Grenade launchers, rockets,
grenades and mines are not enough to finish a mission. If that random gun is
one of them, a firearm is placed on an early check in the multiworld instead
of being added to your start. You still begin with the explosive. A solo seed
starts on a firearm, because there is no other game to hide that gun in. Each mission then starts every
firearm at 100 rounds, the amount the cart gives the gun you begin with, or
at that type's maximum when it is lower. Grenades, rockets and mines start
with one magazine. A weapon you already own gets that same reserve even when
this option is off. Silver PP7 and Gold PP7 are not in that inventory.
A guard who drops one leaves the ammunition, not the gun. The Train and
Cradle cheats still grant them. Baron Samedi only dies to the Golden Gun. With this option
on, the gun in the temple does not stay in your inventory, so Egyptian cannot
be cleared until the Golden Gun item arrives. Recovering that gun is still
possible without owning it. A seed with only one check per mission and no
objective or cheat checks starts you with the Golden Gun, because there is
nowhere else to put it. `mission_tools` is a difficulty
setting. On, each mission still starts with its tools. Off, those tools are
items in the multiworld, and a mission that needs one is not required until
you have it. It only applies when `global_weapons` is on. A seed with very
few item slots cannot turn it off.

`goal` is `cradle`, `aztec`, `egyptian`, or `campaign`.
`goal_difficulty` is `agent`, `secret_agent`, `00_agent`, or `all`.
`all` means every difficulty that `difficulty_checks` includes. The goal has
to be one of those checks, or `all`. Generation stops if it is not, because
that clear would not exist. 007 mode counts as 00 Agent.
`difficulty_checks` is `all`, `agent_only`, `secret_agent`, or `00_agent`.
Only that difficulty sends the mission check and the objective check.
`agent_only` counts an Agent clear only. A clear on another difficulty does
not send it.
`difficulty_dossiers` is a hard setting, off by default. Off, one dossier
unlocks every difficulty of that mission. On, each check difficulty is its
own item: `Dam Dossier (Agent)`, `Dam Dossier (Secret Agent)`, `Dam Dossier
(00 Agent)`. That difficulty stays locked until its dossier arrives. This
adds progression items and makes the seed longer. A seed with too few
locations cannot turn it on. Starting missions include one dossier for each
check difficulty. With it on, a cheat whose target difficulty is not a check
is left out, because that difficulty cannot be played.
`starting_missions` is how many campaign missions start unlocked. The seed
picks them at random, so the first dossier is not always Dam. If every one
of them needs a tool that is still in the pool, one is swapped for a mission
that can be cleared without it.
`include_bonus` adds Aztec and Egyptian.

`enemy_skins` and `enemy_heads` are `off`, `grouped`, or `chaotic`. Grouped puts one combat uniform on the mission, and one male head plus one female head. Chaotic rolls each enemy, including guards who are not in the level and show up while you play. Scientists, civilians, and anyone with a named head stay as they are on the cart.
`enemy_weapons` is `off`, `grouped`, or `chaotic`. Grouped hands every enemy
in the mission the same weapon. Chaotic rolls each enemy the same way, including those reinforcements. The pool is the
guns, grenades, and launchers a generic enemy can already hold. Knives and
mines are left out, because a guard does not know what to do with them and
just runs. Remote mines would also spoil the Surface 2 helicopter objective.
`allow_lethal_weapons` only matters when `enemy_weapons` is not off. Off, the
mix leaves out the Golden Gun, the Silver PP7 and the Gold PP7. On, those
three are in the mix. One shot is enough. Generate a new seed after turning
it on. A seed that does not have the option leaves it off.
`enemy_accuracy` uses the same three values and replaces how well those enemies
shoot, from almost never hitting to very lethal. Both weapon and accuracy rolls
are hard: a seed can be unfair, and some seeds will be unreasonable.
`mission_music` swaps the mission's main track. Action stingers stay.
`death_link` is off unless you turn it on. Bond's death sends a death link,
and so does Natalya's. Receiving one kills Bond. Generate a new seed after
turning it on. A seed that does not have the option leaves it off.

Body Armor, Ammo Cache and Ammo Clip fill the spare slots in equal numbers.
Camera Alert sounds the alarm and tells the living guards where Bond is.
Jammed Clip empties the magazine in each hand for ten seconds. The reserve
stays, and the gun reloads from it once the jam ends. A line stays on screen
with the time left. Slipped on the Ice
shoves Bond sideways the way a bullet hit does. The damage flash plays.
Health and armor stay where they were.
Baron Samedi laughs, then chases Bond for the rest of the mission with the
ZMG. He cannot be killed. Greasy Hands throws the gun in hand forward, the
way a grenade is thrown, and switches to fists. Mission tools stay in the watch. Pick the gun up to have it back. The
next mission gives an owned gun back even if it was left on the floor.
Restart Mission reloads the stage from the beginning. A teleport back to the
start pad could finish Frigate immediately, or leave the Train ending behind
you, so the trap starts the mission over instead.
Tiny Bond lasts sixty seconds. It is the cheat, turned on during the mission:
Bond and the camera shrink straight away, and grow back when the time is up.
If Tiny Bond was already on, it stays on.
I am invisible drops Boris in front of you. He does not fight.
DK Mode lasts one minute. Paintball lasts three minutes. Single Bullet adds
one round to the reserve of the gun you are holding. The magazine stays as it is.
A second copy of a timed trap adds its time onto what is left. The countdown
stays on screen even if receipt toasts are hidden.
When someone sends an item to another game, the toast uses that game's item
name, and the other player's location name, once the room's data package
has arrived.
Death Link can be turned on or off from the Archipelago screen, without
generating a new seed. The choice is remembered.
With a shared weapon inventory, the Golden Gun in the Egyptian temple can
still be picked up for Recover the golden gun, even when you do not own the
gun and even when that ammo is already full. Control lends you remote mines
for that mission if you have not found Remote Mine yet.

Generate the multiworld from the Archipelago launcher, then host it. The default
local port is 38281, without TLS.

## Connect the game

In `midas.ini`, next to the executable:

```ini
[archipelago]
enabled=1
host=127.0.0.1
port=38281
slot=Bond
password=
tls=0
```

`slot` is the player name from the yaml, not the game title. `tls=1` and
`host=archipelago.gg` are for a TLS server. A local host uses `tls=0`.
With Discord running, the client shows the mission or the folder. The
application id is part of the game. It is public and cannot be changed
from this file.
`popup=0` hides the receipt toasts. A toast for an item shows the Archipelago mark: gold when it is something you need or a trap, the coloured mark for a useful find such as a gun, and grey for spare supplies. `popup_pos` is `top_right`, `top_left`,
`bottom_right`, or `bottom_left`. The default corner is `bottom_left`. `armor_key` and `ammo_key` are the keys that
use a stored Body Armor and Ammo Cache (default `F5` and `F6`). An Ammo Cache
adds 50 rounds of every ammunition type, or that type's maximum when it is
lower. Using one
plays the cart's armour or ammunition pickup sound. An Ammo Clip is filler.
Receiving one adds one magazine of the weapon in hand, and plays the
ammunition pickup sound. Fists grant nothing. The same rows
are in the files screen under Options, Gameplay, Archipelago. On those two key
rows, confirm and press the new key. Escape cancels. Delete clears the key.

The same settings can be typed in the tilde console: `ap slot Bond`,
`ap host 127.0.0.1`, `ap tls 0`, `ap connect`. `ap status` shows the session.
`ap help` lists the commands.

Each seed keeps its own four folders, under `Data/Saves/ap`. A seed you have
not played starts empty, so a previous seed's times and cheat unlocks do not
come with it. The seed you were already playing keeps the folders from
`eeprom.bin`. With Archipelago off, the game still uses that file.

The folder has a Survival row. It opens a short list: Facility, Bunker,
Archives, Caverns, Egyptian, Statue and Cradle. The map you pick is played
alone, and guards arrive in waves. They use the solo routes, so they can
cross the map. Nothing you do there is a check. With no firearm, the run
lends a PP7. The console command is `ap survival`.

Until the game is connected, campaign missions stay locked. A mission opens
once its dossier arrives. On Select Mission, each photo shows A, SA and 00.
Red is locked, orange is still to play, and green is finished. A difficulty
this seed does not check stays grey. The four save folders share that inventory, so each
of them offers the same missions. Clearing a mission with a cheat active does
not send a check.

007 mode counts as an 00 Agent clear. Aztec and Egyptian are skipped when the
seed turned bonus missions off.
