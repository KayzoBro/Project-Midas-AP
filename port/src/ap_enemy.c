/**
 * Anonymous-guard rolls from the Archipelago slot.
 * Skins, heads, and music are cosmetic. Weapons and accuracy are the hard ones.
 */
#include "ap_enemy.h"

#include <stdio.h>
#include <string.h>

#include <ultra64.h>
#include <bondconstants.h>
#include <bondtypes.h>

#include "boss.h"
#include "game/chr.h"
#include "game/objecthandler.h"
#include "port_console.h"

#define AP_ENEMY_OFF 0
#define AP_ENEMY_GROUPED 1
#define AP_ENEMY_CHAOTIC 2
#define AP_MARK_MAX 1024

static char sSeed[128];
static int sSkins;
static int sHeads;
static int sWeapons;
static int sAccuracy;
static int sMusic;
static int sLethal;
static int sStage = -9999;
static int sLogged;
static unsigned char sMark[AP_MARK_MAX];
/* Boris is built inside chrAllocate, which arms a bare marked slot before
 * the caller can flag him. One spawn skips that arm. */
static int sSpareNext;

static const s32 kBodies[] = {
    BODY_Jungle_Commando,
    BODY_St_Petersburg_Guard,
    BODY_Russian_Soldier,
    BODY_Russian_Infantry,
    BODY_Janus_Special_Forces,
    BODY_Janus_Marine,
    BODY_Russian_Commandant,
    BODY_Naval_Officer,
    BODY_Siberian_Guard_1_Mishkin,
    BODY_Siberian_Guard_2,
    BODY_Siberian_Special_Forces,
    BODY_Special_Operations_Uniform,
    BODY_Arctic_Commando,
    BODY_Moonraker_Elite_1_Male,
    BODY_Moonraker_Elite_2_Female
};

/* Anything a generic enemy can already be handed. Gadgets are absent on purpose.
 * Knives, mines, the Golden Gun, and the Silver and Gold PP7 stay in the
 * table so a seed's other rolls keep their index. weaponPick never hands
 * them out. A guard with a knife or a mine has no behaviour for it and just
 * runs. A remote mine can also stick to the Surface 2 helicopter. The Golden
 * Gun and the two cheat PP7s end a fight in one shot. */
static const struct {
    s32 item;
    s32 prop;
} kGuns[] = {
    { ITEM_KNIFE, PROP_CHRKNIFE },
    { ITEM_THROWKNIFE, PROP_CHRTHROWKNIFE },
    { ITEM_WPPK, PROP_CHRWPPK },
    { ITEM_WPPKSIL, PROP_CHRWPPKSIL },
    { ITEM_TT33, PROP_CHRTT33 },
    { ITEM_SKORPION, PROP_CHRSKORPION },
    { ITEM_AK47, PROP_CHRKALASH },
    { ITEM_UZI, PROP_CHRUZI },
    { ITEM_MP5K, PROP_CHRMP5K },
    { ITEM_MP5KSIL, PROP_CHRMP5KSIL },
    { ITEM_SPECTRE, PROP_CHRSPECTRE },
    { ITEM_M16, PROP_CHRM16 },
    { ITEM_FNP90, PROP_CHRFNP90 },
    { ITEM_SHOTGUN, PROP_CHRSHOTGUN },
    { ITEM_AUTOSHOT, PROP_CHRAUTOSHOT },
    { ITEM_SNIPERRIFLE, PROP_CHRSNIPERRIFLE },
    { ITEM_RUGER, PROP_CHRRUGER },
    { ITEM_GOLDENGUN, PROP_CHRGOLDEN },
    { ITEM_SILVERWPPK, PROP_CHRWPPK },
    { ITEM_GOLDWPPK, PROP_CHRWPPK },
    { ITEM_LASER, PROP_CHRLASER },
    { ITEM_GRENADELAUNCH, PROP_CHRGRENADELAUNCH },
    { ITEM_ROCKETLAUNCH, PROP_CHRROCKETLAUNCH },
    { ITEM_GRENADE, PROP_CHRGRENADE },
    { ITEM_TIMEDMINE, PROP_CHRTIMEDMINE },
    { ITEM_PROXIMITYMINE, PROP_CHRPROXIMITYMINE },
    { ITEM_REMOTEMINE, PROP_CHRREMOTEMINE }
};

