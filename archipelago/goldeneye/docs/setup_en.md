# Project Midas - GE 007 Archipelago setup

Project Midas can join an Archipelago room. A dossier unlocks a mission.
Clearing it sends that check. Weapons you receive stay in Bond's loadout.
Other items are armour, ammunition, or traps.

The world is not in the official Archipelago build yet. Generate the seed on
your machine and host it locally. Windows can also connect with TLS to a
server that already has the world.

Archipelago calls this game `Project Midas - GE 007`.

## Install the world

Use Archipelago 0.6 or newer. Copy `archipelago/goldeneye.apworld` into
`<Archipelago>/custom_worlds/`.

Or copy the folder `archipelago/goldeneye` to
`<Archipelago>/custom_worlds/goldeneye`. That copy must contain `__init__.py`.

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

`name` is the slot you type in the game.

| Option | What it does |
| --- | --- |
| `goal` | `cradle`, `aztec`, `egyptian`, or `campaign` |
| `goal_difficulty` | `agent`, `secret_agent`, `00_agent`, or `all` |
| `difficulty_checks` | Which difficulty sends the check: `all`, `agent_only`, `secret_agent`, or `00_agent` |
| `difficulty_dossiers` | Off, one dossier unlocks every difficulty. On, each checked difficulty is its own dossier |
| `starting_missions` | How many campaign missions start unlocked. The seed picks them |
| `include_bonus` | Adds Aztec and Egyptian |
| `objective_checks` | Each objective is also a check |
| `cheat_checks` | Beating a mission's target time sends that cheat |
| `global_weapons` | Guns you find only give ammunition. Received weapons stay for every mission |
| `mission_tools` | On, each mission still starts with its tools. Off, those tools are items. Needs `global_weapons` |
| `enemy_skins`, `enemy_heads`, `enemy_weapons`, `enemy_accuracy` | `off`, `grouped` (one roll for the mission), or `chaotic` (each enemy) |
| `allow_lethal_weapons` | Lets randomized enemies carry the Golden Gun, Silver PP7, or Gold PP7 |
| `mission_music` | Swaps the mission's main track |
| `death_link` | Bond's death, and Natalya's, is sent to other players who turned it on |

Generate the multiworld from the Archipelago launcher, then host it. A local
host uses port 38281 and no TLS. Change the options, then generate again.

## Connect the game

After the legal screen, fill host, port, and slot, then CONNECT. PLAY stays
locked until the seed accepts you. The same values can live in `midas.ini`,
next to the executable:

```ini
[archipelago]
enabled=1
host=127.0.0.1
port=38281
slot=Bond
password=
tls=0
```

`slot` is the player name from the yaml. A local host uses `tls=0`.
`tls=1` is for a TLS server such as archipelago.gg.

Or use the tilde console: `ap host 127.0.0.1`, `ap port 38281`,
`ap slot Bond`, `ap tls 0`, `ap connect`. `ap help` lists the commands.

`popup` and `popup_pos` control the item toasts. `armor_key` and `ammo_key`
use a stored Body Armor and Ammo Cache (F5 and F6). The same rows are under
Options, Gameplay, Archipelago.

Each seed keeps its own four folders, under `Data/Saves/ap`. A new seed
starts empty. With Archipelago off, the game still uses `Data/Saves/eeprom.bin`.

A mission opens once its dossier arrives. On Select Mission, red is locked,
orange is still to play, and green is finished. Clearing a mission with a
cheat already on does not send a check.

Survival is not on the folder in this version. `ap survival` still opens it.
Nothing you do there is a check.
