# Changelog

## [Unreleased]

### Removed

- The Living Daylights ne sort plus dans une seed. Le drapeau n’est pas visible quand on joue seul, et le piège ne faisait que retirer l’arme. La page de test peut encore le déclencher.

### Changed

- Survival is not on the folder for this version.
- A timed trap no longer ends when the mission does. It waits through the menus and continues on the next mission.
- The connection screen no longer stacks the seed on top of the host line. Each row has its own space.

### Fixed

- In Survival, Start opens the multiplayer pause menu. Leaving that way no longer crashes, and the weapon list no longer says the text is missing.
- Survival gives you the weapons of the multiplayer set, on the map and at the start. They were blank slots before, so none of them could be used.
- Leaving Survival returns to the folder. It was showing a failed Dam on Agent.
- Survival shows the wave and how many guards are left. Guards spawn already alert, so they run at you straight away and open doors on the way. A killed guard falls before he disappears.
- Survival offers Facility, Bunker, Archives, Caverns, Egyptian, Statue and Cradle. Guards can find their way around those maps. The other multiplayer maps stay where they were.
- An Ammo Cache adds 50 rounds of every ammunition type. It was one magazine, and the size depended on which gun the game looked at first. Grenades, rockets and mines still stop at the most you can carry.
- Boris from I am invisible no longer appears holding a gun when enemies have random weapons.

### Added

- The folder has a Survival row. You pick the map on the same screen as a multiplayer match. Guards then arrive in waves on that arena. It does not count toward the seed. If you have no gun, you are handed a PP7 for that run. Dying and choosing to retry starts another run on the same map.
- Each Archipelago seed has its own four save folders. A new seed starts empty. The seed you were already playing keeps your current folders. Turning Archipelago off leaves the normal save where it was.
- Discord can show what you are playing, as long as Discord is open. Nothing to set in the connection file.
- If your starting weapon is a launcher, grenades or mines, a normal gun is placed on an early check in the multiworld. You are not handed that gun at the start. A mission cannot be finished, in the logic, until a gun like that has arrived. Body Armor and Ammo Cache count as useful finds. Generate a new seed for that. The marks in the game already show them as useful.
- Restart Mission and Tiny Bond can show up in a seed. Restart Mission starts the mission over from the beginning. Tiny Bond shrinks Bond for one minute, then he grows back. A clock stays on screen for the timed traps until the time runs out. The AP Debug page can still fire The Living Daylights. Generate a new seed.
- A seed can also hand out I am invisible, DK Mode, Paintball and Single Bullet. Boris appears in front of you and does not fight. DK Mode lasts one minute, Paintball three minutes. Single Bullet adds one round to the reserve of the gun you are holding. Generate a new seed.
- When you send an item to another game, the message shows that item's name and the place it was found. Death Link can be switched on or off from the Archipelago screen, without making a new seed.
- You can pick up the Golden Gun in the Egyptian temple for Recover the golden gun even if you do not own that gun, and even if that ammunition is already full. Control gives you remote mines for that mission if you have not found them yet.
- A seed can allow the Golden Gun, the Silver PP7 and the Gold PP7 in the enemy weapon mix. It is off unless you turn it on, and it does nothing unless enemy weapons are already randomized. One shot is enough. Generate a new seed. An old seed leaves them out. With enemy weapons off, guards still carry what they carry in the cart.
- Chaotic enemies are rolled one by one. Guards who were copies of each other, and guards who show up during the mission, no longer all carry the same gun. Uniforms and faces can stay off, be one look for the whole mission, or be chaotic too. Generate a new seed to turn chaotic uniforms or faces on. A seed that already uses chaotic weapons does not need a new one.
- On Select Mission, each photo shows A, SA and 00. Red is locked, orange is still to play, and green is finished. A difficulty this seed does not check stays grey, so you can see what is left without opening the mission.
- Item toasts show the Archipelago mark. Gold means something you need, or a trap. The coloured mark is a useful find, such as a gun. Grey is spare supplies, such as armour or ammunition.
- A seed can now hand out Baron Samedi and Greasy Hands. Baron laughs, then follows you for the rest of the mission. He cannot be killed. Greasy Hands throws the gun you are holding and leaves you on your fists. Tools stay in the watch. Pick the gun up to have it back. The next mission gives it back even if you leave it. Generate a new seed.
- A test build adds an AP Debug page in the folder, for trying the traps and the other effects. The usual build does not have that page.

