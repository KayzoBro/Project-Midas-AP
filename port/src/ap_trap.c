/**
 * Baron Samedi and Greasy Hands.
 * Both run from the same consumable path as the other traps, including one
 * that was received in the menu and waited for the mission.
 */
#include "ap_trap.h"
#include "ap_enemy.h"

#include <ultra64.h>
#include <bondconstants.h>
#include <bondtypes.h>
#include <bondaicommands.h>

#include "music.h"
#include "snd.h"
#include "game/bondinv.h"
#include "game/chr.h"
#include "game/chraction.h"
#include "game/chrai.h"
#include "game/gun.h"
#include "game/objecthandler.h"
#include "game/player.h"
#include "game/propobj.h"

extern void portConLogf(const char *fmt, ...);
extern PropRecord *chrSpawnAtCoord(s32 bodynum, s32 headnum, coord3d *pos, StandTile *stan, f32 angle, AIListRecord *ailist, s32 spawnflags);
extern s32 chrGetNumFree(void);
extern void chrpropDelist(PropRecord *prop);
extern void chrpropDisable(PropRecord *prop);
extern void chrpropFree(PropRecord *prop);
extern PROP getPropForHeldItem(ITEM_IDS item);

/* Anonymous combat uniforms. Named characters, scientists and civilians are
 * not in this list, so a trap never removes them to make room. */
static const s32 kGuards[] = {
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

static int guardBody(int body)
{
    int i;

    for (i = 0; i < (int) (sizeof(kGuards) / sizeof(kGuards[0])); i++)
    {
        if (kGuards[i] == body)
        {
            return 1;
        }
    }
    return 0;
}

static int disposable(ChrRecord *chr)
{
    PropRecord *player;

    if (chr == NULL || chr->model == NULL || chr->prop == NULL)
    {
        return 0;
    }
    player = getCurrentPlayerProp();
    if (player != NULL && chr->prop == player)
    {
        return 0;
    }
    if (chr->chrflags & (CHRFLAG_INVINCIBLE | CHRFLAG_COUNT_DEATH_AS_CIVILIAN))
    {
        return 0;
    }
    if (chr->bodynum == BODY_Baron_Samedi)
    {
        return 0;
    }
    return guardBody(chr->bodynum);
}

static ChrRecord *farthestGuard(void)
{
    ChrRecord *best = NULL;
    f32 bestScore = -1.0f;
    PropRecord *player;
    int i;

    player = getCurrentPlayerProp();
    if (player == NULL || g_ChrSlots == NULL)
    {
        return NULL;
    }
    for (i = 0; i < g_NumChrSlots; i++)
    {
        ChrRecord *chr = &g_ChrSlots[i];
        f32 dx;
        f32 dy;
        f32 dz;
        f32 score;

        if (!disposable(chr))
        {
            continue;
        }
        dx = chr->prop->pos.x - player->pos.x;
        dy = chr->prop->pos.y - player->pos.y;
        dz = chr->prop->pos.z - player->pos.z;
        score = dx * dx + dy * dy + dz * dz;
        /* A corpse is already out of the fight. Prefer one over a live guard. */
        if (chr->actiontype == ACT_DIE || chr->actiontype == ACT_DEAD)
        {
            score += 1000000000.0f;
        }
        if (score > bestScore)
        {
            bestScore = score;
            best = chr;
        }
    }
    return best;
}

/* chrGetNumFree only cares that model == NULL. Cleanup does that, but the
 * prop is still on the active list, and the next chrTick follows a null
 * model. Delist here, in this call, so the slot is actually free. */
static void retireGuard(ChrRecord *chr)
{
    PropRecord *prop = chr->prop;

    chrpropCleanupForRemoval(prop);
    chrpropDelist(prop);
    chrpropDisable(prop);
    chrpropFree(prop);
}

static int disposableCount(void)
{
    int n = 0;
    int i;

    if (g_ChrSlots == NULL)
    {
        return 0;
    }
    for (i = 0; i < g_NumChrSlots; i++)
    {
        if (disposable(&g_ChrSlots[i]))
        {
            n++;
        }
    }
    return n;
}

static int makeRoom(void)
{
    int need = 3 - (int) chrGetNumFree();
    int guard = 0;

    if (need <= 0)
    {
        return 1;
    }
    /* Removing guards and still coming up short would leave the stage
     * without them and without Baron. */
    if (disposableCount() < need)
    {
        return 0;
    }
    while (chrGetNumFree() < 3)
    {
        s32 before = chrGetNumFree();
        ChrRecord *chr = farthestGuard();

        if (chr == NULL)
        {
            return 0;
        }
        retireGuard(chr);
        if (chrGetNumFree() <= before)
        {
            return 0;
        }
        if (++guard > g_NumChrSlots)
        {
            return 0;
        }
    }
    return 1;
}

int apTrapBaron(void)
{
    PropRecord *player;
    PropRecord *spawned;
    ChrRecord *baron;
    AIRecord *guard;

    player = getCurrentPlayerProp();
    if (player == NULL || player->stan == NULL || g_CurrentPlayer == NULL)
    {
        portConLogf("ge: ap: baron had no player to spawn beside\n");
        return 0;
    }
    if (!makeRoom())
    {
        portConLogf("ge: ap: baron needed a free character slot\n");
        return 0;
    }
    /* Bit 0x10 is the spawn command's "already on screen" allowance. Without
     * it the engine only accepts a spot the camera cannot see, so a Baron
     * beside Bond is refused in every room. */
    {
        coord3d pos;

        pos.x = player->pos.x - g_CurrentPlayer->vv_sintheta * 80.0f;
        pos.y = player->pos.y;
        pos.z = player->pos.z + g_CurrentPlayer->vv_costheta * 80.0f;
        spawned = chrSpawnAtCoord(BODY_Baron_Samedi, -1, &pos, player->stan, 0.0f, NULL, 0x10);
    }
    if (spawned == NULL || spawned->chr == NULL)
    {
        portConLogf("ge: ap: baron spawn returned nothing\n");
        return 0;
    }
    baron = spawned->chr;
    baron->chrflags |= CHRFLAG_INVINCIBLE;
    chrGiveWeapon(baron, PROP_CHRUZI, ITEM_UZI, 0);
    /* A copied guard list starts at its own pad script, which is why he
     * stood where he appeared. The global guard list is the one that
     * runs to Bond and shoots once it sees him. It is loaded on every stage. */
    guard = ailistFindById(GAILIST_STANDARD_GUARD);
    if (guard != NULL)
    {
        baron->ailist = guard;
        baron->aioffset = 0;
    }
    baron->alertness = 255;
    chrlvAlertGuardToPlayerPosition(baron);
    if (g_musicSfxBufferPtr != NULL)
    {
        sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, EVIL_LAUGH_SFX, 0);
    }
    return 1;
}

