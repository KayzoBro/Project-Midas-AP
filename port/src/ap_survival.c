/**
 * Survival on a multiplayer arena.
 *
 * The arena grid is the one the multiplayer menus already draw. Confirming
 * a photo loads that stage's multiplayer setup with one player, then guards
 * are spawned on its pads. A kneel shot used to index the firing table with
 * a negative heading; that index is clamped where the shot is started.
 */
#include "ap_survival.h"
#include "ap_client.h"
#include "ap_enemy.h"

#include <stdio.h>
#include <string.h>

#include <ultra64.h>
#include <bondconstants.h>
#include <bondaicommands.h>

#include "boss.h"
#include "game/bondinv.h"
#include "game/chr.h"
#include "game/chrai.h"
#include "game/chraction.h"
#include "game/front.h"
#include "game/gun.h"
#include "game/mp_weapon.h"
#include "game/objecthandler.h"
#include "game/player.h"
#include "memp.h"

extern void set_solo_and_ptr_briefing(LEVELID stage);
extern void frontChangeMenu(MENU menu, s32 reload);
extern PropRecord *chrSpawnAtCoord(s32 bodynum, s32 headnum, coord3d *pos, StandTile *stan, f32 angle, AIListRecord *ailist, s32 spawnflags);
extern s32 chrGetNumFree(void);
extern void chrpropCleanupForRemoval(PropRecord *prop);
extern void chrpropDelist(PropRecord *prop);
extern void chrpropDisable(PropRecord *prop);
extern void chrpropFree(PropRecord *prop);
extern s32 get_ammo_type_for_weapon(s32 weapon);
extern ITEM_IDS starting_weapon[];
extern s32 g_GlobalTimer;

#define SURV_MAX 8
#define SURV_SLOTS 12
#define SURV_PAD_CAP 128

static int sPicking;
static int sPending;
static int sRunning;
static LEVELID sStage;
static int sWave;
static int sWait;
static int sLent;
static int sDrawn;
static ChrRecord *sGuard[SURV_SLOTS];
/* g_GlobalTimer when this slot's death animation finished. -1 while he is up. */
static s32 sDeadSince[SURV_SLOTS];

static const s32 kBody[] = {
    BODY_Russian_Soldier,
    BODY_Russian_Infantry,
    BODY_St_Petersburg_Guard,
    BODY_Janus_Special_Forces,
    BODY_Janus_Marine,
    BODY_Naval_Officer
};

static const struct
{
    s32 item;
    s32 prop;
} kGun[] = {
    { ITEM_WPPK, PROP_CHRWPPK },
    { ITEM_AK47, PROP_CHRKALASH },
    { ITEM_MP5K, PROP_CHRMP5K },
    { ITEM_SHOTGUN, PROP_CHRSHOTGUN },
    { ITEM_RUGER, PROP_CHRRUGER }
};

int apSurvivalPicking(void)
{
    return sPicking;
}

int apSurvivalActive(void)
{
    return sPending || sRunning;
}

/* The multiplayer setup of each of these is the solo pad list, cut short,
 * with the same index still on the same point. The solo path tables name
 * those indices. The other arenas have no such list. The setup files are
 * served from the ROM, so the solo one is loaded by the name the game
 * already uses. */
extern char *setup_text_pointers[];
extern void *_fileNameLoadToBank(char *filename, int loadMethod, s32 size, u8 bank);
extern s32 fileGetIndex(u8 *name);
extern s32 romdataFileServed(s32 index);
extern void portConLogf(const char *fmt, ...);

static int sharedStage(LEVELID stage)
{
    switch (stage)
    {
        case LEVELID_FACILITY:
        case LEVELID_BUNKER2:
        case LEVELID_ARCHIVES:
        case LEVELID_CAVERNS:
        case LEVELID_EGYPT:
        case LEVELID_STATUE:
        case LEVELID_CRADLE:
            return 1;
        default:
            return 0;
    }
}

/* Offsets inside a ROM setup become pointers. A compiled-in file is already
 * pointers; adding the base again would break it. */
static void *promote(void *base, void *val, s32 fromRom)
{
    if (fromRom)
    {
        return (void *) (((u32) base) + ((u32) val));
    }
    return val;
}

