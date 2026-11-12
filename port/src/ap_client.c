/**
 * Archipelago session.
 *
 * midas.ini [archipelago] turns it on. While it is on, the mission folder follows
 * dossiers from the server instead of the cart save, and a completed solo mission
 * is a location check. Ids match archipelago/projectmidas/data.py.
 */
#include "ap_client.h"
#include "ap_discord.h"
#include "ap_survival.h"
#include "ap_enemy.h"
#include "ap_ids.h"
#include "ap_trap.h"
#include "ap_json.h"
#include "ap_net.h"
#include "music.h"
#include "snd.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ultra64.h>
#include <bondconstants.h>

/* SDL2 scancode numbers. The names are resolved in video.c so this file
 * does not include SDL (that header fights the project's string.h). */
#define AP_SC_ESCAPE     41
#define AP_SC_BACKSPACE  42
#define AP_SC_DELETE     76
#define AP_SC_MAX        512

extern int portVideoScancodeFromName(const char *name);
extern const char *portVideoScancodeName(int scancode);
#include "game/player.h"
#include "game/bondinv.h"
#include "game/lv.h"
#include "game/gun.h"
#include "game/cheat.h"
#include "boss.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

extern void portConLogf(const char *fmt, ...);
extern const char *portRootPath(const char *name);
extern FILE *portRootFopen(const char *name, const char *mode);
extern s32 portIniWriteStrs(const char *section, const char *const *keys, const char *const *vals, s32 n);
extern s32 get_ammo_type_for_weapon(ITEM_IDS weapon);
extern s32 get_max_ammo_for_type(s32 type);
extern u32 weaponLoadProjectileModels(ITEM_IDS modelid);
extern void alarmActivate(void);
extern void hudmsgBottomShow(char *mess);
extern s32 g_AppendCheatSinglePlayer;
extern void cheatButtonHandleCheatsTurnedOn(CHEAT_ID cheat_id);
extern void cheatButtonHandleCheatsTurnedOff(CHEAT_ID cheat_id);
extern s32 g_MainStageNum;
extern s32 modelLoad(s32 modelid);
extern void chrSetWeaponFlag4(ChrRecord *chr, GUNHAND hand);
extern void sub_GAME_7F09B398(enum GUNHAND hand);

/* AP_SYNC:missions */
static const char *const kMission[AP_MISSION_COUNT] = {
    "Dam",
    "Facility",
    "Runway",
    "Surface",
    "Bunker",
    "Silo",
    "Frigate",
    "Surface 2",
    "Bunker 2",
    "Statue",
    "Archives",
    "Streets",
    "Depot",
    "Train",
    "Jungle",
    "Control",
    "Caverns",
    "Cradle",
    "Aztec",
    "Egyptian",
};
/* AP_SYNC:weapons */
static const char *const kWeapon[AP_WEAPON_COUNT] = {
    "PP7",
    "PP7 (silenced)",
    "DD44 Dostovei",
    "Klobb",
    "KF7 Soviet",
    "ZMG (9mm)",
    "D5K Deutsche",
    "D5K (silenced)",
    "Phantom",
    "AR33 Assault Rifle",
    "RC-P90",
    "Shotgun",
    "Automatic Shotgun",
    "Sniper Rifle",
    "Cougar Magnum",
    "Golden Gun",
    "Moonraker Laser",
    "Grenade Launcher",
    "Rocket Launcher",
    "Hand Grenade",
    "Timed Mine",
    "Proximity Mine",
    "Remote Mine",
    "Throwing Knife",
};
/* AP_SYNC:difficulties */
static const char *const kDiff[AP_DIFF_COUNT] = {
    "Agent",
    "Secret Agent",
    "00 Agent",
};
/* AP_SYNC:end */

static const ITEM_IDS kWeaponId[AP_WEAPON_COUNT] = {
    ITEM_WPPK,
    ITEM_WPPKSIL,
    ITEM_TT33,
    ITEM_SKORPION,
    ITEM_AK47,
    ITEM_UZI,
    ITEM_MP5K,
    ITEM_MP5KSIL,
    ITEM_SPECTRE,
    ITEM_M16,
    ITEM_FNP90,
    ITEM_SHOTGUN,
    ITEM_AUTOSHOT,
    ITEM_SNIPERRIFLE,
    ITEM_RUGER,
    ITEM_GOLDENGUN,
    ITEM_LASER,
    ITEM_GRENADELAUNCH,
    ITEM_ROCKETLAUNCH,
    ITEM_GRENADE,
    ITEM_TIMEDMINE,
    ITEM_PROXIMITYMINE,
    ITEM_REMOTEMINE,
    ITEM_THROWKNIFE,
};
/* AP_SYNC:tools */
static const char *const kTool[AP_TOOL_COUNT] = {
    "Watch Magnet",
    "Watch Laser",
    "Key Analyser",
    "Camera",
    "Datathief",
    "Plastique",
    "Bomb Defuser",
    "Tracker Bug",
    "Explosive Floppy",
};
/* AP_SYNC:end */

/* AP_SYNC:cheats */
static const char *const kCheat[AP_CHEAT_COUNT] = {
    "Dam - Paintball Mode",
    "Facility - Invincible",
    "Runway - DK Mode",
    "Surface - 2x Grenade Launcher",
    "Bunker - 2x Rocket Launcher",
    "Silo - Turbo Mode",
    "Frigate - No Radar",
    "Surface 2 - Tiny Bond",
    "Bunker 2 - 2x Throwing Knife",
    "Statue - Fast Animation",
    "Archives - Bond Invisible",
    "Streets - Enemy Rockets",
    "Depot - Slow Animation",
    "Train - Silver PP7",
    "Jungle - 2x Hunting Knife",
    "Control - Infinite Ammo",
    "Caverns - 2x RC-P90",
    "Cradle - Gold PP7",
    "Aztec - 2x Laser",
    "Egyptian - All Guns",
    "Cradle - Magnum",
    "Aztec - Laser",
    "Egyptian - Golden Gun",
};
/* AP_SYNC:end */
static const ITEM_IDS kToolId[AP_TOOL_COUNT] = {
    ITEM_WATCHMAGNETATTRACT,
    ITEM_WATCHLASER,
    ITEM_KEYANALYSERCASE,
    ITEM_CAMERA,
    ITEM_DATATHIEF,
    ITEM_PLASTIQUE,
    ITEM_BOMBDEFUSER,
    ITEM_BUG,
    ITEM_EXPLOSIVEFLOPPY,
};

static const s32 kLevelId[AP_MISSION_COUNT] = {
    LEVELID_DAM,
    LEVELID_FACILITY,
    LEVELID_RUNWAY,
    LEVELID_SURFACE,
    LEVELID_BUNKER1,
    LEVELID_SILO,
    LEVELID_FRIGATE,
    LEVELID_SURFACE2,
    LEVELID_BUNKER2,
    LEVELID_STATUE,
    LEVELID_ARCHIVES,
    LEVELID_STREETS,
    LEVELID_DEPOT,
    LEVELID_TRAIN,
    LEVELID_JUNGLE,
    LEVELID_CONTROL,
    LEVELID_CAVERNS,
    LEVELID_CRADLE,
    LEVELID_AZTEC,
    LEVELID_EGYPT,
};

#define AP_MSG_CAP (2 * 1024 * 1024)
#define AP_ITEM_CAP 4096

static int sInited;
static int sEnabled;
static int sTls = 1;
static int sPort = 38281;
static int sHold;
static int sLive;
static int sWasOpen;
static int sConnectSent;
static int sPlayingSent;
static int sGoalAnnounced;
static int sGoalJustMet;
static int sResend;
static int sNeedSync;
static time_t sSyncNext;
static int sWaitLog;
static int sSettle;
static int sGoal = AP_GOAL_CRADLE;
static int sGoalDiff;
static int sGoalAll;
static int sChecks = AP_CHECKS_ALL;
static int sCheckMask = 7;
static int sSplitDossiers;
static int sBonus = 1;
static int sObjectiveChecks;
static int sCheatChecks;
static int sGlobalWeapons;
static int sMissionTools = 1;
static int sDeathLink;
static int sDeathFromLink;
static int sDeathQueued;
static int sDeathSeq;
static int sNatalyaSeen;
static int sNatalyaLatched;
static int sNatalyaMission = -2;
static char sLastDeathTime[48];
static char sDeathToast[96];
static int sVerMaj;
static int sVerMin = 6;
static int sVerBld = 2;
static int sItemIndex;
static int sMySlot;
static unsigned int sDossierDiff[AP_DIFF_COUNT];
static unsigned int sWeapons;
static unsigned int sTools;
static int sPendArmor;
static int sPendAmmo;
static int sArmorStock;
static int sAmmoStock;
static char sArmorKey[32] = "F5";
static char sAmmoKey[32] = "F6";
static int sArmorSc;
static int sAmmoSc;
static int sKeyCap = -1;
static int sSwallowSc;
static int sPendAlert;
static int sPendJam;
static int sPendSlip;
static int sPendClip;
static int sPendBaron;
static int sPendGreasy;
static int sPendRestart;
static int sPendTiny;
static int sPendFlag;
static s32 sJamUntil;
static s32 sTinyUntil;
static s32 sFlagUntil;
static s32 sClockSeen = -1;
static int sTinyOwned;
static int sTinyWasOn;
static int sFlagOwned;
static int sFlagAdded;
static int sFlagBody;
static int sFlagTries;
static ITEM_IDS sFlagRight;
static ITEM_IDS sFlagLeft;
static s32 sDkUntil;
static int sDkOwned;
static int sDkWasOn;
static s32 sPaintUntil;
static int sPaintOwned;
static int sPaintWasOn;
/* Remaining ticks while the effect is parked between missions. The stage
 * clock restarts on every load, so a deadline from the previous mission
 * cannot be compared with it. Menus do not spend these ticks. */
static s32 sJamLeft;
static s32 sTinyLeft;
static s32 sFlagLeft;
static s32 sDkLeft;
static s32 sPaintLeft;
static int sTrapPaused;
static int sPendBoris;
static int sPendDk;
static int sPendPaint;
static int sPendBullet;
/* Menu override for the DeathLink tag. -1 follows the seed's slot data.
 * 0 and 1 are written to midas.ini and sent with ConnectUpdate, so the
 * switch works on a seed that was generated with the option the other way. */
static int sDeathChoice = -1;
/* How many distinct games in this room still need a datapackage, and which
 * one we asked for last. Declared up here because connect resets them. */
static int sPackN;
static int sPackNext;
static int sPackWait;
#ifdef GE_AP_DEBUG
static int sPendDeath;
static char sDebugNote[72];
#endif
static unsigned char sChecked[AP_MISSION_COUNT];
static unsigned short sObjChecked[AP_MISSION_COUNT][4];
static unsigned int sCheatChecked;
static int sOwned[AP_ITEM_CAP];
static int sOwnedCount;
static char sHost[128] = "archipelago.gg";
/* Public Discord application id. It only names the presence; it is not a secret. */
static char sDiscordApp[24] = "1570005502576820305";
static char sSlot[48];
static char sPassword[80];
static char sUuid[40];
static char sSeed[128];
/* Seed already stored when this process started. Its cart follows that seed. */
static char sCartSeed[128];
static char sNames[64][20];
static char *sMsg;
static time_t sNextTry;

