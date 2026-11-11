# Changelog

## [Unreleased]

### Removed

- The Living Daylights is not placed in seeds. In solo the flag is attached to Bond's body, which is not drawn, so the trap only removes the gun. The item and the AP Debug page still fire it.

### Changed

- The Survival row is hidden on the folder. The mode is still in the build, and `ap survival` still opens it.
- Timed traps pause when a mission ends and resume on the next one. The menus do not spend the remaining time. A new seed still clears them.
- The Archipelago connection screen gives the title, the seed line and each field its own row. The seed used to be drawn through the host name. A long seed keeps its last eight digits.

### Fixed

- Survival uses the multiplayer pause menu. The solo watch asked for a text bank the arena does not load, and leaving through it opened the mission-failed screen with a null briefing name.
- Survival places the multiplayer weapon set. The arena pads and the guns you start with were left as raw slot numbers, so the watch had no name for them and they could not be used.
- Leaving Survival returns to the folder. It was opening the mission-failed screen, and an arena has no briefing, so that screen was Dam on Agent.
- Survival shows the wave and how many guards are still up. Guards spawn alert and are sent running until they are close, which is the path that opens doors. A dead guard stays through the fall before the body is removed.
- Survival offers Facility, Bunker, Archives, Caverns, Egyptian, Statue and Cradle. Those arenas keep the solo pads, so the solo path graph is used and the guards can route. The other multiplayer arenas stay on the normal grid.
- An Ammo Cache adds 50 rounds of each ammunition type. It was one magazine of the first weapon that uses that type, so 9mm gained a PP7 magazine and rifle rounds a KF7 magazine. A type whose maximum is lower than 50 stops there.
- I am invisible spawns Boris with empty hands when Random Enemy Weapons is on. The roll arms a character while he is being created, and a freed slot still carried the previous guard's mark.

### Added

- Survival, from the folder row of that name or `ap survival`. The map is chosen on the multiplayer arena grid. The run is one player on that arena's setup, and guards are spawned in waves. A run with no firearm is lent a PP7 for that stage. Clears, objectives, cheat checks and Death Link are not sent. Retry on the failed screen starts it again. A kneel shot whose heading fell outside one full turn no longer reads off the firing-animation table.
- Cart folders are stored per Archipelago seed in `Data/Saves/ap/<seed>.bin`. A new seed starts from empty folders. The seed already recorded in `archipelago.txt` keeps the folders that were in `eeprom.bin`. Archipelago off still uses `eeprom.bin`.
- Discord rich presence (`port/src/ap_discord.c`). The application id is compiled in and is not read from `midas.ini`. The pipe is non-blocking. The text is the mission and difficulty, or the folder and the slot.
- With a shared weapon inventory, a clear is not reachable until the player has a firearm. Grenade launchers, rockets, grenades and mines do not count. If the starting weapon is one of those, one firearm is marked early so the fill places it in sphere 1. It is not added to the starting inventory. A solo seed rolls a firearm as that single start, because it has no other sphere-1 check. Body Armor and Ammo Cache are useful items. Ammo Clip stays filler. Generate a new seed for the classification. An old seed still shows those two as useful in this client.
- Traps Restart Mission (`640306`) and Tiny Bond (`640307`, 60 seconds). Restart reloads the stage through `bossSetLoadedStage`. A teleport to the start pad could clear Frigate immediately or leave the Train ending behind Bond. Tiny Bond uses `cheatButtonHandleCheatsTurnedOn`, so the model and the camera height shrink during the mission and the cheat's own off path restores them. A second copy of a timed trap extends the clock. Timed traps keep a countdown line up until the effect ends, even when receipt toasts are off. The Living Daylights (`640308`) stays on the AP Debug page and is not added to the pool. Generate a new seed.
- Traps I am invisible (`640309`), DK Mode (`640310`, 60 seconds), Paintball (`640311`, 3 minutes) and Single Bullet (`640312`). Boris is invincible, unarmed, and his sight is zero so the standard guard list idles. DK Mode and Paintball use the cheat on/off path and do not turn a cheat off that was already on. Single Bullet adds one round to the reserve of the current firearm. The magazine is left alone. Generate a new seed.
- Outgoing item lines use the other game's item and location names from `DataPackage`, requested per game after connect. Death Link can be toggled on the Archipelago screen; `midas.ini` `death_link` is `seed`, `on`, or `off`, and `on`/`off` send `ConnectUpdate` immediately.
- Global weapons no longer fills Golden Gun ammo unless that gun is owned. The shrine gun can still be collected when the reserve is already full, and the prop is kept so Recover the golden gun can complete without granting the shared weapon. Control's mission kit lends remote mines and the detonator for that mission only.
- `allow_lethal_weapons` in the world yaml, off by default. It only applies when random enemy weapons are on. Off, the mix leaves out the Golden Gun, the Silver PP7 and the Gold PP7. On, those three are in the mix. One shot is enough. Generate a new seed. An old seed leaves it off. Random enemy weapons off still uses the cart's guns.
- Chaotic enemy rolls use the guard's own slot, not the setup id. Guards who share a number, and guards who appear during the mission, no longer copy one weapon, one uniform, or one face. `enemy_skins` and `enemy_heads` are `off`, `grouped`, or `chaotic`. An old seed that turned skins or heads on stays grouped. Generate a new seed to pick chaotic for those two. A seed that already says chaotic weapons keeps that setting.
- Mission select photos show A, SA and 00. Red is locked, orange is still to play, green is cleared. A difficulty outside `difficulty_checks` stays grey (`apMissionMark`).
- Receipt toasts draw the Archipelago flower (`assets/archipelago`, baked by `scripts/port/make_ap_icons.py`). Progression and traps use the gold mark, useful items the colour mark, and filler the grey mark. The mark follows the server's item flags, and the local item id when a packet has none.
- Traps Baron Samedi (`640304`) and Greasy Hands (`640305`). Baron laughs, then stays for the rest of the mission with the ZMG and cannot be killed. If the stage has fewer than three free character slots, anonymous guards are removed until he fits. Two receipts spawn two. Greasy Hands drops the gun in each hand, switches to fists, and takes that gun out of the watch cycle, including a dual entry. Mission tools stay. The gun is still owned, so picking it up returns it, and the next mission's loadout gives it back. Either trap received in the menu waits until the mission is underway. Generate a new seed.
- CMake option `GE_AP_DEBUG` (default off). The folder gains an AP Debug page, and the tilde console accepts `ap debug`, both of which fire the traps, the supplies, an ammo clip, and a death link. A build without the option has neither.