static stagesetup *loadSoloPaths(LEVELID stage)
{
    char *name;
    stagesetup *file;
    s32 fromRom;
    int i;

    if ((int) stage < 0 || setup_text_pointers[stage] == NULL)
    {
        return NULL;
    }
    name = setup_text_pointers[stage];
    file = (stagesetup *) _fileNameLoadToBank(name, 1, 256, MEMPOOL_STAGE);
    if (file == NULL)
    {
        portConLogf("ge: ap: survival paths had no setup %s\n", name);
        return NULL;
    }
    fromRom = romdataFileServed(fileGetIndex((u8 *) name));
    file->pathwaypoints = promote(file, file->pathwaypoints, fromRom);
    file->waypointgroups = promote(file, file->waypointgroups, fromRom);
    file->pads = promote(file, file->pads, fromRom);
    if (file->pathwaypoints != NULL)
    {
        for (i = 0; file->pathwaypoints[i].padID >= 0; i++)
        {
            file->pathwaypoints[i].neighbours = promote(file, file->pathwaypoints[i].neighbours, fromRom);
        }
    }
    if (file->waypointgroups != NULL)
    {
        for (i = 0; file->waypointgroups[i].neighbours != NULL; i++)
        {
            file->waypointgroups[i].neighbours = promote(file, file->waypointgroups[i].neighbours, fromRom);
            file->waypointgroups[i].waypoints = promote(file, file->waypointgroups[i].waypoints, fromRom);
        }
    }
    return file;
}

void apSurvivalGraftPaths(void)
{
    stagesetup *solo;
    PadRecord *mpPads;
    PadRecord *soloPads;
    waypoint *wp;
    int mpCount;
    int i;

    if (!sPending && !sRunning || !sharedStage(sStage))
    {
        return;
    }
    solo = loadSoloPaths(sStage);
    if (solo == NULL || solo->pathwaypoints == NULL || solo->waypointgroups == NULL || solo->pads == NULL)
    {
        return;
    }
    mpPads = g_CurrentSetup.pads;
    if (mpPads == NULL)
    {
        return;
    }
    mpCount = 0;
    while (mpPads[mpCount].plink != NULL && mpCount < 4096)
    {
        mpCount++;
    }
    soloPads = solo->pads;
    wp = solo->pathwaypoints;
    for (i = 0; wp[i].padID >= 0; i++)
    {
        int id = wp[i].padID;
        f32 dx;
        f32 dy;
        f32 dz;

        if (id >= mpCount || soloPads[id].plink == NULL)
        {
            portConLogf("ge: ap: survival paths refused, pad %d\n", id);
            return;
        }
        dx = soloPads[id].pos.x - mpPads[id].pos.x;
        dy = soloPads[id].pos.y - mpPads[id].pos.y;
        dz = soloPads[id].pos.z - mpPads[id].pos.z;
        /* A reordered pad list would send guards through the walls. */
        if ((dx * dx) + (dy * dy) + (dz * dz) > 1.0f)
        {
            portConLogf("ge: ap: survival paths refused, pad %d moved\n", id);
            return;
        }
    }
    if (i == 0)
    {
        return;
    }
    /* Neighbour pointers already address the solo tables. Rebasing them
     * would add the multiplayer file base and break the route. The search
     * writes distance on these groups and clears it at the start. */
    g_CurrentSetup.pathwaypoints = solo->pathwaypoints;
    g_CurrentSetup.waypointgroups = solo->waypointgroups;
}

void apSurvivalOpen(void)
{
    sPicking = 1;
    frontChangeMenu(MENU_MP_STAGE_SELECT, FALSE);
}

void apSurvivalCancelPick(void)
{
    sPicking = 0;
}

static void wipeGuards(void)
{
    int i;

    for (i = 0; i < SURV_SLOTS; i++)
    {
        sGuard[i] = NULL;
        sDeadSince[i] = -1;
    }
}

static void stopRun(void)
{
    sPending = 0;
    sRunning = 0;
    sWave = 0;
    sWait = 0;
    sLent = 0;
    sDrawn = 0;
    wipeGuards();
    apPopupTimer(4, NULL, 0);
    /* The arena used the multiplayer difficulty mask. The folder is Agent. */
    set_selected_difficulty(DIFFICULTY_AGENT);
}