static int eqI(const char *a, const char *b)
{
    if (a == NULL || b == NULL)
    {
        return 0;
    }
    while (*a != '\0' && *b != '\0')
    {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char) (ca + 32);
        if (cb >= 'A' && cb <= 'Z') cb = (char) (cb + 32);
        if (ca != cb)
        {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static void trim(char *s)
{
    char *e;
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
    {
        memmove(s, s + 1, strlen(s));
    }
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n'))
    {
        *--e = '\0';
    }
}

static int truth(const char *s)
{
    return eqI(s, "1") || eqI(s, "true") || eqI(s, "yes") || eqI(s, "on");
}

static int missionIndex(int levelid)
{
    int i;
    for (i = 0; i < AP_MISSION_COUNT; i++)
    {
        if (kLevelId[i] == levelid)
        {
            return i;
        }
    }
    return -1;
}

static int inMission(void)
{
    if (!sEnabled || !sLive || g_CurrentPlayer == NULL)
    {
        return 0;
    }
    if (getPlayerCount() != 1 || lvlGetSelectedDifficulty() == DIFFICULTY_MULTI)
    {
        return 0;
    }
    return missionIndex((int) bossGetStageNum()) >= 0;
}

static int sQuietNotify;

static void notify(const char *msg, int hud)
{
    char line[96];
    int i;

    portConLogf("ge: ap: %s\n", msg);
    if (!hud || sQuietNotify || !inMission())
    {
        return;
    }
    snprintf(line, sizeof(line), "%s", msg);
    for (i = 0; line[i] != '\0'; i++)
    {
        if (line[i] >= 'a' && line[i] <= 'z')
        {
            line[i] = (char) (line[i] - 32);
        }
    }
    hudmsgBottomShow(line);
}

/* Items that entered this slot. Checks and other players' finds stay off this toast. */
static void toastItem(const char *msg, int flags)
{
    portConLogf("ge: ap: %s\n", msg);
    if (!sQuietNotify)
    {
        apPopupPush(msg, 1, flags);
    }
}

static void itemName(int id, char *buf, int n)
{
    if (id >= AP_ITEM_DOSSIER(0) && id < AP_ITEM_DOSSIER(AP_MISSION_COUNT))
    {
        snprintf(buf, (size_t) n, "%s Dossier", kMission[id - AP_ITEM_DOSSIER(0)]);
    }
    else if (id >= AP_ITEM_WEAPON(0) && id < AP_ITEM_WEAPON(AP_WEAPON_COUNT))
    {
        snprintf(buf, (size_t) n, "%s", kWeapon[id - AP_ITEM_WEAPON(0)]);
    }
    else if (id >= AP_ITEM_TOOL(0) && id < AP_ITEM_TOOL(AP_TOOL_COUNT))
    {
        snprintf(buf, (size_t) n, "%s", kTool[id - AP_ITEM_TOOL(0)]);
    }
    else if (id == AP_ITEM_ARMOR) snprintf(buf, (size_t) n, "Body Armor");
    else if (id == AP_ITEM_AMMO) snprintf(buf, (size_t) n, "Ammo Cache");
    else if (id == AP_ITEM_CLIP) snprintf(buf, (size_t) n, "Ammo Clip");
    else if (id == AP_ITEM_ALERT) snprintf(buf, (size_t) n, "Camera Alert");
    else if (id == AP_ITEM_JAM) snprintf(buf, (size_t) n, "Jammed Clip");
    else if (id == AP_ITEM_SLIP) snprintf(buf, (size_t) n, "Slipped on the Ice");
    else if (id == AP_ITEM_BARON) snprintf(buf, (size_t) n, "Baron Samedi");
    else if (id == AP_ITEM_GREASY) snprintf(buf, (size_t) n, "Greasy Hands");
    else if (id == AP_ITEM_RESTART) snprintf(buf, (size_t) n, "Restart Mission");
    else if (id == AP_ITEM_TINY) snprintf(buf, (size_t) n, "Tiny Bond");
    else if (id == AP_ITEM_FLAG) snprintf(buf, (size_t) n, "The Living Daylights");
    else if (id == AP_ITEM_BORIS) snprintf(buf, (size_t) n, "I am invisible");
    else if (id == AP_ITEM_DK) snprintf(buf, (size_t) n, "DK Mode");
    else if (id == AP_ITEM_PAINT) snprintf(buf, (size_t) n, "Paintball");
    else if (id == AP_ITEM_BULLET) snprintf(buf, (size_t) n, "Single Bullet");
    else if (id >= AP_ITEM_DIFFDOSSIER(0, 0) && id < AP_ITEM_DIFFDOSSIER(AP_MISSION_COUNT, 0))
    {
        int rel = id - AP_ITEM_DIFFDOSSIER(0, 0);
        int mission = rel / 3;
        int diff = rel % 3;
        if (mission >= 0 && mission < AP_MISSION_COUNT && diff >= 0 && diff < AP_DIFF_COUNT)
        {
            snprintf(buf, (size_t) n, "%s Dossier (%s)", kMission[mission], kDiff[diff]);
            return;
        }
        snprintf(buf, (size_t) n, "Item %d", id);
    }
    else snprintf(buf, (size_t) n, "Item %d", id);
}

static void locLabel(int mission, int diff, char *buf, int n)
{
    snprintf(buf, (size_t) n, "%s (%s)", kMission[mission], kDiff[diff]);
}

static const char *goalName(void)
{
    if (sGoal == AP_GOAL_AZTEC) return "Aztec";
    if (sGoal == AP_GOAL_EGYPTIAN) return "Egyptian";
    if (sGoal == AP_GOAL_CAMPAIGN) return "the campaign";
    return "Cradle";
}

static void ensureSaveDir(void)
{
    char path[1200];

    snprintf(path, sizeof(path), "%s", portRootPath("Data"));
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
    snprintf(path, sizeof(path), "%s", portRootPath("Data/Saves"));
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

static void saveState(void)
{
    FILE *f;
    int m, d, first;

    ensureSaveDir();
    f = portRootFopen("Data/Saves/archipelago.txt", "w");
    if (f == NULL)
    {
        return;
    }
    fprintf(f, "uuid=%s\nseed=%s\nindex=%d\nchecked=", sUuid, sSeed, sItemIndex);
    first = 1;
    for (m = 0; m < AP_MISSION_COUNT; m++)
    {
        int slot;
        for (d = 0; d < AP_DIFF_COUNT; d++)
        {
            if (sChecked[m] & (1u << d))
            {
                fprintf(f, "%s%d", first ? "" : ",", AP_LOC(m, d));
                first = 0;
            }
        }
        for (d = 0; d < 4; d++)
        {
            for (slot = 0; slot < AP_OBJ_SLOTS; slot++)
            {
                if (sObjChecked[m][d] & (1u << slot))
                {
                    fprintf(f, "%s%d", first ? "" : ",", AP_OBJ(m, d, slot));
                    first = 0;
                }
            }
        }
    }
    for (m = 0; m < AP_CHEAT_COUNT; m++)
    {
        if (sCheatChecked & (1u << m))
        {
            fprintf(f, "%s%d", first ? "" : ",", AP_CHEAT(m));
            first = 0;
        }
    }
    fprintf(f, "\narmor=%d\nammo=%d\n", sArmorStock, sAmmoStock);
    fclose(f);
}

static void rememberId(int id)
{
    int index;

    if (id >= AP_ITEM_DOSSIER(0) && id < AP_ITEM_DOSSIER(AP_MISSION_COUNT))
    {
        int mission = id - AP_ITEM_DOSSIER(0);
        int diff;
        for (diff = 0; diff < AP_DIFF_COUNT; diff++)
        {
            sDossierDiff[diff] |= 1u << mission;
        }
    }
    else if (id >= AP_ITEM_DIFFDOSSIER(0, 0) && id < AP_ITEM_DIFFDOSSIER(AP_MISSION_COUNT, 0))
    {
        int rel = id - AP_ITEM_DIFFDOSSIER(0, 0);
        int mission = rel / 3;
        int diff = rel % 3;
        if (mission >= 0 && mission < AP_MISSION_COUNT && diff >= 0 && diff < AP_DIFF_COUNT)
        {
            sDossierDiff[diff] |= 1u << mission;
        }
    }
    else if (id >= AP_ITEM_WEAPON(0) && id < AP_ITEM_WEAPON(AP_WEAPON_COUNT))
    {
        index = id - AP_ITEM_WEAPON(0);
        sWeapons |= 1u << index;
    }
    else if (id >= AP_ITEM_TOOL(0) && id < AP_ITEM_TOOL(AP_TOOL_COUNT))
    {
        sTools |= 1u << (id - AP_ITEM_TOOL(0));
    }
}

static void rebuildOwned(void)
{
    int i;
    memset(sDossierDiff, 0, sizeof(sDossierDiff));
    sWeapons = 0;
    sTools = 0;
    for (i = 0; i < sOwnedCount; i++)
    {
        rememberId(sOwned[i]);
    }
}

static void addAmmo(int type, int amount)
{
    int cur, max, next;

    if (g_CurrentPlayer == NULL || type <= 0 || type >= 30 || amount <= 0)
    {
        return;
    }
    cur = g_CurrentPlayer->ammoheldarr[type];
    max = get_max_ammo_for_type(type);
    next = cur + amount;
    if (max > 0 && next > max)
    {
        next = max;
    }
    if (next < 0)
    {
        next = 0;
    }
    g_CurrentPlayer->ammoheldarr[type] = next;
}

static void grantWeapon(int index)
{
    ITEM_IDS id;
    int ammo;

    if (index < 0 || index >= AP_WEAPON_COUNT || g_CurrentPlayer == NULL)
    {
        return;
    }
    id = kWeaponId[index];
    weaponLoadProjectileModels(id);
    bondinvAddInvItem(id);
    if (id == ITEM_REMOTEMINE)
    {
        bondinvAddInvItem(ITEM_TRIGGER);
    }
    ammo = get_ammo_type_for_weapon(id);
    {
        WeaponStats *st = get_ptr_item_statistics(id);
        int mag = (st != NULL && st->MagSize > 0) ? st->MagSize : 40;
        addAmmo(ammo, mag);
    }
}

static void applyArmor(void)
{
    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    g_CurrentPlayer->bondarmour = 1.0f;
    g_CurrentPlayer->actual_armor = 2.0f;
}

/* Mission start still hands grenades, rockets and mines a single magazine.
 * giveStartingAmmo leaves those types alone, because they cannot hold 100. */
static void giveBaseMagazines(void)
{
    int seen[30];
    int i;

    memset(seen, 0, sizeof(seen));
    for (i = 0; i < AP_WEAPON_COUNT; i++)
    {
        WeaponStats *st = get_ptr_item_statistics(kWeaponId[i]);
        int type;
        int mag;
        if (st == NULL)
        {
            continue;
        }
        type = st->AmmoType;
        mag = st->MagSize;
        if (type <= 0 || type >= 30 || mag <= 0 || seen[type])
        {
            continue;
        }
        seen[type] = 1;
        addAmmo(type, mag);
    }
}

/* One amount for every ammunition type. Using the first gun of a type made
 * 9mm a PP7 magazine and rifle rounds a KF7 magazine. Types that cannot
 * hold this many stop at their own maximum inside addAmmo. */
#define AP_CACHE_ROUNDS 50

/* Dam, Facility and the other missions hand the starting firearm 100
 * rounds. One magazine is only enough to fill the gun: a shotgun then
 * reads 5 | 5. Types that cannot hold 100 (grenades, rockets, mines)
 * stay on the single magazine. */
#define AP_START_ROUNDS 100

static void giveStartingAmmo(int everyType)
{
    int i;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    for (i = 0; i < AP_WEAPON_COUNT; i++)
    {
        WeaponStats *st;
        int type;
        int max;
        if (!everyType && (sWeapons & (1u << i)) == 0)
        {
            continue;
        }
        st = get_ptr_item_statistics(kWeaponId[i]);
        if (st == NULL)
        {
            continue;
        }
        type = st->AmmoType;
        if (type <= 0 || type >= 30 || st->MagSize <= 0)
        {
            continue;
        }
        /* The shrine Golden Gun is a collect objective. A full reserve
         * makes the game refuse the pickup, so that ammo stays empty until
         * the gun is actually owned. */
        if (kWeaponId[i] == ITEM_GOLDENGUN && (sWeapons & (1u << i)) == 0)
        {
            continue;
        }
        max = get_max_ammo_for_type(type);
        if (max < AP_START_ROUNDS)
        {
            continue;
        }
        if (g_CurrentPlayer->ammoheldarr[type] < AP_START_ROUNDS)
        {
            g_CurrentPlayer->ammoheldarr[type] = AP_START_ROUNDS;
        }
    }
}

static void applyAmmoCache(void)
{
    int seen[30];
    int i;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    memset(seen, 0, sizeof(seen));
    for (i = 0; i < AP_WEAPON_COUNT; i++)
    {
        WeaponStats *st = get_ptr_item_statistics(kWeaponId[i]);
        int type;
        if (st == NULL)
        {
            continue;
        }
        type = st->AmmoType;
        if (type <= 0 || type >= 30 || st->MagSize <= 0 || seen[type])
        {
            continue;
        }
        seen[type] = 1;
        addAmmo(type, AP_CACHE_ROUNDS);
    }
}

static void applyClip(void)
{
    GUNHAND hands[2];
    int i;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    hands[0] = GUNRIGHT;
    hands[1] = GUNLEFT;
    for (i = 0; i < 2; i++)
    {
        ITEM_IDS id = getCurrentPlayerWeaponId(hands[i]);
        WeaponStats *st = get_ptr_item_statistics(id);
        int before;
        if (st == NULL || st->AmmoType <= 0 || st->MagSize <= 0)
        {
            continue;
        }
        before = g_CurrentPlayer->ammoheldarr[st->AmmoType];
        addAmmo(st->AmmoType, st->MagSize);
        if (g_CurrentPlayer->ammoheldarr[st->AmmoType] != before)
        {
            sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, PICKUP_AMMO_SFX, 0);
        }
        return;
    }
}

static void applyAlert(void)
{
    extern ChrRecord *g_ChrSlots;
    extern s32 g_NumChrSlots;
    extern void chrlvAlertGuardToPlayerPosition(ChrRecord *chr);
    int i;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    alarmActivate();
    sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, ALARM3_SFX, 0);
    if (g_ChrSlots == NULL)
    {
        return;
    }
    /* A shot only alerts the room, and only guards whose script checks the
     * alarm. A camera alert tells every living guard where Bond is. */
    for (i = 0; i < g_NumChrSlots; i++)
    {
        ChrRecord *chr = &g_ChrSlots[i];

        if (chr->model == NULL)
        {
            continue;
        }
        if (chr->actiontype == ACT_DIE || chr->actiontype == ACT_DEAD)
        {
            continue;
        }
        if (chr->chrflags & (CHRFLAG_COUNT_DEATH_AS_CIVILIAN | CHRFLAG_INVINCIBLE))
        {
            continue;
        }
        if (chr->bodynum == BODY_Natalya_Skirt || chr->bodynum == BODY_Natalya_Jungle_Fatigues)
        {
            continue;
        }
        chrlvAlertGuardToPlayerPosition(chr);
    }
}

int apWeaponJammed(void)
{
    return g_GlobalTimer < sJamUntil;
}

static void applyJam(void)
{
    int hand;
    int jammed = 0;
    s32 base;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    for (hand = 0; hand < 2; hand++)
    {
        struct hand *held = &g_CurrentPlayer->hands[hand];
        ITEM_IDS id = getCurrentPlayerWeaponId((GUNHAND) hand);
        WeaponStats *stats = get_ptr_item_statistics(id);

        /* A clip is a firearm magazine. Fists, knives and throwables have none. */
        if (stats == NULL || stats->AmmoType == 0 || stats->MagSize <= 1)
        {
            continue;
        }
        held->weapon_ammo_in_magazine = 0;
        held->numvisibleshells = 0;
        if (held->weapon_action_state == GUN_ANIM_STATE_RELOAD_START
            || held->weapon_action_state == GUN_ANIM_STATE_RELOAD_LOWER
            || held->weapon_action_state == GUN_ANIM_STATE_RELOAD_SWAP
            || held->weapon_action_state == GUN_ANIM_STATE_RELOAD_RAISE)
        {
            held->weapon_action_state = GUN_ANIM_STATE_IDLE;
            held->field_890 = 0;
            held->field_88C = 0;
        }
        jammed = 1;
    }
    if (!jammed)
    {
        return;
    }
    /* 600 ticks is ten seconds on the 60Hz clock. A second jam extends it. */
    base = g_GlobalTimer;
    if (sJamUntil > base)
    {
        base = sJamUntil;
    }
    sJamUntil = base + 600;
    sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, EMPTY_GUN_FIRE_SFX, 0);
}

static void applySlip(void)
{
    f32 side;

    if (g_CurrentPlayer == NULL || g_CurrentPlayer->bonddead)
    {
        return;
    }
    /* The bullet shove, sideways, and about twice as hard. Health is left alone. */
    side = (g_GlobalTimer & 1) ? 4.0f : -4.0f;
    g_CurrentPlayer->bondshotspeed.x += (-g_CurrentPlayer->vv_costheta * side) + (-g_CurrentPlayer->vv_sintheta * 1.5f);
    g_CurrentPlayer->bondshotspeed.z += (-g_CurrentPlayer->vv_sintheta * side) + (g_CurrentPlayer->vv_costheta * 1.5f);
    record_damage_kills(0.0f, 0.0f, 0.0f, -1, 0);
    sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, BODY_FALL_C1_SFX, 0);
}

/* The cart's own cheat path. It sets the live bit, halves the camera height,
 * and halves the model that is already on screen. A later rebuild stays tiny
 * because makeonebody reads that same bit. */
static void endTiny(void)
{
    if (!sTinyOwned)
    {
        sTinyUntil = 0;
        return;
    }
    if (!sTinyWasOn)
    {
        if (g_CurrentPlayer != NULL)
        {
            cheatButtonHandleCheatsTurnedOff(CHEAT_TINY_BOND);
        }
        else
        {
            g_CheatPlayerTextRelated[CHEAT_TINY_BOND] = 0;
            if (g_playerPerm != NULL)
            {
                g_playerPerm->player_perspective_height = 1.0f;
            }
        }
    }
    sTinyOwned = 0;
    sTinyWasOn = 0;
    sTinyUntil = 0;
}

static void applyTiny(void)
{
    s32 base;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    base = g_GlobalTimer;
    if (sTinyUntil > base)
    {
        base = sTinyUntil;
    }
    sTinyUntil = base + 60 * 60;
    if (sTinyOwned)
    {
        return;
    }
    sTinyWasOn = cheatIsActive(CHEAT_TINY_BOND) ? 1 : 0;
    sTinyOwned = 1;
    if (!sTinyWasOn)
    {
        cheatButtonHandleCheatsTurnedOn(CHEAT_TINY_BOND);
    }
}

/* The token's first-person header is empty. Filling it with PflagZ crashed:
 * the gun drawer indexes switches the flag model does not have. The flag is
 * the multiplayer prop, attached to Bond the way Flag Tag already does. */
static void attachFlagBody(void)
{
    ChrRecord *chr;

    if (sFlagBody || g_CurrentPlayer == NULL || g_CurrentPlayer->prop == NULL)
    {
        return;
    }
    if (getCurrentPlayerWeaponId(GUNRIGHT) != ITEM_TOKEN)
    {
        return;
    }
    chr = g_CurrentPlayer->prop->chr;
    if (chr == NULL)
    {
        return;
    }
    modelLoad(PROP_FLAG);
    if (chr->weapons_held[GUNRIGHT] != NULL)
    {
        chrSetWeaponFlag4(chr, GUNRIGHT);
    }
    if (chr->weapons_held[GUNRIGHT] == NULL)
    {
        sub_GAME_7F09B398(GUNRIGHT);
        sFlagBody = 1;
    }
    else if (++sFlagTries > 30)
    {
        sFlagBody = 1;
    }
}

static void endFlag(int restore)
{
    if (!sFlagOwned)
    {
        sFlagUntil = 0;
        return;
    }
    if (g_CurrentPlayer != NULL && g_CurrentPlayer->prop != NULL && g_CurrentPlayer->prop->chr != NULL)
    {
        ChrRecord *chr = g_CurrentPlayer->prop->chr;
        /* Only the flag we attached. The starting gun stays if the attach
         * never happened. */
        if (sFlagBody && chr->weapons_held[GUNRIGHT] != NULL)
        {
            chrSetWeaponFlag4(chr, GUNRIGHT);
        }
        if (sFlagAdded)
        {
            bondinvRemoveItemByID(ITEM_TOKEN);
        }
        if (restore && !g_CurrentPlayer->bonddead)
        {
            currentPlayerEquipWeaponWrapper(GUNRIGHT, sFlagRight);
            currentPlayerEquipWeaponWrapper(GUNLEFT, sFlagLeft);
        }
    }
    sFlagOwned = 0;
    sFlagAdded = 0;
    sFlagBody = 0;
    sFlagTries = 0;
    sFlagUntil = 0;
}

static void holdFlag(void)
{
    struct hand *right;
    struct hand *left;

    if (!sFlagOwned || g_CurrentPlayer == NULL || g_CurrentPlayer->bonddead)
    {
        return;
    }
    right = &g_CurrentPlayer->hands[GUNRIGHT];
    left = &g_CurrentPlayer->hands[GUNLEFT];
    if (right->weaponnum != ITEM_TOKEN && right->weapon_next_weapon != ITEM_TOKEN)
    {
        currentPlayerEquipWeaponWrapper(GUNRIGHT, ITEM_TOKEN);
    }
    if (left->weaponnum != ITEM_UNARMED && left->weapon_next_weapon != ITEM_UNARMED)
    {
        currentPlayerEquipWeaponWrapper(GUNLEFT, ITEM_UNARMED);
    }
    /* An empty token does not enter the trigger press. A loaded one would
     * fire, because the flag's stats still carry a damage value. */
    if (right->weaponnum == ITEM_TOKEN)
    {
        s32 type = get_ammo_type_for_weapon(ITEM_TOKEN);

        right->weapon_ammo_in_magazine = 0;
        if (type > 0 && type < 30)
        {
            g_CurrentPlayer->ammoheldarr[type] = 0;
        }
    }
    attachFlagBody();
}

static void armFlag(s32 ticks)
{
    s32 base;

    if (g_CurrentPlayer == NULL || g_CurrentPlayer->bonddead || ticks <= 0)
    {
        return;
    }
    base = g_GlobalTimer;
    if (sFlagUntil > base)
    {
        base = sFlagUntil;
    }
    sFlagUntil = base + ticks;
    if (sFlagOwned)
    {
        return;
    }
    sFlagRight = getCurrentPlayerWeaponId(GUNRIGHT);
    sFlagLeft = getCurrentPlayerWeaponId(GUNLEFT);
    sFlagAdded = bondinvHasInvItem(ITEM_TOKEN) ? 0 : 1;
    if (sFlagAdded)
    {
        bondinvAddInvItem(ITEM_TOKEN);
    }
    sFlagOwned = 1;
    sFlagBody = 0;
    sFlagTries = 0;
    currentPlayerEquipWeaponWrapper(GUNRIGHT, ITEM_TOKEN);
    currentPlayerEquipWeaponWrapper(GUNLEFT, ITEM_UNARMED);
}

static void applyFlag(void)
{
    armFlag(30 * 60);
}

static void dropTimed(int restoreFlag);
static void pauseTimed(s32 now);

/* A raw teleport to the start pad can clear Frigate on the spot, or leave
 * the Train ending behind the player. Reloading the stage runs the same
 * path as starting the mission again. */
static int applyRestart(void)
{
    LEVELID stage;

    if (g_CurrentPlayer == NULL || !inMission())
    {
        return 0;
    }
    stage = bossGetStageNum();
    if ((s32) stage < 0 || stage == LEVELID_TITLE || stage == LEVELID_CUBA)
    {
        return 0;
    }
    if (g_MainStageNum >= 0)
    {
        return 1;
    }
    /* Keep whatever time is left. The reloaded mission resumes it. */
    pauseTimed(g_GlobalTimer);
    bossSetLoadedStage(stage);
    return 1;
}

int apTrapFlagHeld(void)
{
    return sFlagOwned;
}

static void putTicks(int slot, const char *name, s32 ticks)
{
    char buf[64];
    int sec;

    if (ticks <= 0)
    {
        apPopupTimer(slot, NULL, 0);
        return;
    }
    sec = (int) ((ticks + 59) / 60);
    snprintf(buf, sizeof(buf), "%s %d:%02d", name, sec / 60, sec % 60);
    apPopupTimer(slot, buf, AP_POP_TRAP);
}

static void putTimer(int slot, const char *name, s32 until)
{
    if (g_CurrentPlayer == NULL || until <= g_GlobalTimer)
    {
        apPopupTimer(slot, NULL, 0);
        return;
    }
    putTicks(slot, name, until - g_GlobalTimer);
}

static void endCheat(int *owned, int *wasOn, s32 *until, CHEAT_ID id)
{
    if (!*owned)
    {
        *until = 0;
        return;
    }
    if (!*wasOn)
    {
        if (g_CurrentPlayer != NULL)
        {
            cheatButtonHandleCheatsTurnedOff(id);
        }
        else
        {
            g_CheatPlayerTextRelated[id] = 0;
        }
    }
    *owned = 0;
    *wasOn = 0;
    *until = 0;
}

/* A second copy extends the clock. TurnedOn is not called again: DK Mode
 * multiplies every guard's scale, so a second call would shrink them twice. */
static void applyCheat(int *owned, int *wasOn, s32 *until, CHEAT_ID id, s32 ticks)
{
    s32 base;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    base = g_GlobalTimer;
    if (*until > base)
    {
        base = *until;
    }
    *until = base + ticks;
    if (*owned)
    {
        return;
    }
    *wasOn = cheatIsActive(id) ? 1 : 0;
    *owned = 1;
    if (!*wasOn)
    {
        cheatButtonHandleCheatsTurnedOn(id);
    }
}

static void endDk(void)
{
    endCheat(&sDkOwned, &sDkWasOn, &sDkUntil, CHEAT_DK_MODE);
}

static void endPaint(void)
{
    endCheat(&sPaintOwned, &sPaintWasOn, &sPaintUntil, CHEAT_PAINTBALL);
}

static void applyDk(void)
{
    applyCheat(&sDkOwned, &sDkWasOn, &sDkUntil, CHEAT_DK_MODE, 60 * 60);
}

static void applyPaint(void)
{
    applyCheat(&sPaintOwned, &sPaintWasOn, &sPaintUntil, CHEAT_PAINTBALL, 180 * 60);
}

