#ifndef AP_ENEMY_H
#define AP_ENEMY_H

/*
 * Seed-stable cosmetic and difficulty rolls for anonymous guards.
 * Slot data comes from the GoldenEye world. Named characters stay on the cart.
 */

void apEnemySetSeed(const char *seed);
void apEnemySetOptions(int skins, int heads, int weapons, int accuracy, int music, int lethal);

/*
 * 1 if this guard is anonymous and a roll applies.
 * *outBody is the body to spawn. *outHead is a head id, or -1 to keep the cart's head.
 * bodyRandom is 1 when the setup asked for a random body (0xFFFF).
 * explicitHead is the setup head, or a negative value when the cart picks one.
 */
int apEnemyPlan(int body, int bodyRandom, int explicitHead, int invincible, int chrnum,
                int *outBody, int *outHead);

/* Remember that this chr is in the roll. Call once, after the chr exists. */
void apEnemyCommit(void *chr);

/*
 * 1 if the caller should use *outProp and skip its own chrGiveWeapon.
 * 0 if the cart's item should be given unchanged.
 */
int apEnemyTakeGive(void *chr, int model, int item, int flags, void **outProp);

void apEnemyApplyAccuracy(void *chr);
/* 1 if a setup weapon handed to this guard was rewritten to the roll. */
int apEnemyRewriteAssigned(void *chr, int *item, int *propModel);
void apEnemyArmIfBare(void *chr);
/* The chr allocated next must not be given a rolled weapon. One spawn. */
void apEnemySpareNext(void);
void apEnemySpareCancel(void);
/* Drop this chr from the roll, including a mark left by the slot's last guard. */
void apEnemyForget(void *chr);

/* Replacement main track, or -1 to keep the cart's. */
int apEnemyMusic(int stageId);

#endif