void apSurvivalStartStage(int levelId)
{
    if (levelId < 0)
    {
        return;
    }
    sPicking = 0;
    sStage = (LEVELID) levelId;
    sPending = 1;
    sRunning = 0;
    sWave = 0;
    sWait = 0;
    sLent = 0;
    sDrawn = 0;
    wipeGuards();
    /* MULTI is the mask the arena props were built for. Agent skips the pads
     * marked "don't load on Agent", which is where the weapon set sits. */
    set_selected_difficulty(DIFFICULTY_MULTI);
    set_solo_and_ptr_briefing(sStage);
    frontChangeMenu(MENU_RUN_STAGE, TRUE);
}

static int firearmOwned(void)
{
    static const ITEM_IDS guns[] = {
        ITEM_WPPK, ITEM_WPPKSIL, ITEM_TT33, ITEM_SKORPION, ITEM_AK47, ITEM_UZI,
        ITEM_MP5K, ITEM_MP5KSIL, ITEM_SPECTRE, ITEM_M16, ITEM_FNP90,
        ITEM_SHOTGUN, ITEM_AUTOSHOT, ITEM_SNIPERRIFLE, ITEM_RUGER,
        ITEM_GOLDENGUN, ITEM_LASER
    };
    int i;

    for (i = 0; i < (int) (sizeof(guns) / sizeof(guns[0])); i++)
    {
        if (bondinvItemAvailable(guns[i]))
        {
            return 1;
        }
    }
    return 0;
}

/* The arena's eight slots. Names come from the gun list, which is always
 * loaded. A raw 0xF0 slot has no name and is not a weapon. */
static void giveWeaponSet(void)
{
    struct s_mp_weapon_set *set;
    int i;
    int first = -1;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    set = getPtrMPWeaponSetData();
    if (set == NULL)
    {
        return;
    }
    for (i = 0; i < 8; i++)
    {
        int item = (int) set[i].itemID;
        int type;
        int amount;

        if (item <= ITEM_FIST)
        {
            continue;
        }
        bondinvAddInvItem((ITEM_IDS) item);
        weaponLoadProjectileModels((ITEM_IDS) item);
        type = (int) set[i].ammotype;
        amount = (int) set[i].ammoamount;
        if (type > 0 && type < 30 && amount > g_CurrentPlayer->ammoheldarr[type])
        {
            g_CurrentPlayer->ammoheldarr[type] = amount;
        }
        if (first < 0 && set[i].allowpickup)
        {
            first = i;
        }
    }
    if (first >= 0)
    {
        starting_weapon[GUNRIGHT] = (ITEM_IDS) set[first].itemID;
    }
}

static void lendGun(void)
{
    int type;

    giveWeaponSet();
    if (sLent || firearmOwned())
    {
        sLent = 1;
        return;
    }
    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    sLent = 1;
    bondinvAddInvItem(ITEM_WPPK);
    type = get_ammo_type_for_weapon(ITEM_WPPK);
    if (type > 0 && type < 30 && g_CurrentPlayer->ammoheldarr[type] < 100)
    {
        g_CurrentPlayer->ammoheldarr[type] = 100;
    }
    if (starting_weapon[GUNRIGHT] == ITEM_FIST || starting_weapon[GUNRIGHT] == ITEM_UNARMED)
    {
        starting_weapon[GUNRIGHT] = ITEM_WPPK;
    }
}

void apSurvivalOnSpawn(void)
{
    if (!apSurvivalActive())
    {
        return;
    }
    lendGun();
}

static int guardAlive(ChrRecord *chr)
{
    if (chr == NULL || chr->model == NULL)
    {
        return 0;
    }
    return chr->actiontype != ACT_DIE && chr->actiontype != ACT_DEAD;
}

static void retire(ChrRecord *chr)
{
    PropRecord *prop;

    if (chr == NULL || chr->prop == NULL)
    {
        return;
    }
    prop = chr->prop;
    chrpropCleanupForRemoval(prop);
    chrpropDelist(prop);
    chrpropDisable(prop);
    chrpropFree(prop);
}