static void applyBullet(void)
{
    GUNHAND hand;
    ITEM_IDS id;
    WeaponStats *st;
    int type;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    hand = GUNRIGHT;
    id = getCurrentPlayerWeaponId(hand);
    st = get_ptr_item_statistics(id);
    if (st == NULL || st->AmmoType <= 0 || st->MagSize <= 0)
    {
        hand = GUNLEFT;
        id = getCurrentPlayerWeaponId(hand);
        st = get_ptr_item_statistics(id);
    }
    if (st == NULL || st->AmmoType <= 0 || st->MagSize <= 0)
    {
        return;
    }
    /* One round in the reserve of the gun in hand. The magazine is left
     * as it is, and a full reserve stays full. */
    type = st->AmmoType;
    addAmmo(type, 1);
}

static void showTimers(void)
{
    if (sTrapPaused)
    {
        putTicks(0, "Jammed Clip", sJamLeft);
        putTicks(1, "Tiny Bond", sTinyLeft);
        putTicks(2, "The Living Daylights", sFlagLeft);
        putTicks(3, "DK Mode", sDkLeft);
        putTicks(4, "Paintball", sPaintLeft);
        return;
    }
    putTimer(0, "Jammed Clip", sJamUntil);
    putTimer(1, "Tiny Bond", sTinyOwned ? sTinyUntil : 0);
    putTimer(2, "The Living Daylights", sFlagOwned ? sFlagUntil : 0);
    putTimer(3, "DK Mode", sDkOwned ? sDkUntil : 0);
    putTimer(4, "Paintball", sPaintOwned ? sPaintUntil : 0);
}

static void dropTimed(int restoreFlag)
{
    endTiny();
    endFlag(restoreFlag);
    endDk();
    endPaint();
    sJamUntil = 0;
    sJamLeft = sTinyLeft = sFlagLeft = sDkLeft = sPaintLeft = 0;
    sTrapPaused = 0;
}

/* Park a running effect. `now` is the mission clock the deadlines were
 * written against. The cheat and the flag are turned off so the menu and
 * the next body's first spawn are not left mid-effect. */
static void pauseTimed(s32 now)
{
    if (sTrapPaused)
    {
        return;
    }
    if (sJamUntil > now)
    {
        sJamLeft = sJamUntil - now;
    }
    if (sTinyOwned && sTinyUntil > now)
    {
        sTinyLeft = sTinyUntil - now;
    }
    if (sFlagOwned && sFlagUntil > now)
    {
        sFlagLeft = sFlagUntil - now;
    }
    if (sDkOwned && sDkUntil > now)
    {
        sDkLeft = sDkUntil - now;
    }
    if (sPaintOwned && sPaintUntil > now)
    {
        sPaintLeft = sPaintUntil - now;
    }
    if (sJamLeft <= 0 && sTinyLeft <= 0 && sFlagLeft <= 0 && sDkLeft <= 0 && sPaintLeft <= 0)
    {
        return;
    }
    sTrapPaused = 1;
    endTiny();
    endFlag(0);
    endDk();
    endPaint();
    sJamUntil = 0;
}

static void resumeTimed(void)
{
    if (!sTrapPaused || g_CurrentPlayer == NULL || g_CurrentPlayer->bonddead)
    {
        return;
    }
    sTrapPaused = 0;
    if (sJamLeft > 0)
    {
        sJamUntil = g_GlobalTimer + sJamLeft;
    }
    sJamLeft = 0;
    if (sTinyLeft > 0)
    {
        applyCheat(&sTinyOwned, &sTinyWasOn, &sTinyUntil, CHEAT_TINY_BOND, sTinyLeft);
    }
    sTinyLeft = 0;
    if (sDkLeft > 0)
    {
        applyCheat(&sDkOwned, &sDkWasOn, &sDkUntil, CHEAT_DK_MODE, sDkLeft);
    }
    sDkLeft = 0;
    if (sPaintLeft > 0)
    {
        applyCheat(&sPaintOwned, &sPaintWasOn, &sPaintUntil, CHEAT_PAINTBALL, sPaintLeft);
    }
    sPaintLeft = 0;
    if (sFlagLeft > 0)
    {
        armFlag(sFlagLeft);
    }
    sFlagLeft = 0;
}

static void trapMaintain(void)
{
    extern MENU get_currentmenu(void);

    /* Briefing and mission-complete stay on the mission clock, but they are
     * menus. Freeze the remainder there instead of letting it run out. */
    if (inMission() && g_CurrentPlayer != NULL && get_currentmenu() != MENU_RUN_STAGE)
    {
        if (!sTrapPaused)
        {
            pauseTimed(g_GlobalTimer);
        }
        showTimers();
        return;
    }
    if (!inMission() || g_CurrentPlayer == NULL)
    {
        if (!sTrapPaused)
        {
            s32 now = g_GlobalTimer;

            /* The stage clock is already back at zero on this frame. The
             * last tick we saw is the one the deadlines still mean. */
            if (sClockSeen >= 0 && g_GlobalTimer < sClockSeen)
            {
                now = sClockSeen;
            }
            pauseTimed(now);
        }
        sClockSeen = -1;
        showTimers();
        return;
    }
    if (!sTrapPaused && sClockSeen >= 0 && g_GlobalTimer < sClockSeen)
    {
        pauseTimed(sClockSeen);
    }
    sClockSeen = g_GlobalTimer;
    /* Let the new body and the intro weapons exist before the effect
     * grabs them again. Thirty ticks matches the other queued effects. */
    if (sTrapPaused && g_GlobalTimer >= 30)
    {
        resumeTimed();
    }
    if (sTinyOwned && g_GlobalTimer >= sTinyUntil)
    {
        endTiny();
    }
    if (sFlagOwned && g_GlobalTimer >= sFlagUntil)
    {
        endFlag(1);
    }
    if (sDkOwned && g_GlobalTimer >= sDkUntil)
    {
        endDk();
    }
    if (sPaintOwned && g_GlobalTimer >= sPaintUntil)
    {
        endPaint();
    }
    if (sFlagOwned)
    {
        holdFlag();
    }
    showTimers();
}

static int applyKind(int kind)
{
    if (kind == AP_ITEM_ARMOR) applyArmor();
    else if (kind == AP_ITEM_AMMO) applyAmmoCache();
    else if (kind == AP_ITEM_CLIP) applyClip();
    else if (kind == AP_ITEM_ALERT) applyAlert();
    else if (kind == AP_ITEM_JAM) applyJam();
    else if (kind == AP_ITEM_SLIP) applySlip();
    else if (kind == AP_ITEM_BARON) return apTrapBaron();
    else if (kind == AP_ITEM_GREASY) apTrapGreasy();
    else if (kind == AP_ITEM_RESTART) return applyRestart();
    else if (kind == AP_ITEM_TINY) applyTiny();
    else if (kind == AP_ITEM_FLAG) applyFlag();
    else if (kind == AP_ITEM_BORIS) return apTrapBoris();
    else if (kind == AP_ITEM_DK) applyDk();
    else if (kind == AP_ITEM_PAINT) applyPaint();
    else if (kind == AP_ITEM_BULLET) applyBullet();
    return 1;
}

static int consumable(int id)
{
    return id == AP_ITEM_ARMOR || id == AP_ITEM_AMMO || id == AP_ITEM_CLIP ||
           id == AP_ITEM_ALERT || id == AP_ITEM_JAM || id == AP_ITEM_SLIP ||
           id == AP_ITEM_BARON || id == AP_ITEM_GREASY ||
           id == AP_ITEM_RESTART || id == AP_ITEM_TINY || id == AP_ITEM_FLAG ||
           id == AP_ITEM_BORIS || id == AP_ITEM_DK || id == AP_ITEM_PAINT || id == AP_ITEM_BULLET;
}

static void queueConsumable(int id)
{
    if (id == AP_ITEM_ARMOR) sPendArmor++;
    else if (id == AP_ITEM_AMMO) sPendAmmo++;
    else if (id == AP_ITEM_CLIP) sPendClip++;
    else if (id == AP_ITEM_ALERT) sPendAlert++;
    else if (id == AP_ITEM_JAM) sPendJam++;
    else if (id == AP_ITEM_SLIP) sPendSlip++;
    else if (id == AP_ITEM_BARON) sPendBaron++;
    else if (id == AP_ITEM_GREASY) sPendGreasy++;
    else if (id == AP_ITEM_RESTART) sPendRestart++;
    else if (id == AP_ITEM_TINY) sPendTiny++;
    else if (id == AP_ITEM_FLAG) sPendFlag++;
    else if (id == AP_ITEM_BORIS) sPendBoris++;
    else if (id == AP_ITEM_DK) sPendDk++;
    else if (id == AP_ITEM_PAINT) sPendPaint++;
    else if (id == AP_ITEM_BULLET) sPendBullet++;
}

static void flushPending(void)
{
    while (sPendAlert > 0)
    {
        applyAlert();
        sPendAlert--;
        notify("Received Camera Alert", 0);
    }
    while (sPendJam > 0)
    {
        applyJam();
        sPendJam--;
        notify("Received Jammed Clip", 0);
    }
    while (sPendSlip > 0)
    {
        applySlip();
        sPendSlip--;
        notify("Received Slipped on the Ice", 0);
    }
    while (sPendBaron > 0)
    {
        sPendBaron--;
        if (applyKind(AP_ITEM_BARON))
        {
            notify("Received Baron Samedi", 0);
        }
        else
        {
            notify("Baron Samedi had no room", 1);
        }
    }
    while (sPendGreasy > 0)
    {
        apTrapGreasy();
        sPendGreasy--;
        notify("Received Greasy Hands", 0);
    }
    while (sPendRestart > 0)
    {
        sPendRestart--;
        if (applyRestart())
        {
            notify("Received Restart Mission", 0);
        }
    }
    while (sPendTiny > 0)
    {
        applyTiny();
        sPendTiny--;
        notify("Received Tiny Bond", 0);
    }
    while (sPendFlag > 0)
    {
        applyFlag();
        sPendFlag--;
        notify("Received The Living Daylights", 0);
    }
    while (sPendBoris > 0)
    {
        sPendBoris--;
        if (apTrapBoris())
        {
            notify("Received I am invisible", 0);
        }
        else
        {
            notify("Boris had no room", 1);
        }
    }
    while (sPendDk > 0)
    {
        applyDk();
        sPendDk--;
        notify("Received DK Mode", 0);
    }
    while (sPendPaint > 0)
    {
        applyPaint();
        sPendPaint--;
        notify("Received Paintball", 0);
    }
    while (sPendBullet > 0)
    {
        applyBullet();
        sPendBullet--;
        notify("Received Single Bullet", 0);
    }
#ifdef GE_AP_DEBUG
    if (sPendDeath > 0 && g_CurrentPlayer != NULL && !g_CurrentPlayer->bonddead)
    {
        extern void bondviewKillCurrentPlayer(void);

        sPendDeath--;
        sDeathFromLink = 1;
        bondviewKillCurrentPlayer();
        sDeathFromLink = 0;
        notify("Debug: Death Link", 1);
    }
#endif
    while (sPendClip > 0)
    {
        applyClip();
        sPendClip--;
        notify("Received Ammo Clip", 0);
    }
}

static void sideEffect(int id, int player, int flags)
{
    char name[64];
    char line[120];
    const char *from;
    int index;

    rememberId(id);
    itemName(id, name, (int) sizeof(name));
    from = (player > 0 && player < 64 && player != sMySlot) ? sNames[player] : NULL;
    if (from != NULL && from[0] != '\0')
    {
        snprintf(line, sizeof(line), "Received %s from %s", name, from);
    }
    else
    {
        snprintf(line, sizeof(line), "Received %s", name);
    }

    if (id == AP_ITEM_ARMOR)
    {
        sArmorStock++;
        saveState();
        toastItem(line, flags);
        return;
    }
    if (id == AP_ITEM_AMMO)
    {
        sAmmoStock++;
        saveState();
        toastItem(line, flags);
        return;
    }
    if (id >= AP_ITEM_WEAPON(0) && id < AP_ITEM_WEAPON(AP_WEAPON_COUNT))
    {
        index = id - AP_ITEM_WEAPON(0);
        if (inMission())
        {
            grantWeapon(index);
        }
        toastItem(line, flags);
        return;
    }
    if (id >= AP_ITEM_TOOL(0) && id < AP_ITEM_TOOL(AP_TOOL_COUNT))
    {
        if (inMission())
        {
            bondinvAddInvItem(kToolId[id - AP_ITEM_TOOL(0)]);
        }
        toastItem(line, flags);
        return;
    }
    if (consumable(id))
    {
        if (inMission() && sSettle >= 30)
        {
            if (applyKind(id))
            {
                toastItem(line, flags);
            }
            else if (id == AP_ITEM_BARON)
            {
                notify("Baron Samedi had no room", 1);
            }
            else if (id == AP_ITEM_BORIS)
            {
                notify("Boris had no room", 1);
            }
            else
            {
                toastItem(line, flags);
            }
        }
        else
        {
            queueConsumable(id);
            toastItem(line, flags);
        }
        return;
    }
    toastItem(line, flags);
}

static int maskCount(int mask)
{
    int n = 0;
    int d;
    for (d = 0; d < AP_DIFF_COUNT; d++)
    {
        if (mask & (1 << d))
        {
            n++;
        }
    }
    return n;
}

static int goalNeedMask(void)
{
    if (sGoalAll)
    {
        return sCheckMask;
    }
    if (sGoalDiff < 0 || sGoalDiff >= AP_DIFF_COUNT)
    {
        return 1;
    }
    return 1 << sGoalDiff;
}

static int goalMet(void)
{
    int i;
    int need = goalNeedMask();
    if (sGoal == AP_GOAL_CAMPAIGN)
    {
        for (i = 0; i < AP_CAMPAIGN_COUNT; i++)
        {
            if ((sChecked[i] & need) != need)
            {
                return 0;
            }
        }
        return 1;
    }
    i = 17;
    if (sGoal == AP_GOAL_AZTEC) i = 18;
    if (sGoal == AP_GOAL_EGYPTIAN) i = 19;
    return (sChecked[i] & need) == need;
}

static int jsonEscape(const char *s, char *out, int cap)
{
    int n = 0;
    if (s == NULL)
    {
        s = "";
    }
    while (*s != '\0' && n < cap - 2)
    {
        unsigned char c = (unsigned char) *s++;
        if (c == '"' || c == '\\')
        {
            if (n + 2 >= cap)
            {
                break;
            }
            out[n++] = '\\';
            out[n++] = (char) c;
        }
        else if (c >= 0x20)
        {
            out[n++] = (char) c;
        }
    }
    out[n] = '\0';
    return n;
}

static int sendRaw(const char *text)
{
    if (!apNetIsOpen() || text == NULL)
    {
        return 0;
    }
    return apNetSend(text, (int) strlen(text));
}

/* The server's item cursor moved past ours. Ask for the whole list again
 * until a ReceivedItems with index 0 replaces it. */
static void sendSync(void)
{
    if (!sLive || !sNeedSync || !apNetIsOpen())
    {
        return;
    }
    if (time(NULL) < sSyncNext)
    {
        return;
    }
    sSyncNext = time(NULL) + 2;
    sendRaw("[{\"cmd\":\"Sync\"}]");
}

static void sendConnect(void)
{
    char pw[160];
    char name[96];
    char packet[768];

    jsonEscape(sPassword, pw, (int) sizeof(pw));
    jsonEscape(sSlot, name, (int) sizeof(name));
    snprintf(packet, sizeof(packet),
             "[{\"cmd\":\"Connect\",\"password\":\"%s\",\"game\":\"%s\",\"name\":\"%s\","
             "\"uuid\":\"%s\",\"version\":{\"major\":%d,\"minor\":%d,\"build\":%d,\"class\":\"Version\"},"
             "\"items_handling\":7,\"tags\":[\"AP\"],\"slot_data\":true}]",
             pw, AP_GAME_NAME, name, sUuid, sVerMaj, sVerMin, sVerBld);
    if (sendRaw(packet))
    {
        sConnectSent = 1;
    }
}

/* Archipelago ClientStatus. 20 is playing. 30 is the goal, not "in game". */
#define AP_STATUS_PLAYING 20
#define AP_STATUS_GOAL 30

static void sendStatus(int status)
{
    char packet[80];
    snprintf(packet, sizeof(packet), "[{\"cmd\":\"StatusUpdate\",\"status\":%d}]", status);
    sendRaw(packet);
}

static void sendChecks(void)
{
    char packet[4096];
    int m, d, slot, n, first, c;

    if (!sLive || !sResend)
    {
        return;
    }
    n = snprintf(packet, sizeof(packet), "[{\"cmd\":\"LocationChecks\",\"locations\":[");
    first = 1;
    for (m = 0; m < AP_MISSION_COUNT && n < (int) sizeof(packet) - 32; m++)
    {
        for (d = 0; d < AP_DIFF_COUNT && n < (int) sizeof(packet) - 32; d++)
        {
            if (sChecked[m] & (1u << d))
            {
                n += snprintf(packet + n, sizeof(packet) - (size_t) n, "%s%d", first ? "" : ",", AP_LOC(m, d));
                first = 0;
            }
        }
        for (d = 0; d < 4 && n < (int) sizeof(packet) - 32; d++)
        {
            for (slot = 0; slot < AP_OBJ_SLOTS && n < (int) sizeof(packet) - 32; slot++)
            {
                if (sObjChecked[m][d] & (1u << slot))
                {
                    n += snprintf(packet + n, sizeof(packet) - (size_t) n, "%s%d", first ? "" : ",", AP_OBJ(m, d, slot));
                    first = 0;
                }
            }
        }
    }
    for (c = 0; c < AP_CHEAT_COUNT && n < (int) sizeof(packet) - 32; c++)
    {
        if (sCheatChecked & (1u << c))
        {
            n += snprintf(packet + n, sizeof(packet) - (size_t) n, "%s%d", first ? "" : ",", AP_CHEAT(c));
            first = 0;
        }
    }
    if (first)
    {
        sResend = 0;
        return;
    }
    snprintf(packet + n, sizeof(packet) - (size_t) n, "]}]");
    if (sendRaw(packet))
    {
        sResend = 0;
    }
}

static void announceGoal(void)
{
    if (!sLive || sGoalAnnounced || !goalMet())
    {
        return;
    }
    sendStatus(AP_STATUS_GOAL);
    sGoalAnnounced = 1;
    sGoalJustMet = 1;
    notify("Goal complete", 1);
}

/* Leave *out on the value of want. 1 when the key exists. */
static int findKeyWalk(const char *start, const char *end, const char *want, ApJson *out)
{
    ApJson j;
    int started = 0;
    char key[48];

    apJsonInit(&j, start, (int) (end - start));
    for (;;)
    {
        int step = apJsonObjectNext(&j, key, (int) sizeof(key), &started);
        if (step == 0)
        {
            return 0;
        }
        if (step < 0)
        {
            return 0;
        }
        if (strcmp(key, want) == 0)
        {
            *out = j;
            return 1;
        }
        if (!apJsonSkip(&j))
        {
            return 0;
        }
    }
}