/* Negative is clumsy, positive is lethal. -100 never connects. */
static const s8 kAccuracy[] = { -100, -60, -30, 0, 10, 25, 40, 70 };

extern s16 random_tracks[];

static int ready(void)
{
    return sSeed[0] != '\0';
}

static unsigned mix(unsigned h, unsigned v)
{
    h ^= v;
    h *= 16777619u;
    return h;
}

static unsigned seedHash(void)
{
    unsigned h = 2166136261u;
    const char *s;

    for (s = sSeed; *s != '\0'; s++)
    {
        h = mix(h, (unsigned char) *s);
    }
    return h;
}

static int roll(int salt, int who, int n)
{
    unsigned h;

    if (n <= 0)
    {
        return 0;
    }
    h = seedHash();
    h = mix(h, (unsigned) bossGetStageNum());
    h = mix(h, (unsigned) salt);
    h = mix(h, (unsigned) who);
    return (int) (h % (unsigned) n);
}

static void ensureStage(void)
{
    int stage = (int) bossGetStageNum();

    if (stage == sStage)
    {
        return;
    }
    sStage = stage;
    sLogged = 0;
    memset(sMark, 0, sizeof(sMark));
}

static int slotOf(ChrRecord *chr)
{
    int i;

    if (chr == NULL || g_ChrSlots == NULL || g_NumChrSlots <= 0)
    {
        return -1;
    }
    if (chr < g_ChrSlots)
    {
        return -1;
    }
    i = (int) (chr - g_ChrSlots);
    if (i < 0 || i >= g_NumChrSlots || i >= AP_MARK_MAX)
    {
        return -1;
    }
    return i;
}

/* The setup chrnum is shared by a lot of anonymous guards, and a guard who
 * appears during the mission often has none. The slot is one enemy. 0 stays
 * the grouped roll, so a real guard is his slot plus one. At plan time the
 * character does not exist yet: the allocator takes the first free slot,
 * which is the same number this returns. */
static int instanceId(ChrRecord *chr)
{
    int slot;
    int i;
    int n;

    if (chr != NULL)
    {
        slot = slotOf(chr);
        if (slot >= 0)
        {
            return slot + 1;
        }
    }
    if (g_ChrSlots == NULL || g_NumChrSlots <= 0)
    {
        return 1;
    }
    n = g_NumChrSlots;
    if (n > AP_MARK_MAX)
    {
        n = AP_MARK_MAX;
    }
    for (i = 0; i < n; i++)
    {
        if (g_ChrSlots[i].model == NULL)
        {
            return i + 1;
        }
    }
    return 1;
}

static int marked(ChrRecord *chr)
{
    int slot;

    ensureStage();
    slot = slotOf(chr);
    if (slot < 0)
    {
        return 0;
    }
    if (chr->chrflags & CHRFLAG_INVINCIBLE)
    {
        return 0;
    }
    if (chr->chrflags & CHRFLAG_COUNT_DEATH_AS_CIVILIAN)
    {
        return 0;
    }
    return sMark[slot] != 0;
}

static int inBodies(int body)
{
    int i;

    for (i = 0; i < (int) (sizeof(kBodies) / sizeof(kBodies[0])); i++)
    {
        if (kBodies[i] == body)
        {
            return 1;
        }
    }
    return 0;
}

static int inHeadList(const s32 *list, int head)
{
    int i;

    for (i = 0; list[i] >= 0; i++)
    {
        if (list[i] == head)
        {
            return 1;
        }
    }
    return 0;
}

static int genericHead(int head)
{
    if (head < 0)
    {
        return 1;
    }
    return inHeadList(random_male_heads, head) || inHeadList(random_female_heads, head);
}

static int gunIndex(int item)
{
    int i;

    for (i = 0; i < (int) (sizeof(kGuns) / sizeof(kGuns[0])); i++)
    {
        if (kGuns[i].item == item)
        {
            return i;
        }
    }
    return -1;
}

/* A give the cart uses to arm a guard, including a taser, which has no chr model of its own. */
static int isWeaponGive(int item)
{
    if (gunIndex(item) >= 0)
    {
        return 1;
    }
    return item == ITEM_TASER;
}