static int living(void)
{
    int i;
    int n = 0;

    for (i = 0; i < SURV_SLOTS; i++)
    {
        ChrRecord *chr = sGuard[i];

        if (chr == NULL)
        {
            continue;
        }
        if (chr->model == NULL)
        {
            sGuard[i] = NULL;
            sDeadSince[i] = -1;
            continue;
        }
        /* ACT_DIE is the fall. Removing him here skips the animation. */
        if (chr->actiontype == ACT_DIE)
        {
            continue;
        }
        if (chr->actiontype == ACT_DEAD)
        {
            if (sDeadSince[i] < 0)
            {
                sDeadSince[i] = g_GlobalTimer;
            }
            if (g_GlobalTimer - sDeadSince[i] > 90)
            {
                retire(chr);
                sGuard[i] = NULL;
                sDeadSince[i] = -1;
            }
            continue;
        }
        sDeadSince[i] = -1;
        n++;
    }
    return n;
}

static PadRecord *padNumber(int nth)
{
    PadRecord *pad;
    PadRecord *fallback = NULL;
    int seen = 0;
    int walked = 0;
    PropRecord *player;

    if (g_CurrentSetup.pads == NULL || g_CurrentPlayer == NULL || g_CurrentPlayer->prop == NULL)
    {
        return NULL;
    }
    player = g_CurrentPlayer->prop;
    for (pad = g_CurrentSetup.pads; pad->plink != NULL && walked < SURV_PAD_CAP; pad++, walked++)
    {
        f32 dx;
        f32 dz;

        if (pad->stan == NULL)
        {
            continue;
        }
        dx = pad->pos.x - player->pos.x;
        dz = pad->pos.z - player->pos.z;
        if (dx * dx + dz * dz < 400.0f * 400.0f)
        {
            continue;
        }
        if (fallback == NULL)
        {
            fallback = pad;
        }
        if (seen == nth)
        {
            return pad;
        }
        seen++;
    }
    return fallback;
}

static int remember(ChrRecord *chr)
{
    int i;

    for (i = 0; i < SURV_SLOTS; i++)
    {
        if (sGuard[i] == NULL)
        {
            sGuard[i] = chr;
            sDeadSince[i] = -1;
            return 1;
        }
    }
    return 0;
}

static void spawnOne(int index, int gun)
{
    PadRecord *pad;
    PropRecord *spawned;
    ChrRecord *chr;
    AIRecord *guard;
    f32 angle;

    if (chrGetNumFree() < 3)
    {
        return;
    }
    pad = padNumber(index);
    if (pad == NULL)
    {
        return;
    }
    angle = atan2f(pad->look.f[0], pad->look.f[2]);
    apEnemySpareNext();
    spawned = chrSpawnAtCoord(kBody[index % (int) (sizeof(kBody) / sizeof(kBody[0]))],
                               -1, &pad->pos, pad->stan, angle, NULL, 0x10);
    if (spawned == NULL || spawned->chr == NULL)
    {
        apEnemySpareCancel();
        return;
    }
    chr = spawned->chr;
    apEnemyForget(chr);
    chrGiveWeapon(chr, kGun[gun].prop, (ITEM_IDS) kGun[gun].item, 0);
    /* The idle guard list shoots where it stands. This one plots a route,
     * and the route is what opens a door in the way. */
    guard = ailistFindById(GAILIST_PERSISTENTLY_CHASE_AND_ATTACK_BOND);
    if (guard != NULL)
    {
        chr->ailist = guard;
        chr->aioffset = 0;
    }
    chr->speedrating = 50;
    chr->alertness = 255;
    chr->visionrange = 250.0f;
    /* Alert is cleared again at the end of the first AI tick. The run
     * order is what actually starts them moving, doors included. */
    chrlvAlertGuardToPlayerPosition(chr);
    chrGoToBond(chr, SPEED_RUN);
    if (!remember(chr))
    {
        retire(chr);
    }
}

/* Seeing Bond makes the chase list stop and shoot. Past this distance that
 * leaves him standing in the doorway, so he is sent running again. */
#define SURV_SHOOT_RANGE 500.0f