static int readScalar(ApJson *j, int *out)
{
    ApJson k = *j;
    const char *p;
    while (k.p < k.end && (*k.p == ' ' || *k.p == '\t' || *k.p == '\n' || *k.p == '\r'))
    {
        k.p++;
    }
    p = k.p;
    if (k.end - p >= 4 && memcmp(p, "true", 4) == 0)
    {
        *out = 1;
        return 1;
    }
    if (k.end - p >= 5 && memcmp(p, "false", 5) == 0)
    {
        *out = 0;
        return 1;
    }
    return apJsonInt(j, out);
}

/* Choice options are stored as ints. A name is accepted too, in the same order as the world. */
static int readChoice(ApJson *j, int *out, const char *const *names, int nnames)
{
    ApJson copy = *j;
    char s[32];
    int i;

    if (apJsonString(&copy, s, (int) sizeof(s)))
    {
        for (i = 0; i < nnames; i++)
        {
            if (names[i] != NULL && eqI(s, names[i]))
            {
                *out = i;
                return 1;
            }
        }
        if (eqI(s, "true") || eqI(s, "on"))
        {
            *out = 1;
            return 1;
        }
        if (eqI(s, "false") || eqI(s, "off"))
        {
            *out = 0;
            return 1;
        }
        return 0;
    }
    return readScalar(j, out);
}

static void resetProgress(void)
{
    memset(sChecked, 0, sizeof(sChecked));
    memset(sObjChecked, 0, sizeof(sObjChecked));
    sCheatChecked = 0;
    sItemIndex = 0;
    sOwnedCount = 0;
    memset(sDossierDiff, 0, sizeof(sDossierDiff));
    sWeapons = 0;
    sTools = 0;
    sArmorStock = 0;
    sAmmoStock = 0;
    sPendArmor = sPendAmmo = sPendAlert = sPendJam = sPendSlip = sPendClip = sPendBaron = sPendGreasy = 0;
    sPendRestart = sPendTiny = sPendFlag = 0;
    sPendBoris = sPendDk = sPendPaint = sPendBullet = 0;
    dropTimed(1);
    sJamUntil = 0;
#ifdef GE_AP_DEBUG
    sPendDeath = 0;
#endif
    sGoalAnnounced = 0;
    sNeedSync = 0;
}

static void noteCheck(int id)
{
    int rel, mission, slot, diff;

    if (id >= 643000)
    {
        rel = id - 643000;
        if (rel >= 0 && rel < AP_CHEAT_COUNT)
        {
            sCheatChecked |= 1u << rel;
        }
        return;
    }
    if (id >= 642000)
    {
        rel = id - 642000;
        mission = rel / 40;
        rel = rel % 40;
        slot = rel % 10;
        diff = rel / 10;
        if (mission >= 0 && mission < AP_MISSION_COUNT && diff >= 0 && diff < 4 && slot >= 0 && slot < AP_OBJ_SLOTS)
        {
            sObjChecked[mission][diff] |= (unsigned short) (1u << slot);
        }
        return;
    }
    rel = id - 641000;
    mission = rel / 10;
    slot = rel % 10;
    if (mission >= 0 && mission < AP_MISSION_COUNT && slot >= 0 && slot < AP_DIFF_COUNT)
    {
        sChecked[mission] |= (unsigned char) (1u << slot);
    }
}

static void loadState(void)
{
    FILE *f;
    char line[1024];

    f = portRootFopen("Data/Saves/archipelago.txt", "r");
    if (f == NULL)
    {
        return;
    }
    while (fgets(line, (int) sizeof(line), f) != NULL)
    {
        char *eq = strchr(line, '=');
        if (eq == NULL)
        {
            continue;
        }
        *eq = '\0';
        trim(line);
        trim(eq + 1);
        if (eqI(line, "uuid") && eq[1] != '\0')
        {
            snprintf(sUuid, sizeof(sUuid), "%s", eq + 1);
        }
        else if (eqI(line, "seed"))
        {
            snprintf(sSeed, sizeof(sSeed), "%s", eq + 1);
            if (sCartSeed[0] == '\0')
            {
                snprintf(sCartSeed, sizeof(sCartSeed), "%s", sSeed);
            }
        }
        else if (eqI(line, "index"))
        {
            sItemIndex = atoi(eq + 1);
            if (sItemIndex < 0)
            {
                sItemIndex = 0;
            }
        }
        else if (eqI(line, "armor"))
        {
            sArmorStock = atoi(eq + 1);
            if (sArmorStock < 0) sArmorStock = 0;
        }
        else if (eqI(line, "ammo"))
        {
            sAmmoStock = atoi(eq + 1);
            if (sAmmoStock < 0) sAmmoStock = 0;
        }
        else if (eqI(line, "checked"))
        {
            char *tok = eq + 1;
            while (*tok != '\0')
            {
                int id = atoi(tok);
                noteCheck(id);
                while (*tok != '\0' && *tok != ',')
                {
                    tok++;
                }
                if (*tok == ',')
                {
                    tok++;
                }
            }
        }
    }
    fclose(f);
}

static void makeUuid(void)
{
    unsigned int a = (unsigned int) time(NULL) ^ (unsigned int) (uintptr_t) &sUuid;
    unsigned int b = a * 1664525u + 1013904223u;
    unsigned int c = b * 1664525u + 1013904223u;
    unsigned int d = c * 1664525u + 1013904223u;
    snprintf(sUuid, sizeof(sUuid), "%08x-%04x-4%03x-%04x-%08x%04x",
             a, b & 0xffffu, c & 0x0fffu, (unsigned) (((d >> 16) & 0x3fffu) | 0x8000u), c, b & 0xffffu);
}

static void normalizeHost(void)
{
    char *p = sHost;
    char *colon;
    if (strncmp(p, "wss://", 6) == 0)
    {
        sTls = 1;
        memmove(p, p + 6, strlen(p + 6) + 1);
    }
    else if (strncmp(p, "ws://", 5) == 0)
    {
        sTls = 0;
        memmove(p, p + 5, strlen(p + 5) + 1);
    }
    colon = strrchr(p, ':');
    if (colon != NULL && strchr(p, ':') == colon)
    {
        int port = atoi(colon + 1);
        if (port > 0 && port <= 65535)
        {
            sPort = port;
            *colon = '\0';
        }
    }
}

static void loadIni(void)
{
    FILE *f;
    char line[512];
    int in = 0;

    f = portRootFopen("midas.ini", "r");
    if (f == NULL)
    {
        return;
    }
    while (fgets(line, (int) sizeof(line), f) != NULL)
    {
        char *eq;
        trim(line);
        if (line[0] == '[')
        {
            in = eqI(line, "[archipelago]");
            continue;
        }
        if (!in || line[0] == '\0' || line[0] == ';' || line[0] == '#')
        {
            continue;
        }
        eq = strchr(line, '=');
        if (eq == NULL)
        {
            continue;
        }
        *eq = '\0';
        trim(line);
        trim(eq + 1);
        if (eqI(line, "enabled")) sEnabled = truth(eq + 1);
        else if (eqI(line, "host") && eq[1] != '\0') snprintf(sHost, sizeof(sHost), "%s", eq + 1);
        else if (eqI(line, "port")) sPort = atoi(eq + 1);
        else if (eqI(line, "slot")) snprintf(sSlot, sizeof(sSlot), "%s", eq + 1);
        else if (eqI(line, "password")) snprintf(sPassword, sizeof(sPassword), "%s", eq + 1);
        else if (eqI(line, "tls")) sTls = truth(eq + 1);
        else if (eqI(line, "popup")) apPopupSetEnabled(truth(eq + 1));
        else if (eqI(line, "popup_pos")) apPopupSetPosName(eq + 1);
        else if (eqI(line, "armor_key") && eq[1] != '\0') snprintf(sArmorKey, sizeof(sArmorKey), "%s", eq + 1);
        else if (eqI(line, "ammo_key") && eq[1] != '\0') snprintf(sAmmoKey, sizeof(sAmmoKey), "%s", eq + 1);
        else if (eqI(line, "death_link"))
        {
            if (eqI(eq + 1, "on") || eqI(eq + 1, "1")) sDeathChoice = 1;
            else if (eqI(eq + 1, "off") || eqI(eq + 1, "0")) sDeathChoice = 0;
            else sDeathChoice = -1;
        }
    }
    fclose(f);
    if (sPort < 1 || sPort > 65535)
    {
        sPort = 38281;
    }
    normalizeHost();
}

static void saveIni(void)
{
    char en[8], port[16], tls[8], pop[8], death[8];
    const char *keys[11] = {
        "enabled", "host", "port", "slot", "password", "tls", "popup", "popup_pos",
        "armor_key", "ammo_key", "death_link"
    };
    const char *vals[11];

    snprintf(en, sizeof(en), "%d", sEnabled ? 1 : 0);
    snprintf(port, sizeof(port), "%d", sPort);
    snprintf(tls, sizeof(tls), "%d", sTls ? 1 : 0);
    snprintf(pop, sizeof(pop), "%d", apGetPopup() ? 1 : 0);
    vals[0] = en;
    vals[1] = sHost;
    vals[2] = port;
    vals[3] = sSlot;
    vals[4] = sPassword;
    vals[5] = tls;
    vals[6] = pop;
    vals[7] = apPopupPosName();
    vals[8] = sArmorKey;
    vals[9] = sAmmoKey;
    snprintf(death, sizeof(death), "%s", sDeathChoice < 0 ? "seed" : (sDeathChoice ? "on" : "off"));
    vals[10] = death;
    portIniWriteStrs("[archipelago]", keys, vals, 11);
}

static int keyIsNone(const char *s)
{
    return s == NULL || s[0] == '\0' || eqI(s, "none") || eqI(s, "-") || eqI(s, "unbound");
}

static void resolveOneKey(char *name, int n, int *sc, const char *fallback)
{
    int code;
    const char *label;

    if (keyIsNone(name))
    {
        snprintf(name, (size_t) n, "None");
        *sc = 0;
        return;
    }
    code = portVideoScancodeFromName(name);
    if (code <= 0)
    {
        snprintf(name, (size_t) n, "%s", fallback);
        code = portVideoScancodeFromName(name);
    }
    if (code <= 0)
    {
        snprintf(name, (size_t) n, "None");
        *sc = 0;
        return;
    }
    label = portVideoScancodeName(code);
    if (label == NULL || label[0] == '\0')
    {
        *sc = code;
        return;
    }
    *sc = code;
    snprintf(name, (size_t) n, "%s", label);
}

static void resolveSupplyKeys(void)
{
    resolveOneKey(sArmorKey, (int) sizeof(sArmorKey), &sArmorSc, "F5");
    resolveOneKey(sAmmoKey, (int) sizeof(sAmmoKey), &sAmmoSc, "F6");
    if (sArmorSc != 0 && sArmorSc == sAmmoSc)
    {
        snprintf(sAmmoKey, sizeof(sAmmoKey), "None");
        sAmmoSc = 0;
    }
}

static void assignSupplyKey(int which, int sc)
{
    char *name = (which == 0) ? sArmorKey : sAmmoKey;
    int *slot = (which == 0) ? &sArmorSc : &sAmmoSc;
    int *other = (which == 0) ? &sAmmoSc : &sArmorSc;
    char *otherName = (which == 0) ? sAmmoKey : sArmorKey;

    if (sc <= 0 || sc >= AP_SC_MAX)
    {
        snprintf(name, sizeof(sArmorKey), "None");
        *slot = 0;
    }
    else
    {
        const char *label = portVideoScancodeName(sc);

        if (label == NULL || label[0] == '\0')
        {
            snprintf(name, sizeof(sArmorKey), "None");
            *slot = 0;
        }
        else
        {
            snprintf(name, sizeof(sArmorKey), "%s", label);
            *slot = sc;
        }
    }
    if (*slot != 0 && *slot == *other)
    {
        snprintf(otherName, sizeof(sArmorKey), "None");
        *other = 0;
    }
    saveIni();
}

static int sWantConnect;

static void apKick(void)
{
    sEnabled = 1;
    sWantConnect = 1;
    sHold = 0;
    sWaitLog = 0;
    sNextTry = 0;
    sConnectSent = 0;
    sLive = 0;
    saveIni();
    apNetStop();
}

static void ensureInit(void)
{
    if (sInited)
    {
        return;
    }
    sInited = 1;
    loadIni();
    resolveSupplyKeys();
    loadState();
    if (sUuid[0] == '\0')
    {
        makeUuid();
        saveState();
    }
}

/* Cart file for the connected seed. Characters that cannot be a file name
 * become underscores. The four folders of one seed stay out of the next. */
static void cartSavePath(char *out, int n)
{
    char safe[96];
    int i;
    int j = 0;

    for (i = 0; sSeed[i] != '\0' && j < (int) sizeof(safe) - 1; i++)
    {
        char c = sSeed[i];
        int ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z') || c == '-' || c == '_';
        safe[j++] = ok ? c : '_';
    }
    safe[j] = '\0';
    snprintf(out, (size_t) n, "Data/Saves/ap/%s.bin", safe);
}

/* The cart everyone already has belongs to the seed that was saved with it.
 * A seed that has no file yet, and is not that one, starts empty. */
static void migrateCart(const char *dest)
{
    FILE *in;
    FILE *out;
    unsigned char buf[512];
    size_t n;
    extern void portEepromDirs(void);
    extern FILE *portRootFopen(const char *name, const char *mode);

    if (sCartSeed[0] == '\0' || strcmp(sSeed, sCartSeed) != 0)
    {
        return;
    }
    in = portRootFopen(dest, "rb");
    if (in != NULL)
    {
        fclose(in);
        return;
    }
    portEepromDirs();
    in = portRootFopen("Data/Saves/eeprom.bin", "rb");
    if (in == NULL)
    {
        in = portRootFopen("eeprom.bin", "rb");
    }
    if (in == NULL)
    {
        return;
    }
    n = fread(buf, 1, sizeof(buf), in);
    fclose(in);
    if (n == 0)
    {
        return;
    }
    out = portRootFopen(dest, "wb");
    if (out == NULL)
    {
        return;
    }
    fwrite(buf, 1, n, out);
    fclose(out);
    portConLogf("ge: ap: kept the current folders for seed %s\n", sSeed);
}

/* 1 when the cart file changed and the folders must be read again. */
static int bindCart(void)
{
    char path[160];
    extern int portEepromSelect(const char *rel);

    ensureInit();
    if (!sEnabled || sSeed[0] == '\0')
    {
        return portEepromSelect(NULL);
    }
    cartSavePath(path, (int) sizeof(path));
    migrateCart(path);
    return portEepromSelect(path);
}

void apBindCartSave(void)
{
    bindCart();
}

static void noteSeed(const char *seed)
{
    extern void fileValidateSaves(void);

    if (seed == NULL || seed[0] == '\0')
    {
        return;
    }
    if (sSeed[0] != '\0' && strcmp(sSeed, seed) != 0)
    {
        portConLogf("ge: ap: new seed, local checks were reset\n");
        resetProgress();
    }
    snprintf(sSeed, sizeof(sSeed), "%s", seed);
    apEnemySetSeed(sSeed);
    saveState();
    /* Boot already read whichever file was current. Read again only when
     * this seed points somewhere else, including the first connect. */
    if (bindCart())
    {
        fileValidateSaves();
    }
}

static void onRoom(const char *obj, const char *end)
{
    ApJson field;
    char seed[128];
    if (findKeyWalk(obj, end, "version", &field))
    {
        ApJson n;
        const char *vend = end;
        if (findKeyWalk(field.p, vend, "major", &n)) apJsonInt(&n, &sVerMaj);
        if (findKeyWalk(field.p, vend, "minor", &n)) apJsonInt(&n, &sVerMin);
        if (findKeyWalk(field.p, vend, "build", &n)) apJsonInt(&n, &sVerBld);
    }
    seed[0] = '\0';
    if (findKeyWalk(obj, end, "seed_name", &field))
    {
        int num = 0;
        seed[0] = '\0';
        if (!apJsonString(&field, seed, (int) sizeof(seed)) && apJsonInt(&field, &num))
        {
            snprintf(seed, sizeof(seed), "%d", num);
        }
        noteSeed(seed);
    }
    if (!sConnectSent && sSlot[0] != '\0')
    {
        sendConnect();
    }
}

static void onRefused(const char *obj, const char *end)
{
    ApJson field;
    char err[64];
    int started = 0;

    sConnectSent = 0;
    if (!findKeyWalk(obj, end, "errors", &field))
    {
        notify("Connection refused", 0);
        sHold = 1;
        apNetStop();
        return;
    }
    for (;;)
    {
        int step = apJsonArrayNext(&field, &started);
        if (step == 0)
        {
            break;
        }
        if (step < 0 || !apJsonString(&field, err, (int) sizeof(err)))
        {
            break;
        }
        portConLogf("ge: ap: connection refused (%s)\n", err);
        sHold = 1;
    }
    apNetStop();
}

static void takeChecked(const char *obj, const char *end, const char *key)
{
    ApJson field;
    int started = 0;
    if (!findKeyWalk(obj, end, key, &field))
    {
        return;
    }
    for (;;)
    {
        int step = apJsonArrayNext(&field, &started);
        int id;
        if (step == 0)
        {
            break;
        }
        if (step < 0 || !apJsonInt(&field, &id))
        {
            break;
        }
        noteCheck(id);
    }
}

static void takePlayers(const char *obj, const char *end)
{
    ApJson field;
    int started = 0;

    memset(sNames, 0, sizeof(sNames));
    if (!findKeyWalk(obj, end, "players", &field))
    {
        return;
    }
    for (;;)
    {
        int step = apJsonArrayNext(&field, &started);
        const char *one;
        ApJson scan, slotj, namej;
        int slot = 0;
        char name[32];
        if (step == 0)
        {
            break;
        }
        if (step < 0)
        {
            break;
        }
        one = field.p;
        scan = field;
        if (!apJsonSkip(&scan))
        {
            break;
        }
        name[0] = '\0';
        if (findKeyWalk(one, scan.p, "slot", &slotj))
        {
            apJsonInt(&slotj, &slot);
        }
        if (findKeyWalk(one, scan.p, "alias", &namej))
        {
            apJsonString(&namej, name, (int) sizeof(name));
        }
        if (name[0] == '\0' && findKeyWalk(one, scan.p, "name", &namej))
        {
            apJsonString(&namej, name, (int) sizeof(name));
        }
        if (slot > 0 && slot < 64)
        {
            snprintf(sNames[slot], sizeof(sNames[slot]), "%s", name);
        }
        field.p = scan.p;
    }
}

static void takeSlotGames(const char *obj, const char *end);

