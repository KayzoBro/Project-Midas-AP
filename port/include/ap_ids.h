#ifndef AP_IDS_H
#define AP_IDS_H

/*
 * GoldenEye 007 Archipelago ids. Shared with archipelago/goldeneye/data.py.
 * Change both, then run: python archipelago/check_ids.py
 *
 * Items  640001.. dossiers, 640101.. weapons, 6402xx filler, 6403xx traps.
 * Locations 641000 + mission * 10 + difficulty (0 Agent, 1 Secret, 2 00).
 * Objective checks 642000 + mission * 40 + difficulty * 10 + slot.
 * Cheat checks 643000 + cheat index (0-19 target time, 20 Magnum, 21 Laser, 22 Golden Gun).
 */

#define AP_GAME_NAME "Project Midas - GE 007"

#define AP_MISSION_COUNT 20
#define AP_CAMPAIGN_COUNT 18
#define AP_WEAPON_COUNT 24
#define AP_DIFF_COUNT 3

#define AP_ITEM_DOSSIER(i) (640001 + (i))
#define AP_ITEM_DIFFDOSSIER(m, d) (640501 + (m) * 3 + (d))
#define AP_ITEM_WEAPON(i)  (640101 + (i))
#define AP_ITEM_ARMOR 640201
#define AP_ITEM_AMMO  640202
#define AP_ITEM_CLIP  640203
#define AP_ITEM_ALERT  640301
#define AP_ITEM_JAM    640302
#define AP_ITEM_SLIP   640303
#define AP_ITEM_BARON  640304
#define AP_ITEM_GREASY 640305
#define AP_ITEM_RESTART 640306
#define AP_ITEM_TINY 640307
#define AP_ITEM_FLAG 640308
#define AP_ITEM_BORIS 640309
#define AP_ITEM_DK 640310
#define AP_ITEM_PAINT 640311
#define AP_ITEM_BULLET 640312
#define AP_ITEM_TOOL(i) (640401 + (i))
#define AP_TOOL_COUNT 9

#define AP_LOC(mission, difficulty) (641000 + (mission) * 10 + (difficulty))
#define AP_OBJ(mission, diff, slot) (642000 + (mission) * 40 + (diff) * 10 + (slot))
#define AP_OBJ_SLOTS 10
#define AP_CHEAT(i) (643000 + (i))
#define AP_CHEAT_COUNT 23
#define AP_CHEAT_MAGNUM 20
#define AP_CHEAT_LASER 21
#define AP_CHEAT_GOLDEN_GUN 22

#define AP_GOAL_CRADLE 0
#define AP_GOAL_AZTEC 1
#define AP_GOAL_EGYPTIAN 2
#define AP_GOAL_CAMPAIGN 3

#define AP_CHECKS_ALL 0
#define AP_CHECKS_AGENT 1

/* Holdable guns are the first 19 weapon rows (through the rocket launcher). */
#define AP_GUN_COUNT 19

#endif
