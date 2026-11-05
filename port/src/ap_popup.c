/**
 * Short toasts for Archipelago receipts. Drawn in the console's 2D space so
 * the corner follows the window, not the 4:3 mission view.
 * An item line carries the Archipelago flower: gold for progression or a
 * trap, colour for useful, grey for filler.
 */
#ifndef TARGET_N64

#include <stdio.h>
#include <string.h>
#include <ultra64.h>

#include "ap_client.h"
#include "ap_popup_icon.h"
#include "game/textrelated.h"
#include "game/othermodemicrocode.h"
#include "game/bondwalk2.h"
#include "game/dyn.h"
#include "fr.h"

#define AP_POP_N 4
/* Wall time, not frames: a high frame cap was clearing a toast in under a second. */
#define AP_POP_MS 8000
#define AP_POP_LIFE ((OSTime) AP_POP_MS * 46875ull)
#define AP_POP_CHARS 96

static const char *const kPosName[] = { "top_right", "top_left", "bottom_right", "bottom_left" };
static const char *const kPosLabel[] = { "TOP RIGHT", "TOP LEFT", "BOTTOM RIGHT", "BOTTOM LEFT" };

static int sOn = 1;
static int sPos = 3;
static int sCount;
static char sText[AP_POP_N][AP_POP_CHARS];
static OSTime sLife[AP_POP_N];
static int sMine[AP_POP_N];
/* 0 gold, 1 colour, 2 grey, -1 no flower. */
static int sMark[AP_POP_N];
/* Countdown lines for timed traps. They stay until the effect ends. */
#define AP_STAT_N 5
static char sStat[AP_STAT_N][AP_POP_CHARS];
static int sStatMark[AP_STAT_N];
static int sStatOn[AP_STAT_N];

static int eqPos(const char *a, const char *b)
{
    int i;
    if (a == NULL || b == NULL)
    {
        return 0;
    }
    for (i = 0; a[i] != '\0' && b[i] != '\0'; i++)
    {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = (char) (ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char) (cb - 'A' + 'a');
        if (ca == '-' || ca == ' ') ca = '_';
        if (cb == '-' || cb == ' ') cb = '_';
        if (ca != cb)
        {
            return 0;
        }
    }
    return a[i] == '\0' && b[i] == '\0';
}

static void keepAscii(char *dst, int n, const char *src)
{
    int i, o = 0;
    if (n < 2)
    {
        if (n > 0) dst[0] = '\0';
        return;
    }
    for (i = 0; src != NULL && src[i] != '\0' && o < n - 1; i++)
    {
        unsigned char c = (unsigned char) src[i];
        if (c < 0x20 || c >= 0x7F)
        {
            c = ' ';
        }
        if (c == ' ' && (o == 0 || dst[o - 1] == ' '))
        {
            continue;
        }
        dst[o++] = (char) c;
    }
    while (o > 0 && dst[o - 1] == ' ')
    {
        o--;
    }
    dst[o] = '\0';
}

int apGetPopup(void)
{
    return sOn;
}

void apPopupSetEnabled(int on)
{
    sOn = on ? 1 : 0;
    if (!sOn)
    {
        sCount = 0;
    }
}

const char *apPopupPosName(void)
{
    if (sPos < 0 || sPos > 3)
    {
        sPos = 3;
    }
    return kPosName[sPos];
}

const char *apPopupPosLabel(void)
{
    return kPosLabel[sPos >= 0 && sPos <= 3 ? sPos : 3];
}

void apPopupSetPosName(const char *name)
{
    int i;
    if (name == NULL)
    {
        return;
    }
    if (eqPos(name, "tr") || eqPos(name, "0")) { sPos = 0; return; }
    if (eqPos(name, "tl") || eqPos(name, "1")) { sPos = 1; return; }
    if (eqPos(name, "br") || eqPos(name, "2")) { sPos = 2; return; }
    if (eqPos(name, "bl") || eqPos(name, "3")) { sPos = 3; return; }
    for (i = 0; i < 4; i++)
    {
        if (eqPos(name, kPosName[i]) || eqPos(name, kPosLabel[i]))
        {
            sPos = i;
            return;
        }
    }
}