static void onConnected(const char *obj, const char *end)
{
    ApJson field;
    char line[96];

    if (findKeyWalk(obj, end, "slot", &field))
    {
        apJsonInt(&field, &sMySlot);
    }
    sCheatChecks = 0;
    sDeathLink = 0;
    if (findKeyWalk(obj, end, "slot_data", &field))
    {
        ApJson n;
        int v;
        if (findKeyWalk(field.p, end, "goal", &n))
        {
            static const char *const names[] = { "cradle", "aztec", "egyptian", "campaign" };
            if (readChoice(&n, &v, names, 4)) sGoal = v;
        }
        sGoalAll = 0;
        sCheckMask = 7;
        sSplitDossiers = 0;
        sBonus = 1;
        if (findKeyWalk(field.p, end, "goal_difficulty", &n))
        {
            static const char *const names[] = { "agent", "secret_agent", "00_agent", "all" };
            if (readChoice(&n, &v, names, 4))
            {
                if (v == 3)
                {
                    sGoalAll = 1;
                }
                else
                {
                    sGoalDiff = v;
                }
            }
        }
        if (findKeyWalk(field.p, end, "difficulty_checks", &n))
        {
            static const char *const names[] = { "all", "agent_only", "secret_agent", "00_agent" };
            if (readChoice(&n, &v, names, 4))
            {
                sChecks = v;
                if (v == 1) sCheckMask = 1;
                else if (v == 2) sCheckMask = 2;
                else if (v == 3) sCheckMask = 4;
                else sCheckMask = 7;
            }
        }
        if (findKeyWalk(field.p, end, "difficulty_dossiers", &n) && readChoice(&n, &v, NULL, 0))
        {
            sSplitDossiers = v ? 1 : 0;
        }
        if (findKeyWalk(field.p, end, "include_bonus", &n) && readChoice(&n, &v, NULL, 0)) sBonus = v;
        if (findKeyWalk(field.p, end, "objective_checks", &n) && readChoice(&n, &v, NULL, 0)) sObjectiveChecks = v ? 1 : 0;
        if (findKeyWalk(field.p, end, "cheat_checks", &n) && readChoice(&n, &v, NULL, 0)) sCheatChecks = v ? 1 : 0;
        if (findKeyWalk(field.p, end, "global_weapons", &n) && readChoice(&n, &v, NULL, 0)) sGlobalWeapons = v ? 1 : 0;
        if (findKeyWalk(field.p, end, "mission_tools", &n) && readChoice(&n, &v, NULL, 0)) sMissionTools = v ? 1 : 0;
        if (findKeyWalk(field.p, end, "death_link", &n) && readChoice(&n, &v, NULL, 0)) sDeathLink = v ? 1 : 0;
        {
            int skins = 0;
            int heads = 0;
            int weapons = 0;
            int lethal = 0;
            int accuracy = 0;
            int music = 0;
            static const char *const modeNames[] = { "off", "grouped", "chaotic" };

            if (findKeyWalk(field.p, end, "enemy_skins", &n) && readChoice(&n, &v, modeNames, 3)) skins = v;
            if (findKeyWalk(field.p, end, "enemy_heads", &n) && readChoice(&n, &v, modeNames, 3)) heads = v;
            if (findKeyWalk(field.p, end, "enemy_weapons", &n) && readChoice(&n, &v, modeNames, 3)) weapons = v;
            if (findKeyWalk(field.p, end, "allow_lethal_weapons", &n) && readChoice(&n, &v, NULL, 0)) lethal = v ? 1 : 0;
            if (findKeyWalk(field.p, end, "enemy_accuracy", &n) && readChoice(&n, &v, modeNames, 3)) accuracy = v;
            if (findKeyWalk(field.p, end, "mission_music", &n) && readChoice(&n, &v, NULL, 0)) music = v ? 1 : 0;
            apEnemySetOptions(skins, heads, weapons, accuracy, music, lethal);
        }
    }
    if (!sGoalAll && (sGoalDiff < 0 || sGoalDiff >= AP_DIFF_COUNT))
    {
        sGoalDiff = 0;
    }
    if (sChecks < 0 || sChecks > 3)
    {
        sChecks = AP_CHECKS_ALL;
        sCheckMask = 7;
    }
    takePlayers(obj, end);
    sPackN = 0;
    sPackNext = 0;
    sPackWait = 0;
    takeSlotGames(obj, end);
    /* The room is the source of truth. A leftover local file must not
     * re-check the goal the moment the socket opens. */
    memset(sChecked, 0, sizeof(sChecked));
    memset(sObjChecked, 0, sizeof(sObjChecked));
    sCheatChecked = 0;
    takeChecked(obj, end, "checked_locations");
    sLive = 1;
    sNeedSync = 0;
    sResend = 1;
    saveState();
    snprintf(line, sizeof(line), "Connected as %s", sSlot);
    notify(line, 0);
    if (!sPlayingSent)
    {
        sendStatus(AP_STATUS_PLAYING);
        sPlayingSent = 1;
    }
    if (goalMet() && !sGoalAnnounced)
    {
        sendStatus(AP_STATUS_GOAL);
        sGoalAnnounced = 1;
    }
    /* Cradle was already marked clear before the debrief could see the
     * change, so Magnum never fired. A cradle clear already in the room
     * still earns it. Laser and Golden Gun follow the same rule. */
    if (sCheatChecks)
    {
        int d;
        for (d = 0; d < AP_DIFF_COUNT; d++)
        {
            if (sChecked[17] & (1u << d))
            {
                sCheatChecked |= 1u << AP_CHEAT_MAGNUM;
                break;
            }
        }
        if ((sBonus || sGoal == AP_GOAL_AZTEC)
            && (sChecked[18] & ((1u << 1) | (1u << 2))))
        {
            sCheatChecked |= 1u << AP_CHEAT_LASER;
        }
        if ((sBonus || sGoal == AP_GOAL_EGYPTIAN) && (sChecked[19] & (1u << 2)))
        {
            sCheatChecked |= 1u << AP_CHEAT_GOLDEN_GUN;
        }
    }
    sendChecks();
    if (sDeathChoice >= 0)
    {
        sDeathLink = sDeathChoice ? 1 : 0;
    }
    if (sDeathLink)
    {
        sendRaw("[{\"cmd\":\"ConnectUpdate\",\"tags\":[\"AP\",\"DeathLink\"]}]");
    }
    else if (sDeathChoice == 0)
    {
        sendRaw("[{\"cmd\":\"ConnectUpdate\",\"tags\":[\"AP\"]}]");
    }
}

static int isDossierId(int id)
{
    return (id >= AP_ITEM_DOSSIER(0) && id < AP_ITEM_DOSSIER(AP_MISSION_COUNT))
        || (id >= AP_ITEM_DIFFDOSSIER(0, 0) && id < AP_ITEM_DIFFDOSSIER(AP_MISSION_COUNT, 0));
}

static int isWeaponId(int id)
{
    return id >= AP_ITEM_WEAPON(0) && id < AP_ITEM_WEAPON(AP_WEAPON_COUNT);
}

/* Same classes as item_kind in the world. Weapons are useful. The Golden Gun
 * is progression when Egyptian is in the seed and the weapon inventory is
 * shared, because that clear needs the item. */
static int goldenGunProgression(void)
{
    return sGlobalWeapons && (sBonus || sGoal == AP_GOAL_EGYPTIAN);
}

static int itemFlags(int id)
{
    if (isDossierId(id))
    {
        return AP_POP_PROGRESSION;
    }
    if (id >= AP_ITEM_TOOL(0) && id < AP_ITEM_TOOL(AP_TOOL_COUNT))
    {
        return AP_POP_PROGRESSION;
    }
    if (id >= AP_ITEM_ALERT && id <= AP_ITEM_BULLET)
    {
        return AP_POP_TRAP;
    }
    /* Body Armor and Ammo Cache are useful finds. Ammo Clip stays filler:
     * it only refills the gun already in hand. */
    if (id == AP_ITEM_ARMOR || id == AP_ITEM_AMMO)
    {
        return AP_POP_USEFUL;
    }
    if (id == AP_ITEM_WEAPON(15) && goldenGunProgression())
    {
        return AP_POP_PROGRESSION;
    }
    if (isWeaponId(id))
    {
        return AP_POP_USEFUL;
    }
    return 0;
}

static void pushReceived(const char *name, int flags)
{
    char line[96];

    snprintf(line, sizeof(line), "Received %s", name);
    portConLogf("ge: ap: %s\n", line);
    apPopupPush(line, 1, flags);
}

/* Precollected items sit at the front of the inventory: the starting
 * dossiers, then the starting gun when the seed has one. A saved index
 * used to skip them on the next connection. */
static void announceStart(const int *ids, int count)
{
    char name[64];
    int i = 0;
    int dossiers = 0;
    int weapon = -1;
    int shown;

    while (i < count && isDossierId(ids[i]))
    {
        dossiers++;
        i++;
    }
    if (i < count && isWeaponId(ids[i]))
    {
        weapon = ids[i];
    }
    if (dossiers == 0 && weapon < 0)
    {
        return;
    }
    /* Four toasts. The gun keeps the last slot when the dossiers do not fit. */
    shown = (weapon >= 0 && dossiers > 3) ? 2 : dossiers;
    for (i = 0; i < shown; i++)
    {
        itemName(ids[i], name, (int) sizeof(name));
        pushReceived(name, itemFlags(ids[i]));
    }
    if (shown < dossiers)
    {
        portConLogf("ge: ap: Received ...\n");
        apPopupPush("Received ...", 1, AP_POP_PROGRESSION);
    }
    if (weapon >= 0)
    {
        itemName(weapon, name, (int) sizeof(name));
        pushReceived(name, itemFlags(weapon));
    }
}

static void acceptItems(const char *obj, const char *end)
{
    ApJson field;
    int index = -1;
    int started = 0;
    static int batch[AP_ITEM_CAP];
    static int owners[AP_ITEM_CAP];
    static int batchFlags[AP_ITEM_CAP];
    int n = 0;
    int dropped = 0;
    int i;
    int old;

    if (!findKeyWalk(obj, end, "index", &field) || !apJsonInt(&field, &index))
    {
        return;
    }
    if (!findKeyWalk(obj, end, "items", &field))
    {
        return;
    }
    for (;;)
    {
        int step = apJsonArrayNext(&field, &started);
        const char *one;
        ApJson scan, num;
        int id = 0;
        int player = 0;
        int flags = 0;
        int sawFlags = 0;
        if (step == 0)
        {
            break;
        }
        if (step < 0)
        {
            break;
        }
        one = field.p;
        scan = field;
        if (!apJsonSkip(&scan))
        {
            break;
        }
        if (findKeyWalk(one, scan.p, "item", &num))
        {
            apJsonInt(&num, &id);
        }
        if (findKeyWalk(one, scan.p, "player", &num))
        {
            apJsonInt(&num, &player);
        }
        if (findKeyWalk(one, scan.p, "flags", &num) && apJsonInt(&num, &flags))
        {
            sawFlags = 1;
        }
        if (isWeaponId(id) || id == AP_ITEM_ARMOR || id == AP_ITEM_AMMO)
        {
            /* Local class wins over an old seed that still marked these filler. */
            flags = itemFlags(id);
        }
        else if (!sawFlags)
        {
            flags = itemFlags(id);
        }
        if (n < AP_ITEM_CAP)
        {
            batch[n] = id;
            owners[n] = player;
            batchFlags[n] = flags;
            n++;
        }
        else
        {
            dropped++;
        }
        field.p = scan.p;
    }
    if (dropped > 0)
    {
        portConLogf("ge: ap: %d received items did not fit the local list\n", dropped);
    }
    if (index == 0)
    {
        old = sItemIndex;
        if (n > AP_ITEM_CAP)
        {
            n = AP_ITEM_CAP;
        }
        if (n < old)
        {
            old = n;
        }
        sOwnedCount = n;
        for (i = 0; i < n; i++)
        {
            sOwned[i] = batch[i];
        }
        rebuildOwned();
        sQuietNotify = (old == 0);
        for (i = old; i < n; i++)
        {
            sideEffect(sOwned[i], owners[i], batchFlags[i]);
        }
        sQuietNotify = 0;
        announceStart(sOwned, n);
        sItemIndex = n;
        sNeedSync = 0;
        saveState();
        announceGoal();
        return;
    }
    if (index != sItemIndex)
    {
        if (!sNeedSync)
        {
            sNeedSync = 1;
            sSyncNext = 0;
            sResend = 1;
            portConLogf("ge: ap: item index %d did not match local %d, asking for a sync\n", index, sItemIndex);
        }
        sendSync();
        return;
    }
    for (i = 0; i < n; i++)
    {
        if (sOwnedCount < AP_ITEM_CAP)
        {
            sOwned[sOwnedCount++] = batch[i];
        }
        sideEffect(batch[i], owners[i], batchFlags[i]);
    }
    sItemIndex += n;
    sNeedSync = 0;
    saveState();
    announceGoal();
}

static void appendBit(char *text, int *n, int cap, const char *bit)
{
    int k;

    if (bit == NULL)
    {
        return;
    }
    for (k = 0; bit[k] != '\0' && *n < cap - 1; k++)
    {
        unsigned char c = (unsigned char) bit[k];
        if (c < 0x20 || c >= 0x7F)
        {
            c = ' ';
        }
        text[(*n)++] = (char) c;
    }
    text[*n] = '\0';
}

/* PrintJSON only carries numeric ids. Other games' names live in the
 * datapackage, requested once per game in the room after Connected.
 * Our own ids are still named locally, so a toast works before the
 * package arrives. */
#define AP_NAME_MAX 8192
#define AP_NAME_LEN 48

typedef struct ApName
{
    int id;
    char name[AP_NAME_LEN];
} ApName;

static ApName *sItemNames;
static int sItemNameCount;
static ApName *sLocNames;
static int sLocNameCount;
static char sPackGame[16][64];

static void rememberName(ApName **tab, int *count, int id, const char *name)
{
    ApName *grown;
    int i;

    if (name == NULL || name[0] == '\0')
    {
        return;
    }
    for (i = 0; i < *count; i++)
    {
        if ((*tab)[i].id == id)
        {
            return;
        }
    }
    if (*count >= AP_NAME_MAX)
    {
        return;
    }
    if (*tab == NULL || (*count % 256) == 0)
    {
        grown = (ApName *) realloc(*tab, (size_t) (*count + 256) * sizeof(ApName));
        if (grown == NULL)
        {
            return;
        }
        *tab = grown;
    }
    (*tab)[*count].id = id;
    snprintf((*tab)[*count].name, AP_NAME_LEN, "%s", name);
    (*count)++;
}

static const char *findName(ApName *tab, int count, int id)
{
    int i;

    for (i = 0; i < count; i++)
    {
        if (tab[i].id == id)
        {
            return tab[i].name;
        }
    }
    return NULL;
}

static void notePackGame(const char *name)
{
    int i;

    if (name == NULL || name[0] == '\0')
    {
        return;
    }
    for (i = 0; i < sPackN; i++)
    {
        if (eqI(sPackGame[i], name))
        {
            return;
        }
    }
    if (sPackN >= (int) (sizeof(sPackGame) / sizeof(sPackGame[0])))
    {
        return;
    }
    snprintf(sPackGame[sPackN], sizeof(sPackGame[sPackN]), "%s", name);
    sPackN++;
}

static void ingestNameMap(ApJson *map, const char *end, int locations)
{
    int started = 0;

    (void) end;
    for (;;)
    {
        char name[AP_NAME_LEN];
        int step;
        int id;

        step = apJsonObjectNext(map, name, (int) sizeof(name), &started);
        if (step == 0)
        {
            return;
        }
        if (step < 0)
        {
            return;
        }
        if (!apJsonInt(map, &id))
        {
            if (!apJsonSkip(map))
            {
                return;
            }
            continue;
        }
        if (locations)
        {
            rememberName(&sLocNames, &sLocNameCount, id, name);
        }
        else
        {
            rememberName(&sItemNames, &sItemNameCount, id, name);
        }
    }
}

static void onDataPackage(const char *obj, const char *end)
{
    ApJson data, games;
    int started = 0;

    if (!findKeyWalk(obj, end, "data", &data))
    {
        return;
    }
    if (!findKeyWalk(data.p, data.end, "games", &games))
    {
        return;
    }
    for (;;)
    {
        char game[64];
        ApJson body, items;
        ApJson skip;
        int step;

        step = apJsonObjectNext(&games, game, (int) sizeof(game), &started);
        if (step == 0)
        {
            break;
        }
        if (step < 0)
        {
            break;
        }
        body = games;
        skip = games;
        if (!apJsonSkip(&skip))
        {
            break;
        }
        if (findKeyWalk(body.p, skip.p, "item_name_to_id", &items))
        {
            ingestNameMap(&items, skip.p, 0);
        }
        if (findKeyWalk(body.p, skip.p, "location_name_to_id", &items))
        {
            ingestNameMap(&items, skip.p, 1);
        }
        games.p = skip.p;
    }
    if (sPackNext < sPackN)
    {
        sPackNext++;
    }
    sPackWait = 0;
    portConLogf("ge: ap: names %d items, %d locations\n", sItemNameCount, sLocNameCount);
}

static void askPackage(void)
{
    char esc[160];
    char packet[320];

    if (!sLive || !apNetIsOpen() || sPackNext >= sPackN)
    {
        return;
    }
    if (sPackWait != 0 && time(NULL) < sPackWait)
    {
        return;
    }
    if (sPackWait != 0)
    {
        /* That game's package never arrived. Skip it and ask for the next. */
        sPackNext++;
        sPackWait = 0;
        if (sPackNext >= sPackN)
        {
            return;
        }
    }
    jsonEscape(sPackGame[sPackNext], esc, (int) sizeof(esc));
    snprintf(packet, sizeof(packet),
             "[{\"cmd\":\"GetDataPackage\",\"games\":[\"%s\"]}]", esc);
    if (sendRaw(packet))
    {
        sPackWait = (int) time(NULL) + 8;
    }
}

static void takeSlotGames(const char *obj, const char *end)
{
    ApJson field;
    int started = 0;

    if (!findKeyWalk(obj, end, "slot_info", &field))
    {
        return;
    }
    for (;;)
    {
        char key[16];
        char game[64];
        ApJson gamej, skip;
        int step;

        step = apJsonObjectNext(&field, key, (int) sizeof(key), &started);
        if (step == 0)
        {
            return;
        }
        if (step < 0)
        {
            return;
        }
        skip = field;
        if (!apJsonSkip(&skip))
        {
            return;
        }
        game[0] = '\0';
        if (findKeyWalk(field.p, skip.p, "game", &gamej))
        {
            apJsonString(&gamej, game, (int) sizeof(game));
        }
        notePackGame(game);
        field.p = skip.p;
    }
}