- Death Link can be turned on when you generate a seed. It is off unless you turn it on. When Bond dies, the other players who turned it on die too. Natalya dying does the same. When someone else sends one, Bond dies. If it arrives from the menu, he dies once the mission is underway. A death that came from a link does not send another. Generate a new seed. An old seed leaves it off.
- Using a stored Body Armor or Ammo Cache plays the same sound as picking one up in the mission.
- Ammo Clip is an extra item that fills the seed. When one arrives, the weapon in your hand gains one magazine, with the usual ammunition sound. Fists gain nothing. If it arrives from the menu, it waits until you are in a mission.
- A seed can split dossiers by difficulty. Off, one dossier unlocks Agent, Secret Agent and 00 Agent. On, each difficulty that sends a check is its own dossier, and that difficulty stays locked until it arrives. The option says this makes the seed longer. A cheat whose difficulty is not a check is left out. If the options cannot make a seed that can be finished, generation stops and says so.
- Difficulty checks can be Secret Agent only, or 00 Agent only, as well as every difficulty or Agent only. Only that difficulty sends the check.
- The goal difficulty can be All, which means every difficulty the seed checks. The goal has to be one of those checks, or All.
- You can turn on cheat checks when you generate a seed. Beating a mission under its cheat time sends that cheat. Magnum is any clear of Cradle, Laser is Aztec on Secret Agent or 00 Agent, and the Golden Gun is Egyptian on 00 Agent. Finishing with a cheat already on does not count.
- A seed can change the enemies. Skins give every one of them the same uniform for a mission. Heads give the mission one male face and one female face. Music swaps the track that plays under the mission. Weapons are off, shared by the whole mission, or different for each enemy. Accuracy works the same way and can make them harmless or lethal. Weapons and accuracy are the hard ones: a seed can be unfair. Named people, scientists, and civilians stay as they are in the cart.
- You can turn on objective checks when you generate a seed. Each objective is a check on every difficulty the seed includes. An objective the cart does not have on that difficulty is skipped.
- You can turn on a shared weapon inventory. Guns you find only give ammunition. Weapons you receive stay with you, and the seed starts you with your fists and one random gun. Mission tools stay on unless you turn that option off.
- Body Armor and Ammo Cache are stored instead of being used immediately. While you play, the top left of the screen shows how many you have. F5 uses an armor and F6 uses an ammo cache. Both keys can be changed under Options, Gameplay, Archipelago. An ammo cache adds 50 rounds of every ammunition type, or the most that type can hold when that is less. With a shared weapon inventory, each mission starts every firearm at 100 rounds, or at that type's maximum when it is lower. Grenades, rockets and mines start with one magazine. An armor is not used if you are already wearing one. The message when one arrives still says Body Armor or Ammo Cache.

- A small toast tells you when an item arrives for you, and when you send one to someone else. Other players' finds stay out of it. Turn it off or move it from the login screen, or from Options, Gameplay, Archipelago.
- GoldenEye can join an Archipelago multiworld. Missions unlock when their dossier arrives, and finishing a mission sends that check to the room.
- After the title, a login screen asks for the room. CONNECT starts the attempt once the host, the port and the name are filled in. The campaign stays closed until that seed accepts you.
- Weapons you receive are added to Bond's loadout. Some items grant armour or ammo, and some are traps.
- You turn it on in the game settings or from the console. A local Archipelago server hosts the seed until the game is available on archipelago.gg.

### Changed

- The ending that used to follow Cradle now plays when you press Next after the mission that just finished the Archipelago goal. Finishing Cradle before the goal is done continues to the next mission.
- Weapons are useful. The Golden Gun stays something you need when Egyptian is in the seed and guns are shared, because that mission needs it. Generate a new seed. The toast already follows this, even in the seed you are playing.
- The game is now called Project Midas - GE 007 in Archipelago. Your yaml has to use that name, and the seed has to be generated again. A room made under the old name will not accept the game.