int apPopupPosIndex(void)
{
    if (sPos < 0 || sPos > 3)
    {
        sPos = 3;
    }
    return sPos;
}

void apPopupSetPosIndex(int pos)
{
    if (pos < 0 || pos > 3)
    {
        return;
    }
    sPos = pos;
}

void apPopupCyclePos(int dir)
{
    sPos += (dir < 0) ? 3 : 1;
    sPos &= 3;
}

static int markOf(int flags)
{
    /* A trap is a fake progression: same gold flower as a dossier or a tool. */
    if (flags < 0)
    {
        return -1;
    }
    if (flags & (AP_POP_PROGRESSION | AP_POP_TRAP))
    {
        return 0;
    }
    if (flags & AP_POP_USEFUL)
    {
        return 1;
    }
    return 2;
}

void apPopupPush(const char *text, int mine, int flags)
{
    char line[AP_POP_CHARS];
    if (!sOn || text == NULL)
    {
        return;
    }
    keepAscii(line, (int) sizeof(line), text);
    if (line[0] == '\0')
    {
        return;
    }
    if (sCount == AP_POP_N)
    {
        int i;
        for (i = 1; i < AP_POP_N; i++)
        {
            snprintf(sText[i - 1], sizeof(sText[i - 1]), "%s", sText[i]);
            sLife[i - 1] = sLife[i];
            sMine[i - 1] = sMine[i];
            sMark[i - 1] = sMark[i];
        }
        sCount--;
    }
    snprintf(sText[sCount], sizeof(sText[sCount]), "%s", line);
    sLife[sCount] = osGetTime() + AP_POP_LIFE;
    sMine[sCount] = mine ? 1 : 0;
    sMark[sCount] = markOf(flags);
    sCount++;
}

void apPopupTimer(int slot, const char *text, int flags)
{
    if (slot < 0 || slot >= AP_STAT_N)
    {
        return;
    }
    if (text == NULL || text[0] == '\0')
    {
        sStatOn[slot] = 0;
        return;
    }
    keepAscii(sStat[slot], (int) sizeof(sStat[slot]), text);
    if (sStat[slot][0] == '\0')
    {
        sStatOn[slot] = 0;
        return;
    }
    sStatOn[slot] = 1;
    sStatMark[slot] = markOf(flags);
}

void apPopupTick(void)
{
    OSTime now;
    int i, w = 0;

    /* A connect can land before the menu font exists. Hold the lines so
     * the eight seconds start once a screen can actually draw them. */
    if (ptrFontBankGothic == NULL || ptrFontBankGothicChars == NULL)
    {
        OSTime later = osGetTime() + AP_POP_LIFE;
        for (i = 0; i < sCount; i++)
        {
            sLife[i] = later;
        }
        return;
    }
    now = osGetTime();
    for (i = 0; i < sCount; i++)
    {
        if (sLife[i] > now)
        {
            if (w != i)
            {
                snprintf(sText[w], sizeof(sText[w]), "%s", sText[i]);
                sLife[w] = sLife[i];
                sMine[w] = sMine[i];
                sMark[w] = sMark[i];
            }
            w++;
        }
    }
    sCount = w;
}

#define AP_SPACE(gdl, w, h) do { (gdl)->words.w0 = (u32) 0x4c << 24; \
    (gdl)->words.w1 = ((u32) (w) << 16) | (u32) (h); (gdl)++; } while (0)
#define AP_ICON_HDR 16

static struct sImageTableEntry sIcon[AP_ICON_N];
static int sIconState;