static void partLabel(const char *one, const char *end, char *buf, int n)
{
    ApJson tj, typej;
    char type[24];
    char raw[80];
    int id;

    buf[0] = '\0';
    type[0] = '\0';
    raw[0] = '\0';
    if (findKeyWalk(one, end, "type", &typej))
    {
        apJsonString(&typej, type, (int) sizeof(type));
    }
    if (!findKeyWalk(one, end, "text", &tj) || !apJsonString(&tj, raw, (int) sizeof(raw)))
    {
        return;
    }
    if (eqI(type, "player_id"))
    {
        int slot = atoi(raw);
        if (slot > 0 && slot < 64 && sNames[slot][0] != '\0')
        {
            snprintf(buf, (size_t) n, "%s", sNames[slot]);
        }
        else if (slot == sMySlot && sSlot[0] != '\0')
        {
            snprintf(buf, (size_t) n, "%s", sSlot);
        }
        else
        {
            snprintf(buf, (size_t) n, "Player %d", slot);
        }
        return;
    }
    if (eqI(type, "item_id") || eqI(type, "item_name"))
    {
        const char *known;

        id = atoi(raw);
        if (id >= AP_ITEM_DOSSIER(0))
        {
            itemName(id, buf, n);
            if (buf[0] != '\0' && strncmp(buf, "Item ", 5) != 0)
            {
                return;
            }
        }
        known = findName(sItemNames, sItemNameCount, id);
        if (known == NULL && id == 0)
        {
            known = raw;
        }
        if (known != NULL && known[0] != '\0' && (known[0] < '0' || known[0] > '9'))
        {
            snprintf(buf, (size_t) n, "%s", known);
            return;
        }
        if (known != NULL)
        {
            snprintf(buf, (size_t) n, "%s", known);
            return;
        }
        if (eqI(type, "item_name"))
        {
            snprintf(buf, (size_t) n, "%s", raw);
            return;
        }
        snprintf(buf, (size_t) n, "item %d", id);
        return;
    }
    if (eqI(type, "location_id") || eqI(type, "location_name"))
    {
        int rel, mission, diff;
        const char *known;

        id = atoi(raw);
        rel = id - 641000;
        mission = rel / 10;
        diff = rel % 10;
        if (id < 0)
        {
            snprintf(buf, (size_t) n, "the server");
        }
        else if (mission >= 0 && mission < AP_MISSION_COUNT && diff >= 0 && diff < AP_DIFF_COUNT
                 && id >= 641000 && id < 641000 + AP_MISSION_COUNT * 10)
        {
            locLabel(mission, diff, buf, n);
        }
        else if ((known = findName(sLocNames, sLocNameCount, id)) != NULL)
        {
            snprintf(buf, (size_t) n, "%s", known);
        }
        else if (eqI(type, "location_name") || (raw[0] != '\0' && (raw[0] < '0' || raw[0] > '9')))
        {
            snprintf(buf, (size_t) n, "%s", raw);
        }
        else
        {
            snprintf(buf, (size_t) n, "location");
        }
        return;
    }
    snprintf(buf, (size_t) n, "%s", raw);
}

static int partFinder(const char *one, const char *end)
{
    ApJson typej, tj;
    char type[24];
    char raw[48];
    int i;

    type[0] = '\0';
    if (!findKeyWalk(one, end, "type", &typej) || !apJsonString(&typej, type, (int) sizeof(type)))
    {
        return -1;
    }
    if (!eqI(type, "player_id") && !eqI(type, "player_name"))
    {
        return -1;
    }
    if (!findKeyWalk(one, end, "text", &tj) || !apJsonString(&tj, raw, (int) sizeof(raw)))
    {
        return -1;
    }
    if (eqI(type, "player_id"))
    {
        return atoi(raw);
    }
    if (sSlot[0] != '\0' && eqI(raw, sSlot))
    {
        return sMySlot;
    }
    for (i = 1; i < 64; i++)
    {
        if (sNames[i][0] != '\0' && eqI(raw, sNames[i]))
        {
            return i;
        }
    }
    return 0;
}

static int partItemFlags(const char *one, const char *end)
{
    ApJson typej, fj, tj;
    char type[24];
    char raw[48];
    int flags = 0;
    int id;

    type[0] = '\0';
    if (!findKeyWalk(one, end, "type", &typej) || !apJsonString(&typej, type, (int) sizeof(type)))
    {
        return -1;
    }
    if (!eqI(type, "item_id") && !eqI(type, "item_name"))
    {
        return -1;
    }
    if (findKeyWalk(one, end, "text", &tj) && apJsonString(&tj, raw, (int) sizeof(raw)))
    {
        id = atoi(raw);
        if (isWeaponId(id) || id >= AP_ITEM_DOSSIER(0))
        {
            return itemFlags(id);
        }
    }
    if (findKeyWalk(one, end, "flags", &fj) && apJsonInt(&fj, &flags))
    {
        return flags;
    }
    return 0;
}

static void onPrint(const char *obj, const char *end)
{
    ApJson field, typej;
    int started = 0;
    int receiving = -1;
    int finder = -1;
    char kind[24];
    char text[180];
    int n = 0;
    int sentFlags = -1;

    kind[0] = '\0';
    text[0] = '\0';
    if (findKeyWalk(obj, end, "type", &typej))
    {
        apJsonString(&typej, kind, (int) sizeof(kind));
    }
    if (findKeyWalk(obj, end, "receiving", &field))
    {
        apJsonInt(&field, &receiving);
    }
    if (!findKeyWalk(obj, end, "data", &field))
    {
        return;
    }
    for (;;)
    {
        int step = apJsonArrayNext(&field, &started);
        const char *one;
        ApJson scan;
        char bit[80];
        int who;
        int got;
        if (step == 0)
        {
            break;
        }
        if (step < 0)
        {
            break;
        }
        one = field.p;
        scan = field;
        if (!apJsonSkip(&scan))
        {
            break;
        }
        who = partFinder(one, scan.p);
        if (finder < 0 && who >= 0)
        {
            finder = who;
        }
        got = partItemFlags(one, scan.p);
        if (sentFlags < 0 && got >= 0)
        {
            sentFlags = got;
        }
        partLabel(one, scan.p, bit, (int) sizeof(bit));
        appendBit(text, &n, (int) sizeof(text), bit);
        field.p = scan.p;
    }
    if (text[0] == '\0')
    {
        return;
    }
    portConLogf("ge: ap: %s\n", text);
    /* A find of our own that left the slot. Our receipts already toast from the item list. */
    if ((eqI(kind, "ItemSend") || eqI(kind, "ItemCheat"))
        && finder == sMySlot && receiving > 0 && receiving != sMySlot)
    {
        apPopupPush(text, 1, sentFlags);
    }
}

static int readJsonNumber(ApJson *j, char *buf, int cap)
{
    const char *start;
    int n;

    if (j == NULL || buf == NULL || cap < 2)
    {
        return 0;
    }
    while (j->p < j->end && (*j->p == ' ' || *j->p == '\t' || *j->p == '\n' || *j->p == '\r'))
    {
        j->p++;
    }
    start = j->p;
    if (j->p < j->end && (*j->p == '-' || *j->p == '+'))
    {
        j->p++;
    }
    while (j->p < j->end && *j->p >= '0' && *j->p <= '9')
    {
        j->p++;
    }
    if (j->p < j->end && *j->p == '.')
    {
        j->p++;
        while (j->p < j->end && *j->p >= '0' && *j->p <= '9')
        {
            j->p++;
        }
    }
    if (j->p < j->end && (*j->p == 'e' || *j->p == 'E'))
    {
        j->p++;
        if (j->p < j->end && (*j->p == '+' || *j->p == '-'))
        {
            j->p++;
        }
        while (j->p < j->end && *j->p >= '0' && *j->p <= '9')
        {
            j->p++;
        }
    }
    n = (int) (j->p - start);
    if (n <= 0)
    {
        return 0;
    }
    if (n >= cap)
    {
        n = cap - 1;
    }
    memcpy(buf, start, (size_t) n);
    buf[n] = '\0';
    return 1;
}

static void sendDeathLink(const char *cause)
{
    char name[96];
    char esc[180];
    char packet[480];

    if (!sDeathLink || !sLive || sDeathFromLink || !apNetIsOpen())
    {
        return;
    }
    snprintf(sLastDeathTime, sizeof(sLastDeathTime), "%lld%03d",
             (long long) time(NULL), sDeathSeq++ % 1000);
    jsonEscape(sSlot, name, (int) sizeof(name));
    jsonEscape(cause != NULL ? cause : "Bond died", esc, (int) sizeof(esc));
    snprintf(packet, sizeof(packet),
             "[{\"cmd\":\"Bounce\",\"tags\":[\"DeathLink\"],\"data\":{\"time\":%s,\"source\":\"%s\",\"cause\":\"%s\"}}]",
             sLastDeathTime, name, esc);
    if (sendRaw(packet))
    {
        portConLogf("ge: ap: death link sent (%s)\n", cause != NULL ? cause : "");
    }
}

void apOnBondDeath(void)
{
    char cause[96];

    if (apSurvivalActive())
    {
        return;
    }
    if (!sDeathLink || !sLive || sDeathFromLink || !inMission())
    {
        return;
    }
    snprintf(cause, sizeof(cause), "%s died", sSlot[0] != '\0' ? sSlot : "Bond");
    sendDeathLink(cause);
}

static int packetHasTag(const char *obj, const char *end, const char *tag)
{
    ApJson tags;
    int started = 0;

    if (!findKeyWalk(obj, end, "tags", &tags))
    {
        return 0;
    }
    for (;;)
    {
        int step = apJsonArrayNext(&tags, &started);
        char bit[32];

        if (step <= 0)
        {
            return 0;
        }
        bit[0] = '\0';
        if (apJsonString(&tags, bit, (int) sizeof(bit)) && eqI(bit, tag))
        {
            return 1;
        }
    }
}

static void onBounced(const char *obj, const char *end)
{
    ApJson data;
    int started = 0;
    char when[48];
    char source[96];
    char cause[120];

    when[0] = '\0';
    source[0] = '\0';
    cause[0] = '\0';
    if (!sDeathLink || !packetHasTag(obj, end, "DeathLink"))
    {
        return;
    }
    if (!findKeyWalk(obj, end, "data", &data))
    {
        return;
    }
    for (;;)
    {
        char key[24];
        int step = apJsonObjectNext(&data, key, (int) sizeof(key), &started);

        if (step == 0)
        {
            break;
        }
        if (step < 0)
        {
            return;
        }
        if (eqI(key, "time"))
        {
            if (!readJsonNumber(&data, when, (int) sizeof(when)))
            {
                apJsonSkip(&data);
            }
        }
        else if (eqI(key, "source"))
        {
            apJsonString(&data, source, (int) sizeof(source));
        }
        else if (eqI(key, "cause"))
        {
            apJsonString(&data, cause, (int) sizeof(cause));
        }
        else if (!apJsonSkip(&data))
        {
            return;
        }
    }
    if (when[0] != '\0' && strcmp(when, sLastDeathTime) == 0)
    {
        return;
    }
    if (source[0] != '\0' && strcmp(source, sSlot) == 0)
    {
        return;
    }
    if (cause[0] != '\0')
    {
        snprintf(sDeathToast, sizeof(sDeathToast), "DeathLink: %.72s", cause);
    }
    else if (source[0] != '\0')
    {
        snprintf(sDeathToast, sizeof(sDeathToast), "DeathLink from %.64s", source);
    }
    else
    {
        snprintf(sDeathToast, sizeof(sDeathToast), "DeathLink");
    }
    sDeathQueued = 1;
    portConLogf("ge: ap: %s\n", sDeathToast);
}

static void applyQueuedDeath(void)
{
    extern void bondviewKillCurrentPlayer(void);

    if (!sDeathQueued || !sDeathLink || !inMission() || sSettle < 30 || g_CurrentPlayer == NULL)
    {
        return;
    }
    if (g_CurrentPlayer->bonddead || g_CurrentPlayer->cheatBondInvincible)
    {
        if (!g_CurrentPlayer->cheatBondInvincible)
        {
            apPopupPush(sDeathToast, 1, AP_POP_NOLOGO);
        }
        sDeathQueued = 0;
        return;
    }
    sDeathFromLink = 1;
    bondviewKillCurrentPlayer();
    sDeathFromLink = 0;
    sDeathQueued = 0;
    if (g_CurrentPlayer->bonddead)
    {
        apPopupPush(sDeathToast, 1, AP_POP_NOLOGO);
    }
}

static void watchNatalya(void)
{
    extern ChrRecord *g_ChrSlots;
    extern s32 g_NumChrSlots;
    int mission;
    int i;
    int alive = 0;
    int dying = 0;

    if (!sDeathLink || !inMission() || sSettle < 30 || g_ChrSlots == NULL)
    {
        sNatalyaSeen = 0;
        sNatalyaLatched = 0;
        sNatalyaMission = -2;
        return;
    }
    mission = missionIndex(bossGetStageNum());
    if (mission != sNatalyaMission)
    {
        sNatalyaMission = mission;
        sNatalyaSeen = 0;
        sNatalyaLatched = 0;
    }
    for (i = 0; i < g_NumChrSlots; i++)
    {
        ChrRecord *chr = &g_ChrSlots[i];

        if (chr->model == NULL)
        {
            continue;
        }
        if (chr->bodynum != BODY_Natalya_Skirt && chr->bodynum != BODY_Natalya_Jungle_Fatigues)
        {
            continue;
        }
        if (chr->actiontype == ACT_DIE)
        {
            dying = 1;
        }
        else if (chr->actiontype != ACT_DEAD)
        {
            alive = 1;
        }
    }
    if (dying)
    {
        if (sNatalyaSeen && !sNatalyaLatched)
        {
            char cause[96];

            snprintf(cause, sizeof(cause), "%s lost Natalya", sSlot[0] != '\0' ? sSlot : "Bond");
            sendDeathLink(cause);
            sNatalyaLatched = 1;
        }
    }
    else
    {
        sNatalyaLatched = 0;
    }
    if (alive)
    {
        sNatalyaSeen = 1;
    }
    else if (!dying)
    {
        sNatalyaSeen = 0;
    }
}

static void handleMessage(const char *text, int len)
{
    ApJson j;
    int started = 0;

    apJsonInit(&j, text, len);
    for (;;)
    {
        int step = apJsonArrayNext(&j, &started);
        const char *obj;
        ApJson scan, cmdj;
        char cmd[32];
        if (step == 0)
        {
            break;
        }
        if (step < 0)
        {
            portConLogf("ge: ap: ignored a malformed packet\n");
            break;
        }
        obj = j.p;
        scan = j;
        if (!apJsonSkip(&scan))
        {
            break;
        }
        cmd[0] = '\0';
        if (findKeyWalk(obj, scan.p, "cmd", &cmdj))
        {
            apJsonString(&cmdj, cmd, (int) sizeof(cmd));
        }
        if (eqI(cmd, "RoomInfo")) onRoom(obj, scan.p);
        else if (eqI(cmd, "Connected")) onConnected(obj, scan.p);
        else if (eqI(cmd, "ConnectionRefused")) onRefused(obj, scan.p);
        else if (eqI(cmd, "ReceivedItems")) acceptItems(obj, scan.p);
        else if (eqI(cmd, "PrintJSON")) onPrint(obj, scan.p);
        else if (eqI(cmd, "DataPackage")) onDataPackage(obj, scan.p);
        else if (eqI(cmd, "Bounced")) onBounced(obj, scan.p);
        j = scan;
    }
}

static int diffIndex(int difficulty)
{
    if (difficulty == DIFFICULTY_AGENT) return 0;
    if (difficulty == DIFFICULTY_SECRET) return 1;
    if (difficulty == DIFFICULTY_00 || difficulty == DIFFICULTY_007) return 2;
    return -1;
}

static int ownsDossier(int mission, int diff)
{
    if (mission < 0 || mission >= AP_MISSION_COUNT || diff < 0 || diff >= AP_DIFF_COUNT)
    {
        return 0;
    }
    return (sDossierDiff[diff] & (1u << mission)) != 0;
}

static int anyDossier(int mission)
{
    int diff;
    for (diff = 0; diff < AP_DIFF_COUNT; diff++)
    {
        if (ownsDossier(mission, diff))
        {
            return 1;
        }
    }
    return 0;
}

static void copyArg(char *dst, int n, const char *src)
{
    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }
    snprintf(dst, (size_t) n, "%s", src);
}

void apTick(void)
{
    char status[180];
    int open;
    int n;

    ensureInit();
    apDiscordTick();
    apSurvivalTick();
    apPopupTick();
    trapMaintain();
    askPackage();
    if (sEnabled && sMsg == NULL)
    {
        sMsg = (char *) malloc(AP_MSG_CAP);
        if (sMsg == NULL)
        {
            if (!sWaitLog)
            {
                sWaitLog = 1;
                portConLogf("ge: ap: out of memory\n");
            }
            return;
        }
    }
    if (sMsg == NULL)
    {
        return;
    }
    while (apNetTakeStatus(status, (int) sizeof(status)))
    {
        portConLogf("ge: ap: %s\n", status);
    }
    while ((n = apNetPoll(sMsg, AP_MSG_CAP)) != 0)
    {
        if (n > 0)
        {
            handleMessage(sMsg, n);
        }
    }
    if (apNetTakeLoss())
    {
        if (!sNeedSync)
        {
            sNeedSync = 1;
            sSyncNext = 0;
            sResend = 1;
            portConLogf("ge: ap: an item message was dropped, asking for a sync\n");
        }
    }
    if (sNeedSync)
    {
        sendSync();
    }
    open = apNetIsOpen();
    if (sWasOpen && !open)
    {
        sLive = 0;
        sConnectSent = 0;
        sNeedSync = 0;
        sPlayingSent = 0;
        sGoalAnnounced = 0;
        sDeathLink = 0;
        sDeathQueued = 0;
        sNatalyaSeen = 0;
        sNatalyaLatched = 0;
        apEnemySetOptions(0, 0, 0, 0, 0, 0);
        sCheatChecks = 0;
        portConLogf("ge: ap: disconnected\n");
    }
    sWasOpen = open;

    if (!sEnabled || !sWantConnect)
    {
        if (open || apNetBusy())
        {
            apNetStop();
        }
        return;
    }
    if (sSlot[0] == '\0')
    {
        if (!sWaitLog)
        {
            sWaitLog = 1;
            portConLogf("ge: ap: set your slot name (`ap slot Name` or midas.ini [archipelago])\n");
        }
        return;
    }
    if (!sLive && !sWaitLog)
    {
        sWaitLog = 1;
        portConLogf("ge: ap: waiting for the server; missions stay locked\n");
    }
    if (sLive)
    {
        sWaitLog = 0;
    }
    if (sHold)
    {
        return;
    }
    if (!open)
    {
        if (!apNetBusy() && time(NULL) >= sNextTry)
        {
            int rc = apNetStart(sHost, sPort, sTls);
            sNextTry = time(NULL) + 5;
            if (rc < 0)
            {
                sHold = 1;
            }
        }
        return;
    }
    sendChecks();
    announceGoal();
    if (inMission())
    {
        if (sSettle < 30)
        {
            sSettle++;
        }
        else
        {
            flushPending();
            applyQueuedDeath();
            watchNatalya();
        }
    }
    else
    {
        sSettle = 0;
        /* A jam deadline belongs to the mission clock. The parked remainder
         * is sJamLeft, so clearing the deadline here does not end the trap. */
        sJamUntil = 0;
        sNatalyaSeen = 0;
        sNatalyaLatched = 0;
        sNatalyaMission = -2;
    }
}

