#ifndef AP_SURVIVAL_H
#define AP_SURVIVAL_H

/*
 * Side mode for a seed that has stalled. The multiplayer arena grid picks
 * the map. One player, that arena's setup, waves of guards. Nothing that
 * happens here is a check.
 */

void apSurvivalOpen(void);
void apSurvivalCancelPick(void);
int apSurvivalPicking(void);
/* levelId is a LEVELID from the multiplayer stage table. */
void apSurvivalStartStage(int levelId);
void apSurvivalTick(void);
/* Called from the stage loadout, before the starting gun is drawn. */
void apSurvivalOnSpawn(void);
/* 1 from the moment a stage is chosen until that run is left. */
int apSurvivalActive(void);
/* After the multiplayer setup is in place. These arenas ship an empty path
 * list; the solo list addresses the same pads. */
void apSurvivalGraftPaths(void);

#endif