static int iconReady(void)
{
    extern void *portAllocHigh(u32 size);
    int k;

    if (sIconState != 0)
    {
        return sIconState > 0;
    }
    for (k = 0; k < AP_ICON_N; k++)
    {
        u8 *blob;
        u32 *px;
        int i;

        /* Below 0x10000000 fast3d reads the pointer as a segment. The 16
         * bytes in front are 0xFF so texSelect's pool lookup (u16 at -8)
         * misses and uses this entry. */
        blob = (u8 *) portAllocHigh(AP_ICON_HDR + AP_ICON_PX * AP_ICON_PX * 4);
        if (blob == NULL)
        {
            extern void portConLogf(const char *fmt, ...);

            portConLogf("ge: ap: popup logo had no high memory\n");
            sIconState = -1;
            return 0;
        }
        memset(blob, 0xFF, AP_ICON_HDR);
        px = (u32 *) (blob + AP_ICON_HDR);
        for (i = 0; i < AP_ICON_PX; i++)
        {
            int x;
            /* HUD sprites are bottom-up and drawn with flipY, same as the
             * ammo icons. The baked rows are top-first. */
            for (x = 0; x < AP_ICON_PX; x++)
            {
                px[(AP_ICON_PX - 1 - i) * AP_ICON_PX + x] = ap_icon_px[k][i * AP_ICON_PX + x];
            }
        }
        memset(&sIcon[k], 0, sizeof(sIcon[k]));
        sIcon[k].index = (u32) (uintptr_t) px;
        sIcon[k].width = AP_ICON_PX;
        sIcon[k].height = AP_ICON_PX;
        sIcon[k].format = G_IM_FMT_RGBA;
        sIcon[k].depth = G_IM_SIZ_32b;
        sIcon[k].flagsS = G_TX_CLAMP;
        sIcon[k].flagsT = G_TX_CLAMP;
    }
    sIconState = 1;
    return 1;
}