static int heldProp(ITEM_IDS id)
{
    /* PROP is unsigned. -1, the "no model" result, has to be read as signed. */
    return (s32) getPropForHeldItem(id) >= 0;
}

static int dropGun(GUNHAND hand, ITEM_IDS id)
{
    PROP modelnum;

    modelnum = getPropForHeldItem(id);
    if ((s32) modelnum < 0)
    {
        return 0;
    }
    return gunThrowItem(hand, (s32) modelnum, (s32) id);
}

static void toFists(GUNHAND hand)
{
    currentPlayerUnEquipWeaponWrapper(hand, ITEM_FIST);
    g_CurrentPlayer->hands[hand].previous_weapon = ITEM_FIST;
    g_CurrentPlayer->hands[hand].weapon_next_weapon = ITEM_FIST;
}

void apTrapGreasy(void)
{
    ITEM_IDS right;
    ITEM_IDS left;
    int rightGun;
    int leftGun;

    if (g_CurrentPlayer == NULL)
    {
        return;
    }
    right = getCurrentPlayerWeaponId(GUNRIGHT);
    left = getCurrentPlayerWeaponId(GUNLEFT);
    rightGun = heldProp(right);
    leftGun = heldProp(left);
    if (!rightGun && !leftGun)
    {
        return;
    }
    /* The watch entry goes only after the prop is on the floor. Ownership in
     * the seed stays, so the next mission's loadout gives an owned gun back,
     * and picking this one up does too. */
    if (rightGun)
    {
        rightGun = dropGun(GUNRIGHT, right);
    }
    if (leftGun)
    {
        leftGun = dropGun(GUNLEFT, left);
    }
    if (rightGun)
    {
        bondinvRemoveItemByID(right);
        toFists(GUNRIGHT);
    }
    if (leftGun && left != right)
    {
        bondinvRemoveItemByID(left);
    }
    if (leftGun || (rightGun && left == right))
    {
        toFists(GUNLEFT);
    }
}

static void stripHands(ChrRecord *chr)
{
    int hand;

    for (hand = 0; hand < 2; hand++)
    {
        PropRecord *held = chr->weapons_held[hand];

        if (held == NULL || held->obj == NULL)
        {
            continue;
        }
        objDetach(held);
        objFreePermanently(held->obj, TRUE);
    }
}

int apTrapBoris(void)
{
    PropRecord *player;
    PropRecord *spawned;
    ChrRecord *boris;
    AIRecord *idle;

    player = getCurrentPlayerProp();
    if (player == NULL || player->stan == NULL || g_CurrentPlayer == NULL)
    {
        return 0;
    }
    if (!makeRoom())
    {
        return 0;
    }
    {
        coord3d pos;

        /* In front of Bond, close enough to be seen, same on-screen bit as Baron.
         * Spare runs first: chrAllocate arms a bare slot before we get the pointer,
         * and that slot can still carry the previous guard's weapon roll. */
        pos.x = player->pos.x - g_CurrentPlayer->vv_sintheta * 80.0f;
        pos.y = player->pos.y;
        pos.z = player->pos.z + g_CurrentPlayer->vv_costheta * 80.0f;
        apEnemySpareNext();
        spawned = chrSpawnAtCoord(BODY_Boris, -1, &pos, player->stan, 0.0f, NULL, 0x10);
    }
    if (spawned == NULL || spawned->chr == NULL)
    {
        apEnemySpareCancel();
        return 0;
    }
    boris = spawned->chr;
    apEnemyForget(boris);
    stripHands(boris);
    boris->chrflags |= CHRFLAG_INVINCIBLE;
    /* Zero sight so the standard guard list stays on its idle loop instead
     * of running over to shoot. He is the joke, not a fight. */
    boris->visionrange = 0.0f;
    boris->alertness = 0;
    idle = ailistFindById(GAILIST_STANDARD_GUARD);
    if (idle != NULL)
    {
        boris->ailist = idle;
        boris->aioffset = 0;
    }
    return 1;
}