static void keepAlert(ChrRecord *chr)
{
    PropRecord *player;
    f32 dx;
    f32 dz;

    if (!guardAlive(chr) || chr->prop == NULL || g_CurrentPlayer == NULL)
    {
        return;
    }
    player = g_CurrentPlayer->prop;
    if (player == NULL)
    {
        return;
    }
    chr->alertness = 255;
    chrlvAlertGuardToPlayerPosition(chr);
    dx = chr->prop->pos.x - player->pos.x;
    dz = chr->prop->pos.z - player->pos.z;
    if ((dx * dx) + (dz * dz) < SURV_SHOOT_RANGE * SURV_SHOOT_RANGE)
    {
        return;
    }
    if (chr->actiontype == ACT_GOPOS || chr->actiontype == ACT_RUNPOS
        || chr->actiontype == ACT_ATTACKWALK)
    {
        return;
    }
    chrGoToBond(chr, SPEED_RUN);
}

static void showWave(int left)
{
    char line[40];

    snprintf(line, sizeof(line), "Wave %d    %d left", sWave, left);
    apPopupTimer(4, line, 0);
}

static void beginWave(void)
{
    int count;
    int gun;
    int i;
    int guns = (int) (sizeof(kGun) / sizeof(kGun[0]));

    sWave++;
    count = 3 + sWave;
    if (count > SURV_MAX)
    {
        count = SURV_MAX;
    }
    gun = sWave - 1;
    if (gun >= guns)
    {
        gun = guns - 1;
    }
    for (i = 0; i < count; i++)
    {
        spawnOne(sWave + i, gun);
    }
    sWait = 180;
}

static int stageIsReal(LEVELID stage)
{
    return stage != LEVELID_NONE && stage != LEVELID_TITLE;
}

void apSurvivalTick(void)
{
    MENU menu;
    LEVELID stage;

    if (!sPending && !sRunning)
    {
        return;
    }
    menu = get_currentmenu();
    stage = bossGetStageNum();
    if (sPending && stage == sStage && menu == MENU_RUN_STAGE)
    {
        sPending = 0;
        sRunning = 1;
        sWave = 0;
        sWait = 90;
        sLent = 0;
        sDrawn = 0;
        wipeGuards();
    }
    if (sPending && menu == MENU_RUN_STAGE && stageIsReal(stage) && stage != sStage)
    {
        stopRun();
        return;
    }
    if (sPending && (menu == MENU_MISSION_SELECT || menu == MENU_FILE_SELECT || menu == MENU_MODE_SELECT
                     || menu == MENU_MP_OPTIONS))
    {
        stopRun();
        return;
    }
    if (!sRunning)
    {
        return;
    }
    /* Title and an empty stage number happen while the run is closing and
     * the menu is still the stage. Stopping here made the exit look like a
     * failed Dam, the first briefing in the folder. */
    if (menu == MENU_RUN_STAGE && stageIsReal(stage) && stage != sStage)
    {
        stopRun();
        return;
    }
    if (menu == MENU_MISSION_SELECT
        || menu == MENU_FILE_SELECT
        || menu == MENU_MODE_SELECT
        || menu == MENU_MISSION_COMPLETE
        || menu == MENU_MP_OPTIONS
        || menu == MENU_MP_STAGE_SELECT)
    {
        stopRun();
        return;
    }
    if (menu != MENU_RUN_STAGE || g_CurrentPlayer == NULL || g_CurrentPlayer->bonddead)
    {
        return;
    }
    if (g_GlobalTimer < 30)
    {
        return;
    }
    lendGun();
    if (!sDrawn)
    {
        ITEM_IDS hand = getCurrentPlayerWeaponId(GUNRIGHT);

        if (hand != ITEM_FIST && hand != ITEM_UNARMED)
        {
            sDrawn = 1;
        }
        else if (starting_weapon[GUNRIGHT] > ITEM_FIST)
        {
            currentPlayerEquipWeaponWrapper(GUNRIGHT, starting_weapon[GUNRIGHT]);
            sDrawn = 1;
        }
    }
    {
        int left = living();

        if (sWave > 0)
        {
            int i;

            showWave(left);
            for (i = 0; i < SURV_SLOTS; i++)
            {
                if (sGuard[i] != NULL)
                {
                    keepAlert(sGuard[i]);
                }
            }
        }
        if (left > 0)
        {
            sWait = 180;
            return;
        }
    }
    if (sWait > 0)
    {
        sWait--;
        return;
    }
    beginWave();
}