- Death Link (`death_link` in the world yaml, off by default). Bond's death sends one. Natalya's death sends one too. A death link from someone else kills Bond, including one that arrived in the menu once the mission is underway. Bond does not send a second one for a death that a link caused. Generate a new seed and turn the option on. An old seed stays off.
- Using a stored Body Armor or Ammo Cache plays the cart's pickup sound.
- Ammo Clip fills spare locations. Receiving one adds one magazine of the weapon in hand and plays the ammunition pickup sound. Fists grant nothing. A clip received outside a mission waits until the next mission is underway.
- `difficulty_dossiers` in the world yaml. Off, one dossier unlocks every difficulty of that mission. On, each difficulty that sends a check is its own item (`Dam Dossier (Agent)`, `Dam Dossier (Secret Agent)`, `Dam Dossier (00 Agent)`), and that difficulty stays locked until it arrives. The option says it makes the seed longer. A cheat whose target difficulty is not a check is left out, because that difficulty cannot be played. Generation rejects a combination that cannot be finished.
- `difficulty_checks` accepts `secret_agent` and `00_agent` as well as `all` and `agent_only`. Only that difficulty sends the mission check and the objective check.
- `goal_difficulty` accepts `all`, which means every difficulty that `difficulty_checks` includes. A goal difficulty that is not one of those checks is rejected, because that clear does not exist. 007 mode counts as 00 Agent.
- Cheat checks (`cheat_checks` in the world yaml). Beating a mission's cheat target time sends that cheat. Magnum, Laser and Golden Gun follow the cart: any Cradle clear, Aztec on Secret Agent or 00 Agent, and Egyptian on 00 Agent. A run with a cheat turned on does not count, and 007 does not count.
- Enemy rolls in the world yaml. `enemy_skins` puts one combat uniform on every enemy in a mission. `enemy_heads` gives that mission one male head and one female head. `mission_music` swaps the mission's main track. `enemy_weapons` is `off`, `grouped`, or `chaotic`: grouped shares one weapon across the mission, chaotic rolls each enemy, from the guns, blades, grenades, mines, and launchers a generic enemy can already hold. `enemy_accuracy` uses the same three values and replaces their aim, from almost never hitting to very lethal. Weapons and accuracy are hard settings: a seed can be unfair. Scientists, civilians, invincible setup characters, and anyone with a named head stay on the cart. The roll comes from the seed, so reloading the mission repeats it.
- Objective checks (`objective_checks` in the world yaml). Each objective is a separate location on every difficulty that `difficulty_checks` includes. An objective that does not exist on that difficulty is skipped.
- Global weapon inventory (`global_weapons`). The seed starts with fists and one random gun. Loose guns and guard drops give ammunition unless that weapon is already in the Archipelago inventory, which is kept across missions. `mission_tools` (on by default) still gives scripted tools such as the watch magnet. Turning it off puts those tools in the pool and gates the missions that need them. Timed Mine stays the existing weapon item.
- With a shared weapon inventory, each mission starts every firearm ammunition type at 100 rounds, the amount the cart gives the gun you begin with, or at that type's maximum when it is lower. Grenades, rockets and mines start with one magazine. Drawing the gun loads one magazine out of that reserve. Body Armor and Ammo Cache are stocked (`armor` / `ammo` in the slot file). During a mission they stay on screen, top left, with a count. F5 uses an armor and F6 an ammo cache (`armor_key` / `ammo_key` in `[archipelago]`, also the Body Armor and Ammo Cache rows under Options, Gameplay, Archipelago). An ammo cache grants 50 rounds of each ammunition type, or that type's maximum when it is lower. An armor is kept if Bond is already wearing one. The receipt toast keeps the usual name (`Received Body Armor`, `Received Ammo Cache`).