static int heldBack(int item)
{
    if (item == ITEM_KNIFE || item == ITEM_THROWKNIFE
        || item == ITEM_TIMEDMINE || item == ITEM_PROXIMITYMINE || item == ITEM_REMOTEMINE)
    {
        return 1;
    }
    if (!sLethal && (item == ITEM_GOLDENGUN || item == ITEM_SILVERWPPK || item == ITEM_GOLDWPPK))
    {
        return 1;
    }
    return 0;
}

static int weaponPick(ChrRecord *chr)
{
    int who = 0;
    int n = (int) (sizeof(kGuns) / sizeof(kGuns[0]));
    int pick;
    int guard = 0;

    if (sWeapons == AP_ENEMY_CHAOTIC)
    {
        who = instanceId(chr);
    }
    pick = roll(11, who, n);
    while (heldBack(kGuns[pick].item) && guard < n)
    {
        pick = (pick + 1) % n;
        guard++;
    }
    return pick;
}

static void logOnce(void)
{
    if (sLogged || !ready())
    {
        return;
    }
    sLogged = 1;
    portConLogf("ge: ap: enemies stage %d skins=%d heads=%d weapons=%d lethal=%d accuracy=%d music=%d\n",
                (int) bossGetStageNum(), sSkins, sHeads, sWeapons, sLethal, sAccuracy, sMusic);
}

void apEnemySetSeed(const char *seed)
{
    if (seed == NULL)
    {
        sSeed[0] = '\0';
        return;
    }
    snprintf(sSeed, sizeof(sSeed), "%s", seed);
}

void apEnemySetOptions(int skins, int heads, int weapons, int accuracy, int music, int lethal)
{
    sSkins = (skins == AP_ENEMY_GROUPED || skins == AP_ENEMY_CHAOTIC) ? skins : AP_ENEMY_OFF;
    sHeads = (heads == AP_ENEMY_GROUPED || heads == AP_ENEMY_CHAOTIC) ? heads : AP_ENEMY_OFF;
    sWeapons = (weapons == AP_ENEMY_GROUPED || weapons == AP_ENEMY_CHAOTIC) ? weapons : AP_ENEMY_OFF;
    sAccuracy = (accuracy == AP_ENEMY_GROUPED || accuracy == AP_ENEMY_CHAOTIC) ? accuracy : AP_ENEMY_OFF;
    sMusic = music ? 1 : 0;
    sLethal = lethal ? 1 : 0;
}

int apEnemyPlan(int body, int bodyRandom, int explicitHead, int invincible, int chrnum,
                int *outBody, int *outHead)
{
    int next;
    int who;
    int bodies;

    (void) chrnum;
    if (!ready() || invincible)
    {
        return 0;
    }
    if (!sSkins && !sHeads && !sWeapons && !sAccuracy)
    {
        return 0;
    }
    if (!bodyRandom && !inBodies(body))
    {
        return 0;
    }
    if (!genericHead(explicitHead))
    {
        return 0;
    }
    ensureStage();
    logOnce();
    who = 0;
    if (sSkins == AP_ENEMY_CHAOTIC || sHeads == AP_ENEMY_CHAOTIC
        || sWeapons == AP_ENEMY_CHAOTIC || sAccuracy == AP_ENEMY_CHAOTIC)
    {
        who = instanceId(NULL);
    }
    next = body;
    bodies = (int) (sizeof(kBodies) / sizeof(kBodies[0]));
    if (sSkins == AP_ENEMY_GROUPED)
    {
        next = kBodies[roll(1, 0, bodies)];
    }
    else if (sSkins == AP_ENEMY_CHAOTIC)
    {
        next = kBodies[roll(1, who, bodies)];
    }
    *outBody = next;
    *outHead = -1;
    if (sHeads == AP_ENEMY_GROUPED || sHeads == AP_ENEMY_CHAOTIC)
    {
        int headWho = (sHeads == AP_ENEMY_CHAOTIC) ? who : 0;

        if (next >= 0 && next < BODIES_MAX && c_item_entries[next].isMale == 0)
        {
            if (num_female_heads > 0)
            {
                *outHead = random_female_heads[roll(3, headWho, (int) num_female_heads)];
            }
        }
        else if (num_male_heads > 0)
        {
            *outHead = random_male_heads[roll(2, headWho, (int) num_male_heads)];
        }
    }
    return 1;
}