static int weaponIndex(int weaponnum)
{
    int i;
    for (i = 0; i < AP_WEAPON_COUNT; i++)
    {
        if (kWeaponId[i] == (ITEM_IDS) weaponnum)
        {
            return i;
        }
    }
    return -1;
}

static int weaponOwned(int weaponnum)
{
    int i = weaponIndex(weaponnum);
    return i >= 0 && (sWeapons & (1u << i)) != 0;
}

/* Tools the cart's normal intro hands out. Global weapons can skip that
 * list; Mission Tools on still starts the stage with them. */
static void grantKitItem(int itemId)
{
    WeaponStats *st;
    int type;
    int mag;

    if (g_CurrentPlayer == NULL || itemId < 0)
    {
        return;
    }
    weaponLoadProjectileModels((ITEM_IDS) itemId);
    if (!bondinvHasInvItem((ITEM_IDS) itemId))
    {
        bondinvAddInvItem((ITEM_IDS) itemId);
    }
    st = get_ptr_item_statistics((ITEM_IDS) itemId);
    if (st == NULL)
    {
        return;
    }
    type = st->AmmoType;
    mag = st->MagSize;
    if (type <= 0 || type >= 30 || mag <= 0)
    {
        return;
    }
    if (g_CurrentPlayer->ammoheldarr[type] < mag)
    {
        g_CurrentPlayer->ammoheldarr[type] = mag;
    }
}

static void giveMissionKit(void)
{
    static const struct
    {
        s8 mission;
        s16 item;
    } kit[] = {
        { 0, ITEM_BUG },
        { 1, ITEM_REMOTEMINE },
        { 1, ITEM_TRIGGER },
        /* Control destroys the armored mainframes with the remote mines the
         * cart gives at the door. Global weapons blocks that intro item until
         * Remote Mine has been found, so the kit lends them for this mission
         * only. They are not written into the shared inventory. */
        { 15, ITEM_REMOTEMINE },
        { 15, ITEM_TRIGGER },
        { 4, ITEM_KEYANALYSERCASE },
        { 4, ITEM_CAMERA },
        { 4, ITEM_DATATHIEF },
        { 5, ITEM_CAMERA },
        { 5, ITEM_PLASTIQUE },
        { 6, ITEM_BOMBDEFUSER },
        { 6, ITEM_BUG },
        { 8, ITEM_WATCHMAGNETATTRACT },
        { 10, ITEM_WATCHMAGNETATTRACT },
        { 13, ITEM_WATCHLASER },
        { 16, ITEM_TIMEDMINE },
        { 18, ITEM_EXPLOSIVEFLOPPY },
    };
    int mission;
    int i;

    mission = missionIndex((int) bossGetStageNum());
    for (i = 0; i < (int) (sizeof(kit) / sizeof(kit[0])); i++)
    {
        if (kit[i].mission == mission)
        {
            grantKitItem(kit[i].item);
        }
    }
}

void apApplyLoadout(void)
{
    int i;
    int first = -1;

    ensureInit();
    sGoalJustMet = 0;
    if (!inMission())
    {
        apSurvivalOnSpawn();
        return;
    }
    for (i = 0; i < AP_WEAPON_COUNT; i++)
    {
        if ((sWeapons & (1u << i)) == 0)
        {
            continue;
        }
        grantWeapon(i);
        if (first < 0 && i < AP_GUN_COUNT)
        {
            first = i;
        }
    }
    if (first >= 0 &&
        (starting_weapon[GUNRIGHT] == ITEM_FIST || starting_weapon[GUNRIGHT] == ITEM_UNARMED
         || (sGlobalWeapons && !weaponOwned(starting_weapon[GUNRIGHT]))))
    {
        starting_weapon[GUNRIGHT] = kWeaponId[first];
    }
    for (i = 0; i < AP_TOOL_COUNT; i++)
    {
        if (sTools & (1u << i))
        {
            weaponLoadProjectileModels(kToolId[i]);
            bondinvAddInvItem(kToolId[i]);
        }
    }
    if (sGlobalWeapons)
    {
        giveBaseMagazines();
    }
    /* Owned guns always get the cart's starting reserve. With a shared
     * inventory every firearm type does, so a gun received later in the
     * mission is not empty. A higher amount already in the reserve,
     * such as a scripted start, is left as it is. */
    giveStartingAmmo(sGlobalWeapons);
    if (sMissionTools)
    {
        giveMissionKit();
    }
    apSurvivalOnSpawn();
}

int apGlobalWeapons(void)
{
    ensureInit();
    return sGlobalWeapons && sLive;
}

int apSupplyCount(int which)
{
    ensureInit();
    return which == 0 ? sArmorStock : sAmmoStock;
}

int apUseSupply(int which)
{
    ensureInit();
    if (g_CurrentPlayer == NULL)
    {
        return 0;
    }
    if (which == 0)
    {
        if (sArmorStock <= 0)
        {
            return 0;
        }
        if (g_CurrentPlayer->bondarmour > 0.95f)
        {
            notify("Armor already on", 1);
            return 0;
        }
        sArmorStock--;
        applyArmor();
        sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, ARMOUR_COLLECT_SFX, 0);
        saveState();
        notify("Used Body Armor", 1);
        return 1;
    }
    if (sAmmoStock <= 0)
    {
        return 0;
    }
    sAmmoStock--;
    applyAmmoCache();
    sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, PICKUP_AMMO_SFX, 0);
    saveState();
    notify("Used Ammo Cache", 1);
    return 1;
}

int apSupplyHudOn(void)
{
    ensureInit();
    if (!sEnabled && sArmorStock <= 0 && sAmmoStock <= 0)
    {
        return 0;
    }
    if (g_CurrentPlayer == NULL || getPlayerCount() != 1)
    {
        return 0;
    }
    if (lvlGetSelectedDifficulty() == DIFFICULTY_MULTI)
    {
        return 0;
    }
    return missionIndex((int) bossGetStageNum()) >= 0;
}

const char *apSupplyKeyLabel(int which)
{
    static char label[2][40];
    int i = (which == 0) ? 0 : 1;

    ensureInit();
    if (sKeyCap == i)
    {
        return "Press a Key\n";
    }
    snprintf(label[i], sizeof(label[i]), "%s\n", i == 0 ? sArmorKey : sAmmoKey);
    return label[i];
}

int apSupplyCaptureActive(void)
{
    return sKeyCap >= 0;
}

int apSupplyCaptureWhich(void)
{
    return sKeyCap;
}

void apSupplyCaptureBegin(int which)
{
    ensureInit();
    sKeyCap = (which == 0) ? 0 : 1;
}

void apSupplyCaptureCancel(void)
{
    sKeyCap = -1;
}

int apSupplyKeyDown(int scancode, int repeat)
{
    ensureInit();
    if (sKeyCap >= 0)
    {
        if (repeat)
        {
            return 1;
        }
        if (scancode == AP_SC_ESCAPE)
        {
            sKeyCap = -1;
            sSwallowSc = scancode;
            return 1;
        }
        if (scancode == AP_SC_BACKSPACE || scancode == AP_SC_DELETE)
        {
            assignSupplyKey(sKeyCap, 0);
            sKeyCap = -1;
            sSwallowSc = scancode;
            return 1;
        }
        assignSupplyKey(sKeyCap, scancode);
        sKeyCap = -1;
        sSwallowSc = scancode;
        return 1;
    }
    if (repeat || !apSupplyHudOn())
    {
        return 0;
    }
    if (sArmorSc != 0 && scancode == sArmorSc)
    {
        sSwallowSc = scancode;
        apUseSupply(0);
        return 1;
    }
    if (sAmmoSc != 0 && scancode == sAmmoSc)
    {
        sSwallowSc = scancode;
        apUseSupply(1);
        return 1;
    }
    return 0;
}

int apSupplyScancode(int which)
{
    ensureInit();
    return which == 0 ? sArmorSc : sAmmoSc;
}

int apSupplySwallow(void)
{
    return sSwallowSc;
}

void apSupplyClearSwallow(void)
{
    sSwallowSc = 0;
}

/* Silver and Gold PP7 are the Train and Cradle cheats. They are not weapons
 * in the pool, so a guard drop must not add them. */
static int cheatGun(int weaponnum)
{
    return weaponnum == ITEM_SILVERWPPK || weaponnum == ITEM_GOLDWPPK;
}

int apBlockLooseWeapon(int weaponnum)
{
    ensureInit();
    if (!sGlobalWeapons || !sLive)
    {
        return 0;
    }
    if (cheatGun(weaponnum))
    {
        return bondinvHasInvItem((ITEM_IDS) weaponnum) ? 0 : 1;
    }
    if (weaponIndex(weaponnum) < 0)
    {
        return 0;
    }
    return weaponOwned(weaponnum) ? 0 : 1;
}

int apAllowIntroItem(int itemId)
{
    int index;

    ensureInit();
    if (!sGlobalWeapons || !sLive)
    {
        return 1;
    }
    if (cheatGun(itemId) && !bondinvHasInvItem((ITEM_IDS) itemId))
    {
        return 0;
    }
    index = weaponIndex(itemId);
    /* Mission kit stays unless the seed turned Mission Tools off. That option
     * is the hard one: no watch, camera, datathief, plastique, or start mines.
     * Holdable guns are withheld either way until the inventory has them. */
    if (sMissionTools && (index < 0 || index >= AP_GUN_COUNT))
    {
        return 1;
    }
    if (index >= 0 && index < AP_GUN_COUNT && weaponOwned(itemId))
    {
        return 1;
    }
    return 0;
}

int apEquipIntroItem(int itemId)
{
    int index;

    ensureInit();
    if (!sGlobalWeapons || !sLive)
    {
        return 1;
    }
    index = weaponIndex(itemId);
    if (index < 0 || index >= AP_GUN_COUNT)
    {
        return 0;
    }
    return weaponOwned(itemId) ? 1 : 0;
}

void apOnObjectiveComplete(int objectiveIndex)
{
    int mission;
    int band;
    char line[96];

    ensureInit();
    if (apSurvivalActive())
    {
        return;
    }
    if (!sEnabled || !sLive || !sObjectiveChecks)
    {
        return;
    }
    if (g_AppendCheatSinglePlayer)
    {
        return;
    }
    if (objectiveIndex < 0 || objectiveIndex >= AP_OBJ_SLOTS)
    {
        return;
    }
    mission = missionIndex((int) bossGetStageNum());
    if (mission < 0)
    {
        return;
    }
    if (!sBonus && mission >= AP_CAMPAIGN_COUNT)
    {
        return;
    }
    band = diffIndex((int) lvlGetSelectedDifficulty());
    if (band < 0 || (sCheckMask & (1 << band)) == 0)
    {
        return;
    }
    if (sObjChecked[mission][band] & (1u << objectiveIndex))
    {
        return;
    }
    sObjChecked[mission][band] |= (unsigned short) (1u << objectiveIndex);
    sResend = 1;
    snprintf(line, sizeof(line), "Checked %s %c", kMission[mission], 'A' + objectiveIndex);
    notify(line, 1);
    saveState();
    sendChecks();
}

static int cheatTargetDiff(int index)
{
    /* solo_target_time_array: Secret, 00, Agent, repeating, then Aztec Secret and Egyptian 00. */
    static const unsigned char diff[20] = {
        1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2
    };

    if (index < 0 || index >= 20)
    {
        return -1;
    }
    return (int) diff[index];
}

static int cheatInSeed(int index)
{
    int missions;
    int mission;

    missions = sBonus ? AP_MISSION_COUNT : AP_CAMPAIGN_COUNT;
    if (index < 0 || index >= AP_CHEAT_COUNT)
    {
        return 0;
    }
    if (index < 20)
    {
        mission = index;
    }
    else if (index == AP_CHEAT_MAGNUM)
    {
        mission = 17;
    }
    else if (index == AP_CHEAT_LASER)
    {
        mission = 18;
    }
    else
    {
        mission = 19;
    }
    if (mission >= missions)
    {
        return 0;
    }
    /* One dossier opens every difficulty, so every cart target can be played. */
    if (!sSplitDossiers)
    {
        return 1;
    }
    if (index < 20)
    {
        return (sCheckMask & (1 << cheatTargetDiff(index))) != 0;
    }
    if (index == AP_CHEAT_MAGNUM)
    {
        return 1;
    }
    if (index == AP_CHEAT_LASER)
    {
        return (sCheckMask & 6) != 0;
    }
    return (sCheckMask & 4) != 0;
}

void apOnCheatUnlocked(int which)
{
    char line[96];

    ensureInit();
    if (apSurvivalActive())
    {
        return;
    }
    if (!sEnabled || !sLive || !sCheatChecks)
    {
        return;
    }
    if (!cheatInSeed(which))
    {
        return;
    }
    if (sCheatChecked & (1u << which))
    {
        return;
    }
    sCheatChecked |= 1u << which;
    sResend = 1;
    snprintf(line, sizeof(line), "Checked %s", kCheat[which]);
    notify(line, 1);
    saveState();
    sendChecks();
}

void apOnMissionComplete(int levelid, int difficulty)
{
    int mission;
    int diff;
    char label[64];
    char line[96];

    ensureInit();
    if (apSurvivalActive())
    {
        return;
    }
    if (!sEnabled)
    {
        return;
    }
    if (g_AppendCheatSinglePlayer)
    {
        portConLogf("ge: ap: clear ignored because a cheat is active\n");
        return;
    }
    mission = missionIndex(levelid);
    if (mission < 0)
    {
        return;
    }
    if (!sBonus && mission >= AP_CAMPAIGN_COUNT)
    {
        return;
    }
    diff = diffIndex(difficulty);
    if (diff < 0 || (sCheckMask & (1 << diff)) == 0)
    {
        return;
    }
    if (sChecked[mission] & (1u << diff))
    {
        return;
    }
    sChecked[mission] |= (unsigned char) (1u << diff);
    sResend = 1;
    locLabel(mission, diff, label, (int) sizeof(label));
    snprintf(line, sizeof(line), "Checked %s", label);
    notify(line, 1);
    saveState();
    sendChecks();
    announceGoal();
}

int apStageStatus(int spLevel, int difficulty)
{
    int bit;

    ensureInit();
    if (spLevel < 0 || spLevel >= AP_MISSION_COUNT)
    {
        return 0;
    }
    bit = diffIndex(difficulty);
    if (!sLive || bit < 0 || !ownsDossier(spLevel, bit))
    {
        return 0;
    }
    if (sChecked[spLevel] & (1u << bit))
    {
        return 3;
    }
    return 1;
}

int apMissionMark(int mission, int difficulty)
{
    int bit;

    ensureInit();
    if (!sLive)
    {
        return -1;
    }
    bit = diffIndex(difficulty);
    if (mission < 0 || mission >= AP_MISSION_COUNT || bit < 0)
    {
        return 0;
    }
    /* Not a location in this seed. Grey on the thumbnail, so it is not a target. */
    if ((sCheckMask & (1 << bit)) == 0)
    {
        return 3;
    }
    if (!ownsDossier(mission, bit))
    {
        return 0;
    }
    if (sChecked[mission] & (1u << bit))
    {
        return 2;
    }
    return 1;
}

int apGoalOutro(int mission)
{
    (void) mission;
    ensureInit();
    /* Only the debrief of the mission that just finished the goal. Cradle
     * itself no longer starts the credits. */
    if (!sLive || !goalMet() || !sGoalJustMet)
    {
        return 0;
    }
    sGoalJustMet = 0;
    return 1;
}

int apFirstUnlockedMission(void)
{
    int i;

    ensureInit();
    if (!sLive)
    {
        return -1;
    }
    for (i = 0; i < AP_MISSION_COUNT; i++)
    {
        if (anyDossier(i))
        {
            return i;
        }
    }
    return -1;
}

static void printStatus(void)
{
    int i, n = 0;
    char list[256];
    int used = 0;

    portConLogf("ge: ap: %s %s@%s:%d tls=%d items=%d\n",
                sLive ? "connected" : (sHold ? "held" : "offline"),
                sSlot[0] ? sSlot : "(no slot)", sHost, sPort, sTls, sItemIndex);
    portConLogf("ge: ap: goal %s on %s, checks %d, dossiers %s, bonus %s\n",
                goalName(),
                sGoalAll ? "All" : kDiff[sGoalDiff < AP_DIFF_COUNT ? sGoalDiff : 0],
                sCheckMask,
                sSplitDossiers ? "split" : "shared",
                sBonus ? "on" : "off");
    list[0] = '\0';
    for (i = 0; i < AP_MISSION_COUNT; i++)
    {
        if (anyDossier(i))
        {
            int w = snprintf(list + used, sizeof(list) - (size_t) used, "%s%s", n ? ", " : "", kMission[i]);
            if (w < 0 || used + w >= (int) sizeof(list))
            {
                break;
            }
            used += w;
            n++;
        }
    }
    portConLogf("ge: ap: unlocked: %s\n", n ? list : "(none yet)");
}

int apSeedReady(void)
{
    ensureInit();
    return sLive;
}

/* mission, briefing slot, minimum difficulty. Same list as data.py OBJECTIVES
 * plus OBJECTIVE_RANK (missing there means Agent). */
static const unsigned char kObjRow[][3] = {
    { 0, 0, 1 }, { 0, 1, 2 }, { 0, 2, 2 }, { 0, 3, 0 },
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 2, 0 }, { 1, 3, 0 }, { 1, 4, 0 },
    { 2, 0, 0 }, { 2, 1, 2 }, { 2, 2, 1 }, { 2, 3, 0 },
    { 3, 0, 0 }, { 3, 1, 1 }, { 3, 2, 1 }, { 3, 3, 0 },
    { 4, 0, 1 }, { 4, 1, 0 }, { 4, 2, 2 }, { 4, 3, 2 }, { 4, 4, 0 },
    { 5, 0, 2 }, { 5, 1, 0 }, { 5, 2, 1 }, { 5, 3, 1 }, { 5, 4, 0 },
    { 6, 0, 0 }, { 6, 1, 1 }, { 6, 2, 1 }, { 6, 3, 0 },
    { 7, 0, 2 }, { 7, 1, 0 }, { 7, 2, 1 }, { 7, 3, 0 },
    { 8, 0, 1 }, { 8, 1, 0 }, { 8, 2, 1 }, { 8, 3, 2 }, { 8, 4, 0 },
    { 9, 0, 0 }, { 9, 1, 0 }, { 9, 2, 0 }, { 9, 3, 0 }, { 9, 4, 0 },
    { 10, 0, 0 }, { 10, 1, 0 }, { 10, 2, 1 }, { 10, 3, 0 },
    { 11, 0, 1 }, { 11, 1, 0 }, { 11, 2, 0 },
    { 12, 0, 2 }, { 12, 1, 1 }, { 12, 2, 1 }, { 12, 3, 1 }, { 12, 4, 0 },
    { 13, 0, 0 }, { 13, 1, 0 }, { 13, 2, 1 }, { 13, 3, 2 }, { 13, 4, 0 },
    { 14, 0, 0 }, { 14, 1, 0 }, { 14, 2, 1 }, { 14, 3, 0 },
    { 15, 0, 0 }, { 15, 1, 0 }, { 15, 2, 0 },
    { 16, 0, 1 }, { 16, 1, 1 }, { 16, 2, 1 }, { 16, 3, 2 }, { 16, 4, 0 },
    { 17, 0, 0 }, { 17, 1, 0 },
    { 18, 0, 0 }, { 18, 1, 0 },
    { 19, 0, 0 }, { 19, 1, 0 },
};