- Receipt toasts (`port/src/ap_popup.c`). `[archipelago] popup` turns them on, and `popup_pos` picks a corner. They show items this slot receives, and items it sends to someone else. The same rows are under Options, Gameplay, Archipelago.
- Archipelago client in the PC port (`port/src/ap_client.c`, `ap_net.c`, `ap_json.c`). It speaks the Archipelago 0.6 WebSocket protocol, locks the solo folder to received dossiers, and sends a location check when a mission is cleared.
- Archipelago login screen (`MENU_ARCHIPELAGO`, `port/src/ap_menu.c`). Solo menus and mission start stay on it until the server accepts the slot. The cart's Dam-first folder is no longer reachable.
- GoldenEye 007 world package in `archipelago/goldeneye` for local seed generation. Slot data (`goal`, `goal_difficulty`, `difficulty_checks`, `include_bonus`) is applied by the client.
- `midas.ini` `[archipelago]` settings and tilde-console `ap` commands. Windows connects with TLS through WinHTTP; other builds use plain `ws://`.

### Changed

- The ending cinematic no longer starts because the mission was Cradle. On a connected seed, Next on the mission-complete screen plays it only after the mission that just finished the goal. Cradle without the goal opens the next mission or mission select.
- Weapons are useful items. The Golden Gun stays progression when Egyptian is in the seed and the weapon inventory is shared, because that clear needs it. Generate a new seed. An old seed keeps the classes it was built with, except the in-game toast, which follows this rule.
- The Archipelago game name is `Project Midas - GE 007`. The yaml `game` line and the options heading must use that name. Copy the world again and generate a new seed. A room made as GoldenEye 007 will not accept this client.

- The armor and ammo counters say `Arm.` and `Amm.` The vest and magazine drawings are gone.
- Spare item slots are split evenly between Body Armor, Ammo Cache and Ammo Clip. Extra filler was all Body Armor, so a seed had a handful of caches and clips and a lot of armor. Generate a new seed. The current one keeps its pool.
- `difficulty_checks: agent_only` sends the check only for an Agent clear. A clear on Secret Agent or 00 Agent no longer counts as that location. Generate a new seed. An old seed that counted objectives once, on any difficulty, used different location ids.
- `starting_missions` gives that many random campaign dossiers. A new seed no longer always starts on Dam.
- Solo mission select opens on the first dossier in the inventory. All four folders share that inventory.
- Receipt toasts stay up for eight seconds of real time. A high frame cap no longer clears them early.
- Receipt toasts start in the bottom left corner.
- The folder menu after a save has an Archipelago page. It shows the seed's goal and how many mission, objective and goal checks are done. The login and that page leave the photograph, the OHMSS word and the stamps off, so the lines sit on the paper.
- The folder menu no longer offers GoldenEye's multiplayer.
- The Archipelago login can be used with the mouse. The crosshair picks a line, and a click focuses a field or runs Connect and Play.
- On a connected seed the watch inventory grows by 48 slots. The cart's 30 were filled by shared guns, so a tool such as the GoldenEye key could not be added.
- The Archipelago row in Options fits above the crosshair's lower limit, so the mouse can select it.

### Fixed