void apEnemyCommit(void *chrp)
{
    int slot;
    ChrRecord *chr = (ChrRecord *) chrp;

    ensureStage();
    slot = slotOf(chr);
    if (slot >= 0)
    {
        sMark[slot] = 1;
        apEnemyApplyAccuracy(chr);
    }
}

int apEnemyTakeGive(void *chrp, int model, int item, int flags, void **outProp)
{
    ChrRecord *chr = (ChrRecord *) chrp;
    int pick;
    PropRecord *prop;

    (void) model;
    if (outProp != NULL)
    {
        *outProp = NULL;
    }
    if (!ready() || sWeapons == AP_ENEMY_OFF || !marked(chr) || !isWeaponGive(item))
    {
        return 0;
    }
    if (chr->weapons_held[GUNRIGHT] != NULL)
    {
        if (outProp != NULL)
        {
            *outProp = chr->weapons_held[GUNRIGHT];
        }
        return 1;
    }
    if (chr->weapons_held[GUNLEFT] != NULL)
    {
        if (outProp != NULL)
        {
            *outProp = chr->weapons_held[GUNLEFT];
        }
        return 1;
    }
    pick = weaponPick(chr);
    prop = chrGiveWeapon(chr, kGuns[pick].prop, (ITEM_IDS) kGuns[pick].item, flags);
    if (outProp != NULL)
    {
        *outProp = prop;
    }
    return 1;
}

/* Setup weapons are attached after the guard exists, by chr id, and never
 * pass through the AI give. Without this, every placed guard keeps the
 * level's gun and only later spawns are rolled. */
int apEnemyRewriteAssigned(void *chrp, int *item, int *propModel)
{
    ChrRecord *chr = (ChrRecord *) chrp;
    int pick;

    if (!ready() || sWeapons == AP_ENEMY_OFF || chr == NULL || item == NULL || propModel == NULL)
    {
        return 0;
    }
    if (!marked(chr))
    {
        return 0;
    }
    if (gunIndex(*item) < 0 && *item != ITEM_TASER)
    {
        return 0;
    }
    pick = weaponPick(chr);
    *item = (int) kGuns[pick].item;
    *propModel = (int) kGuns[pick].prop;
    return 1;
}

void apEnemyApplyAccuracy(void *chrp)
{
    ChrRecord *chr = (ChrRecord *) chrp;
    int who = 0;
    int n;

    if (!ready() || sAccuracy == AP_ENEMY_OFF || !marked(chr))
    {
        return;
    }
    if (sAccuracy == AP_ENEMY_CHAOTIC)
    {
        who = instanceId(chr);
    }
    n = (int) (sizeof(kAccuracy) / sizeof(kAccuracy[0]));
    chr->accuracyrating = kAccuracy[roll(5, who, n)];
}

void apEnemySpareNext(void)
{
    sSpareNext = 1;
}

void apEnemySpareCancel(void)
{
    sSpareNext = 0;
}

void apEnemyForget(void *chrp)
{
    int slot = slotOf((ChrRecord *) chrp);

    if (slot >= 0)
    {
        sMark[slot] = 0;
    }
}

void apEnemyArmIfBare(void *chrp)
{
    ChrRecord *chr = (ChrRecord *) chrp;
    int pick;

    if (sSpareNext)
    {
        sSpareNext = 0;
        return;
    }
    if (!ready() || sWeapons == AP_ENEMY_OFF || !marked(chr))
    {
        return;
    }
    if (chr->weapons_held[GUNRIGHT] != NULL || chr->weapons_held[GUNLEFT] != NULL)
    {
        return;
    }
    pick = weaponPick(chr);
    chrGiveWeapon(chr, kGuns[pick].prop, (ITEM_IDS) kGuns[pick].item, 0);
}

int apEnemyMusic(int stageId)
{
    int count;

    (void) stageId;
    if (!ready() || !sMusic)
    {
        return -1;
    }
    count = 0;
    while (random_tracks[count] != M_NONE)
    {
        count++;
    }
    if (count <= 0)
    {
        return -1;
    }
    return (int) random_tracks[roll(6, 0, count)];
}