- The armor and ammo counters say Arm. and Amm. The drawings are gone.
- The extra items in a seed are split evenly between Body Armor, Ammo Cache and Ammo Clip. Before, almost all of the extra ones were armor, so a seed had about five clips, five caches and a lot of armor. Generate a new seed. The one you are playing keeps its items.
- Agent only now counts an Agent clear and nothing else. Clearing the mission on another difficulty no longer sends that check. Generate a new seed after changing this. An old seed that counted each objective once used different checks.
- A new seed hands out its starting missions at random, so the first one is not always Dam. Every save folder plays that same list. This room already recorded a win, so host a new one after you generate again.
- Item toasts stay on screen for about eight seconds.
- Item toasts start in the bottom left corner.
- After you pick a save, Archipelago shows the goal for this seed and how far the checks have got. The login and that page are a blank sheet, so the lines are not written over the photograph or the stamps.
- Multiplayer is no longer in the folder menu.
- On the Archipelago login, the mouse selects a line. A click fills a field, or runs Connect and Play. The arrows still work.
- The watch can hold the guns and the mission tools together. A key no longer disappears once 30 items are already there.
- Archipelago in the Gameplay list can be chosen with the mouse.

### Fixed

- The mark on an item toast lines up with the words.
- Enemy weapon rolls no longer give out a knife or a mine. A guard who got one just ran around. The other guns stay as they were.
- Enemy weapon rolls no longer give out the Silver PP7 or the Gold PP7. Either one kills in one shot. The other guns stay as they were.
- Guards who start the mission no longer all keep the level's gun when weapons are randomized. Each of them is rolled, the same as guards who show up later.
- Baron Samedi stayed where he landed and did not move. He now chases you and shoots, like a guard who has seen you.
- Greasy Hands left the gun at your feet. It is now thrown ahead of you, the way a grenade or a mine is thrown. Pick it up to have it back.
- Jammed Clip no longer cuts your ammunition in half. The gun in your hands clicks empty and stays empty for about ten seconds. Your reserve is still there, and the gun reloads from it once that is over.
- Slipped on the Ice no longer takes health or armor. You get shoved, the way a shot knocks you sideways, and the red flash still plays.

- Finishing Cradle did not send the Magnum check. It does now. The same applies to the Laser on Aztec and the Golden Gun on Egyptian. Open the game again and the check for a Cradle you already finished is sent.
- Next, after the mission that finishes the goal, plays the ending. It does the same on that goal's last mission once the goal is done, instead of jumping to a later mission.
- Enemies could be given the Golden Gun. That roll is now the Silver PP7 instead. Their other weapons stay the same.
- The game could miss an item when several messages arrived from the room at once, then complain that its item count did not match the server. It keeps those messages now, and if one is still missed it asks the server for the full list again.
- With a shared weapon inventory, picking up a Silver PP7 or a Gold PP7 dropped by a guard added it to your weapons. Those two come from the Train and Cradle cheats. The drop now only leaves ammunition. A cheat you have already unlocked still gives you its gun.
- Egyptian could come up before you had the Golden Gun. Baron Samedi only dies to that gun. With a shared weapon inventory, the gun in the temple does not stay in your hands, so the seed now has to give you the Golden Gun before Egyptian can be finished. Picking up the one in the temple still counts as recovering it. A very small seed, with one difficulty and no objective or cheat checks, starts with the Golden Gun instead. Generate a new seed. Without a shared inventory, that gun is still there to pick up.
- Enemies could be given a remote mine. On Surface 2, a guard who threw one could stick it to the helicopter and fail the objective to plant your own. Enemies are no longer given remote mines. Their other weapons stay the same.
- Camera Alert, Jammed Clip and Slipped on the Ice did not change the mission in a way you could see. The alarm now sounds, and the guards are told where you are. Scientists, civilians, Natalya and invincible people stay as they were. A jammed clip empties the gun in your hands for about ten seconds and plays the empty click. Your reserve stays, and the gun reloads from it afterwards. A slip knocks you sideways like a shot. The red flash plays, and you do not lose health or armor.
- With Mission Tools on, Frigate now starts with the tracker bug and the bomb defuser. The bug has a charge, so it can be planted. The other missions' door tools are given the same way. With Mission Tools off, dying no longer hands those tools out.
- With a shared weapon inventory, a mission started each weapon with a single magazine. The shotgun then showed 5 | 5, and switching to another gun left no reserve. Firearms now start with 100 rounds, and dying gives that reserve back.

- A dossier for one difficulty no longer arrives as a bare item number. Connecting shows the starting dossier and the starting weapon, even if this slot was already saved.
- After a mission, Next opens the next dossier you have received. Finishing Dam no longer walks you into Facility when that dossier is still locked.
- Joining a room no longer tells the server that you already finished the game.
