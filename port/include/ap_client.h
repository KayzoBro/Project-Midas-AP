#ifndef AP_CLIENT_H
#define AP_CLIENT_H

/*
 * Archipelago session for the PC port.
 * Off unless midas.ini [archipelago] enabled=1. While it is on, solo mission
 * select follows received dossiers instead of the cart's save progression.
 */

void apTick(void);
void apApplyLoadout(void);
void apOnMissionComplete(int levelid, int difficulty);
void apOnBondDeath(void);
void apOnObjectiveComplete(int objectiveIndex);
void apOnCheatUnlocked(int which);
/* 1 when Next on this mission should play the credits, the way Cradle does. */
int apGoalOutro(int mission);
int apGlobalWeapons(void);
int apSupplyCount(int which);
int apUseSupply(int which);
int apSupplyHudOn(void);
const char *apSupplyKeyLabel(int which);
int apSupplyCaptureActive(void);
int apSupplyCaptureWhich(void);
void apSupplyCaptureBegin(int which);
void apSupplyCaptureCancel(void);
int apSupplyKeyDown(int scancode, int repeat);
int apBlockLooseWeapon(int weaponnum);
/* 1 while a jammed clip is keeping magazines from refilling. */
int apWeaponJammed(void);
int apAllowIntroItem(int itemId);
int apEquipIntroItem(int itemId);

/* -1 = leave the cart's unlock rules alone. Otherwise a STAGESTATUS value. */
int apStageStatus(int spLevel, int difficulty);
/* Select this seed's cart file before the folders are read. */
void apBindCartSave(void);

/* Select Mission marks. -1 if this slot is not in a seed.
 * 0 locked, 1 still to play, 2 cleared, 3 this seed does not check it. */
int apMissionMark(int mission, int difficulty);

/* Lowest mission index whose dossier is in the inventory, or -1. */
int apFirstUnlockedMission(void);

/* Tilde console: `ap <verb> [rest]`. */
void apConsole(const char *verb, const char *rest);

/* 1 only after the server has accepted this slot on a running seed. */
int apSeedReady(void);

/* Folder page: the seed's goal and how many checks are in. */
struct ApFolderStatus {
    char goal[80];
    int goalDone;
    int goalTotal;
    int missionDone;
    int missionTotal;
    int objectiveOn;
    int objectiveDone;
    int objectiveTotal;
    int cheatOn;
    int cheatDone;
    int cheatTotal;
};
void apFolderStatus(struct ApFolderStatus *out);
void apConnectionLine(char *buf, int n);
/* What Discord should show. details is the top line, state the second. */
void apPresenceFill(char *details, int nd, char *state, int ns);
const char *apDiscordApp(void);

void apGetHost(char *buf, int n);
void apGetSlot(char *buf, int n);
void apGetPassword(char *buf, int n);
int apGetPort(void);
int apGetTls(void);
void apSetHost(const char *s);
void apSetSlot(const char *s);
void apSetPassword(const char *s);
void apSetPort(int port);
void apApplyConnection(const char *host, const char *slot, const char *password, int port, int tls);

/* Receipt toasts. popup= and popup_pos= in [archipelago]. */
int apGetPopup(void);
void apSetPopup(int on);
/* 1 when Death Link is live for this slot. The menu can change it without a new seed. */
int apGetDeathLink(void);
void apToggleDeathLink(void);
const char *apPopupPosLabel(void);
void apCyclePopupPos(int dir);
void apPopupSetEnabled(int on);
const char *apPopupPosName(void);
void apPopupSetPosName(const char *name);
int apPopupPosIndex(void);
void apPopupSetPosIndex(int pos);
void apPopupCyclePos(int dir);
/* Archipelago item flags. A trap uses the gold progression logo. -1 draws none. */
#define AP_POP_NOLOGO (-1)
#define AP_POP_PROGRESSION 1
#define AP_POP_USEFUL 2
#define AP_POP_TRAP 4
void apPopupPush(const char *text, int mine, int flags);
/* A countdown line. slot is 0..2. text NULL clears that slot. It stays up
 * until cleared, unlike a receipt toast, and it draws even if receipts are off. */
void apPopupTimer(int slot, const char *text, int flags);
void apPopupTick(void);

#ifdef GE_AP_DEBUG
int apDebugCount(void);
const char *apDebugLabel(int index);
const char *apDebugNote(void);
void apDebugFire(int index);
#endif

#endif