- The receipt toast flower sits on the same line as the words. It was a few pixels high in the bar.
- Enemy weapon rolls no longer hand out a knife or a mine. A guard has no behaviour for those and just runs. That roll moves to the next weapon in the table. Guns already rolled stay on the same index.
- Enemy weapon rolls no longer hand out the Silver PP7 or the Gold PP7. Either one kills in a single shot. That roll moves to the next weapon in the table. Guns already rolled stay on the same index.
- Placed guards kept the level's gun. Chaotic and grouped weapon rolls only reached guards who appeared later, because the setup hands the gun over by character id after the guard exists. That hand-over now uses the same roll.
- Baron Samedi stayed where he appeared. He was running a guard's pad script, which does not chase. He now uses the normal guard behaviour, so he runs to Bond and shoots once he sees him.
- Greasy Hands left the gun at Bond's feet. It is now thrown forward, the same way a mine leaves the hand.
- Jammed Clip no longer halves the ammunition reserve. It empties the magazine in each hand and keeps it empty for ten seconds. The reserve stays, and the gun reloads from it once the jam ends.
- Slipped on the Ice no longer takes health or armor. It uses the shove of a bullet hit, sideways, so Bond moves, and the damage flash still plays.

- Cradle - Magnum never sent its check. The folder already said Cradle was finished before the debrief compared it, so the change was invisible. A Cradle clear now sends it directly. Laser and Golden Gun use the same path. Connecting again also sends one that a finished Cradle, Aztec or Egyptian clear already earned.
- Next after the mission that finishes the goal plays the credits, the same way Next does on Cradle outside a seed. It also does that on the goal's last dossier once the goal is done, instead of opening a later unlocked mission.
- Enemy weapon rolls could hand out the Golden Gun. That roll now moves to the next weapon in the table, the Silver PP7. The other weapons in a seed keep their roll.
- The client could fall behind the server's item list (`item index 25 did not match local 24`) when several server messages arrived together. The socket kept three messages and threw the rest away, including a ReceivedItems. It now keeps sixteen, drops a text line before an item list, and asks for the full list again until that list arrives.
- With a shared weapon inventory, a Silver PP7 or Gold PP7 dropped by a guard was added to Bond's inventory. Those two are the Train and Cradle cheats, not weapons in the pool. The drop now leaves the ammunition. A cheat that is already unlocked still grants its gun.
- Egyptian could be placed before the Golden Gun. Baron Samedi only dies to that gun. With a shared weapon inventory, the gun in the temple does not stay, so the Golden Gun item is required before any Egyptian clear, before defeating Baron Samedi, and before the Egyptian cheats. Recovering the gun in the temple does not require already owning it. A seed with one check per mission and no objective or cheat checks starts with the Golden Gun, because that seed has no spare location for it. Generate a new seed. An inventory that is not shared can still pick the gun up in the level.
- Enemy weapon rolls could hand out a remote mine. A guard who threw one could stick it to the Surface 2 helicopter and spoil the aircraft objective. Remote mines are no longer rolled. The other weapons in a seed keep their roll.
- Camera Alert, Jammed Clip and Slipped on the Ice barely showed up in play. The alarm now sounds and living guards are told where Bond is. Scientists, civilians, Natalya and invincible setup characters are left out. A jammed clip empties the magazine in each hand for ten seconds and plays the dry-fire click. The reserve is left alone, and the gun reloads from it once the jam ends. A slip shoves Bond the way a bullet does, sideways, with the damage flash and no health or armor lost.
- Frigate did not start with the tracker bug while Mission Tools was on. That option now puts the stage's door tools back in the inventory, with one charge when the tool uses ammunition. Frigate starts with the tracker bug and the bomb defuser. Dying no longer hands those tools out when Mission Tools is off.
- A shared weapon inventory started each ammunition type with a single magazine. The equipped shotgun then read 5 | 5, and drawing another gun emptied its reserve. Firearms now start at 100 rounds, and dying restores that reserve with the weapons.
- A difficulty dossier arrived as `Received Item 640555`. The toast now uses its name, such as `Aztec Dossier (Agent)`. Connecting shows the starting dossier and the starting weapon, including when this slot was already saved.
- After a clear, Next followed the cart order (Dam then Facility). It now opens the next dossier that is actually unlocked.
- Connecting sent Archipelago status 30, which means the goal is complete. Playing is 20 and the goal is 30, so a new slot is no longer marked finished on join.
- Checks stored on disk are replaced by the room's list when the slot is accepted, so an old file cannot re-send a finished goal.