static Gfx *drawLogo(Gfx *gdl, int mark, s32 x, s32 y, s32 box)
{
    f32 xy[2];
    f32 half[2];

    if (mark < 0 || mark >= AP_ICON_N || box <= 0 || !iconReady())
    {
        return gdl;
    }
    texSelect(&gdl, &sIcon[mark], 2, 0, 0);
    half[0] = (f32) box * 0.5f;
    half[1] = half[0];
    xy[0] = (f32) x + half[0];
    xy[1] = (f32) y + half[1];
    /* Bottom-up in memory, so flipY matches the other HUD sprites. */
    display_image_at_position(&gdl, xy, half, sIcon[mark].width, sIcon[mark].height,
                              0, 0, 1, 255, 255, 255, 255, 0, 0);
    /* textRender leaves the combiner alone. Put the font one back. */
    gDPPipeSync(gdl++);
    gDPSetCombineLERP(gdl++,
            0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0,
            0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    return gdl;
}

Gfx *apPopupRender(Gfx *gdl)
{
    extern void portConsoleSpace(s32 *w, s32 *h);
    s32 spaceW = 640, spaceH = 480;
    s16 savedX, savedY;
    s32 textW[AP_POP_N + AP_STAT_N];
    char *line[AP_POP_N + AP_STAT_N];
    int lineMark[AP_POP_N + AP_STAT_N];
    int lineMine[AP_POP_N + AP_STAT_N];
    int n = 0;
    s32 width = 0;
    s32 box;
    s32 lead = 0;
    s32 base;
    s32 cap;
    s32 iconRel;
    s32 rowTop;
    s32 rowH;
    s32 i;
    s32 x0, y0, pitch;
    const s32 margin = 16;
    const s32 gap = 6;
    const s32 pad = 5;
    const struct fontchar *capChar;

    if (ptrFontBankGothic == NULL || ptrFontBankGothicChars == NULL)
    {
        return gdl;
    }
    if (sOn)
    {
        for (i = 0; i < sCount && n < AP_POP_N + AP_STAT_N; i++)
        {
            line[n] = sText[i];
            lineMark[n] = sMark[i];
            lineMine[n] = sMine[i];
            n++;
        }
    }
    for (i = 0; i < AP_STAT_N && n < AP_POP_N + AP_STAT_N; i++)
    {
        if (!sStatOn[i])
        {
            continue;
        }
        line[n] = sStat[i];
        lineMark[n] = sStatMark[i];
        lineMine[n] = 1;
        n++;
    }
    if (n <= 0)
    {
        return gdl;
    }
    if (dynGetFreeGfx2(gdl) < 80 + n * 48)
    {
        return gdl;
    }

    portConsoleSpace(&spaceW, &spaceH);
    savedX = viGetX();
    savedY = viGetY();
    viSetXY((s16) spaceW, (s16) spaceH);
    AP_SPACE(gdl, spaceW, spaceH);
    gdl = microcode_constructor(gdl);
    gDPSetScissor(gdl++, G_SC_NON_INTERLACE, 0, 0, spaceW, spaceH);

    for (i = 0; i < n; i++)
    {
        s32 h = 0;

        textW[i] = 0;
        textMeasure(&h, &textW[i], line[i], ptrFontBankGothicChars, ptrFontBankGothic, 0);
        if (textW[i] > width)
        {
            width = textW[i];
        }
        if (lineMark[i] >= 0)
        {
            lead = 1;
        }
    }
    /* textMeasure stays at 0 without a newline, and the pen is not the top of
     * the ink. Capitals share a baseline, so the flower is centred on that. */
    capChar = &ptrFontBankGothicChars['A' - 0x21];
    base = capChar->baseline;
    cap = capChar->height;
    if (cap < 8)
    {
        cap = 8;
    }
    box = cap;
    if (box < 14)
    {
        box = 14;
    }
    if (box > AP_ICON_PX)
    {
        box = AP_ICON_PX;
    }
    iconRel = base + (cap - box) / 2;
    rowTop = iconRel < base ? iconRel : base;
    rowH = iconRel + box;
    if (base + cap > rowH)
    {
        rowH = base + cap;
    }
    rowH -= rowTop;
    if (lead && !iconReady())
    {
        lead = 0;
    }
    if (lead)
    {
        lead = box + gap;
        width += lead;
    }
    if (width > spaceW - margin * 2 - 16)
    {
        width = spaceW - margin * 2 - 16;
    }
    pitch = rowH + 3;
    if (sPos & 1)
    {
        x0 = margin;
    }
    else
    {
        x0 = spaceW - margin - width - 12;
        if (x0 < margin)
        {
            x0 = margin;
        }
    }
    if (sPos >= 2)
    {
        y0 = spaceH - margin - (n * pitch - 3);
    }
    else
    {
        y0 = margin;
    }
    if (y0 < pad - rowTop)
    {
        y0 = pad - rowTop;
    }

    gdl = microcode_constructor_related_to_menus(gdl, x0 - pad, y0 - pad,
                                                 x0 + width + pad, y0 + n * pitch - 3 + pad, 0xC0);
    for (i = 0; i < n; i++)
    {
        s32 ty = y0 + i * pitch - rowTop;
        s32 tx = x0 + lead;
        u32 colour = lineMine[i] ? 0xE8D080FFu : 0xC8DCF8FFu;

        if (lead > 0 && lineMark[i] >= 0)
        {
            gdl = drawLogo(gdl, lineMark[i], x0, ty + iconRel, box);
        }
        gdl = textRender(gdl, &tx, &ty, line[i], ptrFontBankGothicChars, ptrFontBankGothic,
                         colour, spaceW, spaceH, 0, 0);
    }

    gdl = combiner_bayer_lod_perspective(gdl);
    AP_SPACE(gdl, 0, 0);
    viSetXY(savedX, savedY);
    return gdl;
}

static Gfx *supplyFill(Gfx *gdl, s32 x0, s32 y0, s32 x1, s32 y1, u32 color)
{
    return microcode_constructor_related_to_menus(gdl, x0, y0, x1, y1, color);
}

static void supplyKey(char *dst, int n, int which)
{
    const char *s = apSupplyKeyLabel(which);
    int i = 0;

    if (n < 1)
    {
        return;
    }
    if (s == NULL)
    {
        dst[0] = '\0';
        return;
    }
    while (s[i] != '\0' && s[i] != '\n' && i < n - 1)
    {
        dst[i] = s[i];
        i++;
    }
    dst[i] = '\0';
}

Gfx *apSupplyRender(Gfx *gdl)
{
    extern void portConsoleSpace(s32 *w, s32 *h);
    s32 spaceW = 640, spaceH = 480;
    s16 savedX, savedY;
    char count[2][16];
    char key[2][32];
    s32 labelW[2];
    s32 countW[2];
    s32 keyW[2];
    s32 panelR;
    s32 x0, y0, i;
    const s32 margin = 18;
    const s32 pitch = 22;
    static const char *const supplyLabel[2] = { "Arm.", "Amm." };

    if (!apSupplyHudOn() || ptrFontBankGothic == NULL || ptrFontBankGothicChars == NULL)
    {
        return gdl;
    }
    if (dynGetFreeGfx2(gdl) < 160)
    {
        return gdl;
    }

    portConsoleSpace(&spaceW, &spaceH);
    savedX = viGetX();
    savedY = viGetY();
    viSetXY((s16) spaceW, (s16) spaceH);
    AP_SPACE(gdl, spaceW, spaceH);
    gdl = microcode_constructor(gdl);
    gDPSetScissor(gdl++, G_SC_NON_INTERLACE, 0, 0, spaceW, spaceH);

    panelR = 0;
    for (i = 0; i < 2; i++)
    {
        s32 h = 0;
        snprintf(count[i], sizeof(count[i]), " : %d", apSupplyCount(i));
        supplyKey(key[i], (int) sizeof(key[i]), i);
        textMeasure(&h, &labelW[i], supplyLabel[i], ptrFontBankGothicChars, ptrFontBankGothic, 0);
        textMeasure(&h, &countW[i], count[i], ptrFontBankGothicChars, ptrFontBankGothic, 0);
        keyW[i] = 0;
        if (key[i][0] != '\0' && strcmp(key[i], "None") != 0 && strcmp(key[i], "Press a Key") != 0)
        {
            textMeasure(&h, &keyW[i], key[i], ptrFontBankGothicChars, ptrFontBankGothic, 0);
        }
        else
        {
            key[i][0] = '\0';
        }
        if (labelW[i] + countW[i] + (keyW[i] > 0 ? 10 + keyW[i] : 0) > panelR)
        {
            panelR = labelW[i] + countW[i] + (keyW[i] > 0 ? 10 + keyW[i] : 0);
        }
    }

    x0 = margin;
    y0 = margin;
    gdl = supplyFill(gdl, x0 - 8, y0 - 6, x0 + panelR + 10, y0 + pitch * 2 + 4, 0x000000B0u);
    for (i = 0; i < 2; i++)
    {
        s32 tx = x0;
        s32 ty = y0 + i * pitch;
        u32 labelColour = (i == 0) ? 0x8EC4F0FFu : 0xE8D080FFu;

        gdl = textRender(gdl, &tx, &ty, supplyLabel[i], ptrFontBankGothicChars, ptrFontBankGothic,
                         labelColour, spaceW, spaceH, 0, 0);
        tx = x0 + labelW[i];
        ty = y0 + i * pitch;
        gdl = textRender(gdl, &tx, &ty, count[i], ptrFontBankGothicChars, ptrFontBankGothic,
                         0xF4F4F4FFu, spaceW, spaceH, 0, 0);
        if (key[i][0] != '\0')
        {
            tx = x0 + labelW[i] + countW[i] + 8;
            ty = y0 + i * pitch;
            gdl = textRender(gdl, &tx, &ty, key[i], ptrFontBankGothicChars, ptrFontBankGothic,
                             0xE8D080FFu, spaceW, spaceH, 0, 0);
        }
    }

    gdl = combiner_bayer_lod_perspective(gdl);
    AP_SPACE(gdl, 0, 0);
    viSetXY(savedX, savedY);
    return gdl;
}

#else

void apPopupSetEnabled(int on) { (void) on; }
const char *apPopupPosName(void) { return "bottom_left"; }
const char *apPopupPosLabel(void) { return "BOTTOM LEFT"; }
void apPopupSetPosName(const char *name) { (void) name; }
int apPopupPosIndex(void) { return 3; }
void apPopupSetPosIndex(int pos) { (void) pos; }
void apPopupCyclePos(int dir) { (void) dir; }
void apPopupPush(const char *text, int mine, int flags) { (void) text; (void) mine; (void) flags; }
void apPopupTimer(int slot, const char *text, int flags) { (void) slot; (void) text; (void) flags; }
void apPopupTick(void) {}
int apGetPopup(void) { return 0; }
Gfx *apPopupRender(Gfx *gdl) { return gdl; }
Gfx *apSupplyRender(Gfx *gdl) { return gdl; }

#endif