static int seedMissionCount(void)
{
    if (sBonus || sGoal == AP_GOAL_AZTEC || sGoal == AP_GOAL_EGYPTIAN)
    {
        return AP_MISSION_COUNT;
    }
    return AP_CAMPAIGN_COUNT;
}

static const char *goalTitle(void)
{
    if (sGoal == AP_GOAL_AZTEC) return "Aztec";
    if (sGoal == AP_GOAL_EGYPTIAN) return "Egyptian";
    if (sGoal == AP_GOAL_CAMPAIGN) return "Campaign";
    return "Cradle";
}

void apFolderStatus(struct ApFolderStatus *out)
{
    int missions;
    int need;
    int m, d, row;
    int goalDone = 0;
    int goalMissions;

    if (out == NULL)
    {
        return;
    }
    memset(out, 0, sizeof(*out));
    ensureInit();
    missions = seedMissionCount();
    need = goalNeedMask();
    if (sGoalAll)
    {
        snprintf(out->goal, sizeof(out->goal), "%s on All", goalTitle());
    }
    else
    {
        int diff = sGoalDiff;
        if (diff < 0 || diff >= AP_DIFF_COUNT)
        {
            diff = 0;
        }
        snprintf(out->goal, sizeof(out->goal), "%s on %s", goalTitle(), kDiff[diff]);
    }
    goalMissions = (sGoal == AP_GOAL_CAMPAIGN) ? AP_CAMPAIGN_COUNT : 1;
    out->goalTotal = goalMissions * maskCount(need);
    if (sGoal == AP_GOAL_CAMPAIGN)
    {
        for (m = 0; m < AP_CAMPAIGN_COUNT; m++)
        {
            for (d = 0; d < AP_DIFF_COUNT; d++)
            {
                if ((need & (1 << d)) && (sChecked[m] & (1u << d)))
                {
                    goalDone++;
                }
            }
        }
    }
    else
    {
        int target = 17;
        if (sGoal == AP_GOAL_AZTEC) target = 18;
        if (sGoal == AP_GOAL_EGYPTIAN) target = 19;
        for (d = 0; d < AP_DIFF_COUNT; d++)
        {
            if ((need & (1 << d)) && (sChecked[target] & (1u << d)))
            {
                goalDone++;
            }
        }
    }
    out->goalDone = goalDone;
    out->missionTotal = missions * maskCount(sCheckMask);
    for (m = 0; m < missions; m++)
    {
        for (d = 0; d < AP_DIFF_COUNT; d++)
        {
            if ((sCheckMask & (1 << d)) && (sChecked[m] & (1u << d)))
            {
                out->missionDone++;
            }
        }
    }
    if (sObjectiveChecks)
    {
        out->objectiveOn = 1;
        for (row = 0; row < (int) (sizeof(kObjRow) / sizeof(kObjRow[0])); row++)
        {
            int mission = kObjRow[row][0];
            int slot = kObjRow[row][1];
            int minimum = kObjRow[row][2];
            if (mission >= missions || slot < 0 || slot >= AP_OBJ_SLOTS)
            {
                continue;
            }
            for (d = minimum; d < AP_DIFF_COUNT; d++)
            {
                if ((sCheckMask & (1 << d)) == 0)
                {
                    continue;
                }
                out->objectiveTotal++;
                if (sObjChecked[mission][d] & (1u << slot))
                {
                    out->objectiveDone++;
                }
            }
        }
    }
    if (!sCheatChecks)
    {
        return;
    }
    out->cheatOn = 1;
    for (m = 0; m < AP_CHEAT_COUNT; m++)
    {
        if (!cheatInSeed(m))
        {
            continue;
        }
        out->cheatTotal++;
        if (sCheatChecked & (1u << m))
        {
            out->cheatDone++;
        }
    }
}

const char *apDiscordApp(void)
{
    ensureInit();
    return sDiscordApp;
}

void apPresenceFill(char *details, int nd, char *state, int ns)
{
    extern MENU get_currentmenu(void);
    int mission;
    const char *diff = "";

    if (details == NULL || state == NULL || nd < 2 || ns < 2)
    {
        return;
    }
    ensureInit();
    details[0] = '\0';
    state[0] = '\0';
    mission = missionIndex((int) bossGetStageNum());
    if (mission >= 0 && get_currentmenu() == MENU_RUN_STAGE)
    {
        int d = (int) lvlGetSelectedDifficulty();

        if (d >= 0 && d < AP_DIFF_COUNT)
        {
            diff = kDiff[d];
        }
        snprintf(details, (size_t) nd, "%s", kMission[mission]);
        if (sLive && sSlot[0] != '\0')
        {
            snprintf(state, (size_t) ns, "%s%s%s", diff, diff[0] ? " - " : "", sSlot);
        }
        else
        {
            snprintf(state, (size_t) ns, "%s", diff[0] ? diff : "Solo");
        }
        return;
    }
    snprintf(details, (size_t) nd, "%s", "In the folder");
    if (sLive && sSlot[0] != '\0')
    {
        snprintf(state, (size_t) ns, "Archipelago - %s", sSlot);
    }
    else
    {
        snprintf(state, (size_t) ns, "%s", "GoldenEye 007");
    }
}

void apConnectionLine(char *buf, int n)
{
    ensureInit();
    if (buf == NULL || n < 2)
    {
        return;
    }
    if (sLive && sSeed[0] != '\0')
    {
        snprintf(buf, (size_t) n, "Seed %s", sSeed);
    }
    else if (sLive)
    {
        snprintf(buf, (size_t) n, "Connected");
    }
    else if (!sWantConnect)
    {
        snprintf(buf, (size_t) n, "Fill host, port and slot, then CONNECT");
    }
    else if (sHold)
    {
        snprintf(buf, (size_t) n, "Connection refused");
    }
    else if (sSlot[0] == '\0')
    {
        snprintf(buf, (size_t) n, "Enter your slot name");
    }
    else if (apNetIsOpen() || apNetBusy())
    {
        snprintf(buf, (size_t) n, "Connecting to %s:%d", sHost, sPort);
    }
    else
    {
        snprintf(buf, (size_t) n, "Waiting for the server");
    }
}

static void copyField(char *dst, int n, const char *src)
{
    if (dst == NULL || n < 1)
    {
        return;
    }
    snprintf(dst, (size_t) n, "%s", src != NULL ? src : "");
}

void apGetHost(char *buf, int n) { ensureInit(); copyField(buf, n, sHost); }
void apGetSlot(char *buf, int n) { ensureInit(); copyField(buf, n, sSlot); }
void apGetPassword(char *buf, int n) { ensureInit(); copyField(buf, n, sPassword); }
int apGetPort(void) { ensureInit(); return sPort; }
int apGetTls(void) { ensureInit(); return sTls ? 1 : 0; }

void apSetHost(const char *s)
{
    ensureInit();
    copyField(sHost, (int) sizeof(sHost), s);
    normalizeHost();
    apKick();
}

void apSetSlot(const char *s)
{
    ensureInit();
    copyField(sSlot, (int) sizeof(sSlot), s);
    apKick();
}

void apSetPassword(const char *s)
{
    ensureInit();
    copyField(sPassword, (int) sizeof(sPassword), s);
    apKick();
}

void apSetPort(int port)
{
    ensureInit();
    if (port > 0 && port <= 65535)
    {
        sPort = port;
    }
    apKick();
}

void apSetTls(int on)
{
    ensureInit();
    sTls = on ? 1 : 0;
    apKick();
}

void apApplyConnection(const char *host, const char *slot, const char *password, int port, int tls)
{
    ensureInit();
    copyField(sHost, (int) sizeof(sHost), host);
    normalizeHost();
    copyField(sSlot, (int) sizeof(sSlot), slot);
    copyField(sPassword, (int) sizeof(sPassword), password);
    if (port > 0 && port <= 65535)
    {
        sPort = port;
    }
    sTls = tls ? 1 : 0;
    apKick();
}

static void pushDeathLinkTag(void)
{
    if (!sLive || !apNetIsOpen())
    {
        return;
    }
    if (sDeathLink)
    {
        sendRaw("[{\"cmd\":\"ConnectUpdate\",\"tags\":[\"AP\",\"DeathLink\"]}]");
    }
    else
    {
        sendRaw("[{\"cmd\":\"ConnectUpdate\",\"tags\":[\"AP\"]}]");
    }
}

int apGetDeathLink(void)
{
    ensureInit();
    if (sLive)
    {
        return sDeathLink ? 1 : 0;
    }
    return sDeathChoice > 0 ? 1 : 0;
}

void apToggleDeathLink(void)
{
    int next;

    ensureInit();
    next = apGetDeathLink() ? 0 : 1;
    sDeathChoice = next;
    sDeathLink = next;
    saveIni();
    pushDeathLinkTag();
}

void apSetPopup(int on)
{
    ensureInit();
    apPopupSetEnabled(on ? 1 : 0);
    saveIni();
    if (on)
    {
        apPopupPush("Popup preview", 1, AP_POP_USEFUL);
    }
}

void apCyclePopupPos(int dir)
{
    ensureInit();
    apPopupCyclePos(dir);
    saveIni();
    if (apGetPopup())
    {
        apPopupPush("Popup preview", 1, AP_POP_USEFUL);
    }
}

s32 portApOptionGet(const char *key, s32 *out)
{
    ensureInit();
    if (out == NULL || key == NULL)
    {
        return 0;
    }
    if (eqI(key, "popup"))
    {
        *out = apGetPopup() ? 1 : 0;
        return 1;
    }
    if (eqI(key, "popup_pos"))
    {
        *out = apPopupPosIndex();
        return 1;
    }
    if (eqI(key, "tls"))
    {
        *out = sTls ? 1 : 0;
        return 1;
    }
    return 0;
}

s32 portApOptionSet(const char *key, s32 value)
{
    ensureInit();
    if (key == NULL)
    {
        return 0;
    }
    if (eqI(key, "popup"))
    {
        apPopupSetEnabled(value ? 1 : 0);
        if (value)
        {
            apPopupPush("Popup preview", 1, AP_POP_USEFUL);
        }
        return 1;
    }
    if (eqI(key, "popup_pos"))
    {
        apPopupSetPosIndex(value);
        if (apGetPopup())
        {
            apPopupPush("Popup preview", 1, AP_POP_USEFUL);
        }
        return 1;
    }
    if (eqI(key, "tls"))
    {
        sTls = value ? 1 : 0;
        sHold = 0;
        sNextTry = 0;
        apKick();
        return 1;
    }
    return 0;
}

#ifdef GE_AP_DEBUG
static const char *const kDebug[] = {
    "Baron Samedi",
    "Greasy Hands",
    "Camera Alert",
    "Jammed Clip",
    "Slipped on the Ice",
    "Ammo Clip",
    "Body Armor",
    "Ammo Cache",
    "Death Link",
    "Restart Mission",
    "Tiny Bond",
    "The Living Daylights",
    "I am invisible",
    "DK Mode",
    "Paintball",
    "Single Bullet"
};

int apDebugCount(void)
{
    return (int) (sizeof(kDebug) / sizeof(kDebug[0]));
}

const char *apDebugLabel(int index)
{
    if (index < 0 || index >= apDebugCount())
    {
        return "";
    }
    return kDebug[index];
}

const char *apDebugNote(void)
{
    return sDebugNote;
}

static void debugSay(const char *msg)
{
    snprintf(sDebugNote, sizeof(sDebugNote), "%s", msg);
    notify(msg, 1);
}

static int debugItem(int index)
{
    if (index == 1) return AP_ITEM_GREASY;
    if (index == 2) return AP_ITEM_ALERT;
    if (index == 3) return AP_ITEM_JAM;
    if (index == 4) return AP_ITEM_SLIP;
    if (index == 5) return AP_ITEM_CLIP;
    if (index == 9) return AP_ITEM_RESTART;
    if (index == 10) return AP_ITEM_TINY;
    if (index == 11) return AP_ITEM_FLAG;
    if (index == 12) return AP_ITEM_BORIS;
    if (index == 13) return AP_ITEM_DK;
    if (index == 14) return AP_ITEM_PAINT;
    if (index == 15) return AP_ITEM_BULLET;
    return AP_ITEM_BARON;
}

void apDebugFire(int index)
{
    extern void bondviewKillCurrentPlayer(void);

    if (index < 0 || index >= apDebugCount())
    {
        return;
    }
    if (index == 6)
    {
        sArmorStock++;
        saveState();
        debugSay("Debug: Body Armor");
        return;
    }
    if (index == 7)
    {
        sAmmoStock++;
        saveState();
        debugSay("Debug: Ammo Cache");
        return;
    }
    if (index == 8)
    {
        if (inMission() && sSettle >= 30 && g_CurrentPlayer != NULL && !g_CurrentPlayer->bonddead)
        {
            sDeathFromLink = 1;
            bondviewKillCurrentPlayer();
            sDeathFromLink = 0;
            debugSay("Debug: Death Link");
        }
        else
        {
            sPendDeath++;
            debugSay("Debug: Death Link waits for the mission");
        }
        return;
    }
    if (inMission() && sSettle >= 30)
    {
        char line[80];
        int id = debugItem(index);

        if (applyKind(id))
        {
            snprintf(line, sizeof(line), "Debug: %s", kDebug[index]);
        }
        else
        {
            snprintf(line, sizeof(line), "Debug: %s had no room", kDebug[index]);
        }
        debugSay(line);
        return;
    }
    {
        int id = debugItem(index);
        char line[80];

        queueConsumable(id);
        snprintf(line, sizeof(line), "Debug: %s waits for the mission", kDebug[index]);
        debugSay(line);
    }
}
#endif

void apConsole(const char *verb, const char *rest)
{
    ensureInit();
    if (verb == NULL || verb[0] == '\0' || eqI(verb, "help"))
    {
        portConLogf("ge: ap: status, connect, disconnect, enable 0|1, host, port, tls 0|1, slot, password, popup 0|1, place, say, survival\n");
#ifdef GE_AP_DEBUG
        portConLogf("ge: ap: debug, or debug 1-%d\n", apDebugCount());
#endif
        return;
    }
#ifdef GE_AP_DEBUG
    if (eqI(verb, "debug"))
    {
        int which;
        int i;

        if (rest == NULL || rest[0] == '\0')
        {
            portConLogf("ge: ap: debug 1-%d\n", apDebugCount());
            for (i = 0; i < apDebugCount(); i++)
            {
                portConLogf("ge: ap: %d %s\n", i + 1, apDebugLabel(i));
            }
            return;
        }
        which = atoi(rest);
        apDebugFire(which - 1);
        return;
    }
#endif
    if (eqI(verb, "survival"))
    {
        apSurvivalOpen();
        portConLogf("ge: ap: survival arena grid\n");
        return;
    }
    if (eqI(verb, "status"))
    {
        printStatus();
        return;
    }
    if (eqI(verb, "say"))
    {
        char esc[160];
        char packet[400];
        if (!sLive)
        {
            portConLogf("ge: ap: not connected\n");
            return;
        }
        jsonEscape(rest != NULL ? rest : "", esc, (int) sizeof(esc));
        snprintf(packet, sizeof(packet), "[{\"cmd\":\"Say\",\"text\":\"%s\"}]", esc);
        sendRaw(packet);
        return;
    }
    if (eqI(verb, "connect"))
    {
        sEnabled = 1;
        sHold = 0;
        sWaitLog = 0;
        sNextTry = 0;
        saveIni();
        apNetStop();
        portConLogf("ge: ap: connecting\n");
        return;
    }
    if (eqI(verb, "disconnect"))
    {
        sHold = 1;
        apNetStop();
        portConLogf("ge: ap: disconnected; `ap connect` to try again\n");
        return;
    }
    if (eqI(verb, "enable"))
    {
        sEnabled = rest != NULL && truth(rest);
        sHold = 0;
        sWaitLog = 0;
        saveIni();
        if (!sEnabled)
        {
            apNetStop();
        }
        portConLogf("ge: ap: enabled = %d\n", sEnabled);
        return;
    }
    if (eqI(verb, "host"))
    {
        copyArg(sHost, (int) sizeof(sHost), rest);
        normalizeHost();
        sHold = 0;
        sNextTry = 0;
        saveIni();
        apNetStop();
        portConLogf("ge: ap: host = %s port %d tls %d\n", sHost, sPort, sTls);
        return;
    }
    if (eqI(verb, "port"))
    {
        int port = rest != NULL ? atoi(rest) : 0;
        if (port > 0 && port <= 65535)
        {
            sPort = port;
        }
        sHold = 0;
        sNextTry = 0;
        saveIni();
        apNetStop();
        portConLogf("ge: ap: port = %d\n", sPort);
        return;
    }
    if (eqI(verb, "tls"))
    {
        sTls = rest != NULL && truth(rest);
        sHold = 0;
        sNextTry = 0;
        saveIni();
        apNetStop();
        portConLogf("ge: ap: tls = %d\n", sTls);
        return;
    }
    if (eqI(verb, "popup"))
    {
        apSetPopup(rest != NULL && truth(rest));
        portConLogf("ge: ap: popup = %d (%s)\n", apGetPopup(), apPopupPosName());
        return;
    }
    if (eqI(verb, "place") || eqI(verb, "popup_pos"))
    {
        if (rest != NULL && rest[0] != '\0')
        {
            apPopupSetPosName(rest);
            saveIni();
            if (apGetPopup())
            {
                apPopupPush("Popup preview", 1, AP_POP_USEFUL);
            }
        }
        else
        {
            apCyclePopupPos(1);
        }
        portConLogf("ge: ap: popup position = %s\n", apPopupPosName());
        return;
    }
    if (eqI(verb, "slot"))
    {
        copyArg(sSlot, (int) sizeof(sSlot), rest);
        sHold = 0;
        sWaitLog = 0;
        sNextTry = 0;
        saveIni();
        apNetStop();
        portConLogf("ge: ap: slot = %s\n", sSlot);
        return;
    }
    if (eqI(verb, "password"))
    {
        copyArg(sPassword, (int) sizeof(sPassword), rest);
        sHold = 0;
        sNextTry = 0;
        saveIni();
        apNetStop();
        portConLogf("ge: ap: password %s\n", sPassword[0] ? "set" : "cleared");
        return;
    }
    portConLogf("ge: ap: unknown command; try `ap help`\n");
}
