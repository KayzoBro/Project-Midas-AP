/**
 * Room screen shown instead of the file select until a seed accepts the slot.
 * The cart's own folder (Dam already open) is not reachable from here.
 */
#include <ultra64.h>
#include <bondconstants.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ap_client.h"
#include "fr.h"
#include "joy.h"
#include "game/textrelated.h"
#include "music.h"

extern Gfx *frontPrintText(Gfx *gdl, s32 *x, s32 *y, s8 *text, s32 second_font_table, s32 first_font_table, s32 arg6, s32 view_x, s32 view_y, s32 arg9, s32 arga);
extern Gfx *frontDrawCursor(Gfx *gdl);
extern Gfx *frontSetupMenuBackground(Gfx *dl);
extern Gfx *microcode_constructor_related_to_menus(Gfx *gdl, s32 ulx, s32 uly, s32 lrx, s32 lry, u32 color);
extern void frontUpdateControlStickPosition(void);
extern void frontChangeMenu(MENU menu, s32 reload);
extern void load_walletbond(void);
extern void disable_all_switches(Model *arg0);
extern void set_item_visibility_in_objinstance(Model *objinstance, s32 item, s32 mode);
extern Model *walletinst[];
extern f32 cursor_h_pos;
extern f32 cursor_v_pos;

#define AP_MENU_ROWS 10
#define AP_ROW_HOST 0
#define AP_ROW_PORT 1
#define AP_ROW_SLOT 2
#define AP_ROW_PASSWORD 3
#define AP_ROW_TLS 4
#define AP_ROW_POPUP 5
#define AP_ROW_PLACE 6
#define AP_ROW_DEATH 7
#define AP_ROW_CONNECT 8
#define AP_ROW_PLAY 9
/* Folder paper, in the menu's 320x240 space. The title and the status each
 * take a full line of their own. The old status sat two pixels above HOST,
 * so the seed was drawn through the host name. */
#define AP_TITLE_Y 0x28
#define AP_ROW_Y0  0x4C
#define AP_ROW_PITCH 0x10
#define AP_HINT_Y  0xF2

static int sRow;
static int sBlink;
static char sHost[128];
static char sPort[8];
static char sSlot[48];
static char sPassword[80];
static int sTls;
static char sTyped[8];
static int sTypedN;
static int sUp;
static int sDown;
static int sLeft;
static int sRight;
static int sBack;
static int sEnter;

static void loadFields(void)
{
    apGetHost(sHost, (int) sizeof(sHost));
    apGetSlot(sSlot, (int) sizeof(sSlot));
    apGetPassword(sPassword, (int) sizeof(sPassword));
    snprintf(sPort, sizeof(sPort), "%d", apGetPort());
    sTls = apGetTls();
}

static char *fieldBuf(int row)
{
    if (row == AP_ROW_HOST) return sHost;
    if (row == AP_ROW_PORT) return sPort;
    if (row == AP_ROW_SLOT) return sSlot;
    if (row == AP_ROW_PASSWORD) return sPassword;
    return NULL;
}

static int fieldCap(int row)
{
    if (row == AP_ROW_HOST) return (int) sizeof(sHost);
    if (row == AP_ROW_PORT) return (int) sizeof(sPort);
    if (row == AP_ROW_SLOT) return (int) sizeof(sSlot);
    if (row == AP_ROW_PASSWORD) return (int) sizeof(sPassword);
    return 0;
}

static int fieldsReady(void)
{
    int port = atoi(sPort);

    return sHost[0] != '\0' && sSlot[0] != '\0' && port > 0 && port <= 65535;
}

static void connectNow(void)
{
    if (!fieldsReady())
    {
        return;
    }
    apApplyConnection(sHost, sSlot, sPassword, atoi(sPort), sTls);
    snprintf(sPort, sizeof(sPort), "%d", apGetPort());
    apGetHost(sHost, (int) sizeof(sHost));
}

static void moveRow(int dir)
{
    sRow += dir;
    if (sRow < 0) sRow = AP_MENU_ROWS - 1;
    if (sRow >= AP_MENU_ROWS) sRow = 0;
}

static int rowAtCursor(void)
{
    int i;

    for (i = 0; i < AP_MENU_ROWS; i++)
    {
        f32 top = (f32) (AP_ROW_Y0 + i * AP_ROW_PITCH) - 4.0f;
        if (cursor_v_pos >= top && cursor_v_pos < top + (f32) AP_ROW_PITCH)
        {
            return i;
        }
    }
    return -1;
}

/* The crosshair selects a row when it moves. A still cursor leaves a keyboard
 * move alone, so the arrows are not put back on the next frame. */
static void pickFromCursor(void)
{
    static f32 lastH;
    static f32 lastV;
    static int have;
    int row;

    if (have && cursor_h_pos == lastH && cursor_v_pos == lastV)
    {
        return;
    }
    have = 1;
    lastH = cursor_h_pos;
    lastV = cursor_v_pos;
    row = rowAtCursor();
    if (row >= 0)
    {
        sRow = row;
    }
}

void apMenuInit(void)
{
    sRow = AP_ROW_SLOT;
    sBlink = 0;
    sTypedN = 0;
    load_walletbond();
    loadFields();
    musicTrack1Play(M_FOLDERS);
}

void apMenuUpdate(void)
{
    sBlink++;
}

int apMenuWantsKeys(void)
{
    extern MENU current_menu;
    return current_menu == MENU_ARCHIPELAGO;
}

void apMenuText(unsigned char c)
{
    if (sTypedN < (int) sizeof(sTyped) - 1 && c >= 32 && c < 127)
    {
        if (sRow == AP_ROW_CONNECT || sRow == AP_ROW_PLAY || sRow == AP_ROW_TLS
            || sRow == AP_ROW_POPUP || sRow == AP_ROW_PLACE)
        {
            return;
        }
        sTyped[sTypedN++] = (char) c;
        sTyped[sTypedN] = '\0';
    }
}

void apMenuKey(int kind)
{
    /* 1 up, 2 down, 3 left, 4 right, 5 backspace, 6 enter */
    if (kind == 1) sUp = 1;
    else if (kind == 2) sDown = 1;
    else if (kind == 3) sLeft = 1;
    else if (kind == 4) sRight = 1;
    else if (kind == 5) sBack = 1;
    else if (kind == 6) sEnter = 1;
}

static void takeTyped(void)
{
    char *buf;
    int cap;
    int i;
    int n;

    if (sRow == AP_ROW_TLS || sRow == AP_ROW_POPUP || sRow == AP_ROW_PLACE
        || sRow == AP_ROW_DEATH || sRow == AP_ROW_CONNECT || sRow == AP_ROW_PLAY)
    {
        sTypedN = 0;
        return;
    }
    buf = fieldBuf(sRow);
    cap = fieldCap(sRow);
    n = (int) strlen(buf);
    for (i = 0; i < sTypedN && n < cap - 1; i++)
    {
        if (sRow == AP_ROW_PORT && (sTyped[i] < '0' || sTyped[i] > '9'))
        {
            continue;
        }
        buf[n++] = sTyped[i];
    }
    buf[n] = '\0';
    sTypedN = 0;
}

static void backspace(void)
{
    char *buf = fieldBuf(sRow);
    int n;

    if (buf == NULL)
    {
        return;
    }
    n = (int) strlen(buf);
    if (n > 0)
    {
        buf[n - 1] = '\0';
    }
}

void apMenuInterface(void)
{
    viSetFovY(FOV_Y_F);
    viSetAspect(ASPECT_RATIO_SD);
    viSetZRange(100.0f, 10000.0f);
    viSetUseZBuf(FALSE);
    if (walletinst[0] != NULL)
    {
        disable_all_switches(walletinst[0]);
        set_item_visibility_in_objinstance(walletinst[0], SW_TABS, 1);
        set_item_visibility_in_objinstance(walletinst[0], SW_PAPER, 1);
    }
    frontUpdateControlStickPosition();
    pickFromCursor();

    takeTyped();
    if (joyGetButtonsPressedThisFrame(PLAYER_1, U_JPAD | U_CBUTTONS)) sUp = 1;
    if (joyGetButtonsPressedThisFrame(PLAYER_1, D_JPAD | D_CBUTTONS)) sDown = 1;
    if (joyGetButtonsPressedThisFrame(PLAYER_1, L_JPAD | L_CBUTTONS)) sLeft = 1;
    if (joyGetButtonsPressedThisFrame(PLAYER_1, R_JPAD | R_CBUTTONS)) sRight = 1;
    if (sUp) { sUp = 0; moveRow(-1); }
    if (sDown) { sDown = 0; moveRow(1); }
    if (sBack) { sBack = 0; backspace(); }
    if (sLeft || sRight)
    {
        int dir = sRight ? 1 : -1;
        sLeft = sRight = 0;
        if (sRow == AP_ROW_TLS)
        {
            sTls = !sTls;
        }
        else if (sRow == AP_ROW_POPUP)
        {
            apSetPopup(!apGetPopup());
        }
        else if (sRow == AP_ROW_PLACE)
        {
            apCyclePopupPos(dir);
        }
        else if (sRow == AP_ROW_DEATH)
        {
            /* Live tag, not a new seed. The choice is kept in midas.ini. */
            apToggleDeathLink();
        }
    }
    {
        int mouseClick = joyGetButtonsPressedThisFrame(PLAYER_1, Z_TRIG);
        int confirm = sEnter || joyGetButtonsPressedThisFrame(PLAYER_1, A_BUTTON | START_BUTTON);
        int onRow = mouseClick && rowAtCursor() == sRow;

        sEnter = 0;
        /* A click on a text field only focuses it. Enter still steps to the next line. */
        if (!(onRow && sRow < AP_ROW_TLS) && (confirm || onRow))
        {
        if (sRow == AP_ROW_TLS)
        {
            sTls = !sTls;
        }
        else if (sRow == AP_ROW_POPUP)
        {
            apSetPopup(!apGetPopup());
        }
        else if (sRow == AP_ROW_PLACE)
        {
            apCyclePopupPos(1);
        }
        else if (sRow == AP_ROW_DEATH)
        {
            apToggleDeathLink();
        }
        else if (sRow == AP_ROW_CONNECT)
        {
            connectNow();
        }
        else if (sRow == AP_ROW_PLAY)
        {
            if (apSeedReady())
            {
                frontChangeMenu(MENU_FILE_SELECT, FALSE);
            }
        }
        else
        {
            moveRow(1);
        }
        }
    }
}

/* The seed is a 20-digit number. Left on one line it runs under the crest
 * and, when the rows start too high, through the host. Keep the tail. */
static void fitStatus(char *dst, int n, const char *src)
{
    const char *tail;
    int len;

    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }
    len = (int) strlen(src);
    if (strncmp(src, "Seed ", 5) == 0 && len > 16)
    {
        tail = src + len - 8;
        snprintf(dst, (size_t) n, "Seed ...%s", tail);
        return;
    }
    if (len >= n)
    {
        len = n - 1;
    }
    memcpy(dst, src, (size_t) len);
    dst[len] = '\0';
}

static Gfx *printLine(Gfx *dl, s32 x, s32 y, const char *text)
{
    char buf[160];

    /* 0xFF is black ink, which is what the folder pages use. On a bare black
     * clear it disappears, so fall back to white when the wallet is missing. */
    s32 ink = (walletinst[0] != NULL) ? 0xFF : (s32) 0xFFFFFFFF;

    snprintf(buf, sizeof(buf), "%s\n", text);
    return frontPrintText(dl, &x, &y, (s8 *) buf, (s32) ptrFontZurichBoldChars, (s32) ptrFontZurichBold,
                          ink, viGetX(), viGetY(), 0, 0);
}

Gfx *apMenuConstructor(Gfx *dl)
{
    static const char *const labels[AP_MENU_ROWS] = {
        "HOST", "PORT", "SLOT", "PASSWORD", "TLS", "POPUP", "PLACE", "DEATH LINK", "CONNECT", "PLAY"
    };
    char line[160];
    char shown[96];
    char status[128];
    char statusFit[48];
    int i;
    const char *value;

    dl = viSetFillColor(dl, 0, 0, 0);
    dl = viFillScreen(dl);
    if (walletinst[0] != NULL)
    {
        dl = frontSetupMenuBackground(dl);
    }
    dl = microcode_constructor(dl);

    {
        extern const char g_MidasVersion[];
        char title[40];

        snprintf(title, sizeof(title), "ARCHIPELAGO  %s", g_MidasVersion);
        dl = printLine(dl, 0x3C, AP_TITLE_Y, title);
    }
    apConnectionLine(status, (int) sizeof(status));
    fitStatus(statusFit, (int) sizeof(statusFit), status);
    dl = printLine(dl, 0x3C, AP_TITLE_Y + AP_ROW_PITCH, statusFit);

    for (i = 0; i < AP_MENU_ROWS; i++)
    {
        value = "";
        if (i == AP_ROW_HOST) value = sHost;
        else if (i == AP_ROW_PORT) value = sPort;
        else if (i == AP_ROW_SLOT) value = sSlot;
        else if (i == AP_ROW_PASSWORD)
        {
            int n = (int) strlen(sPassword);
            int k;
            if (n > (int) sizeof(shown) - 1) n = (int) sizeof(shown) - 1;
            for (k = 0; k < n; k++) shown[k] = '*';
            shown[n] = '\0';
            value = shown;
        }
        else if (i == AP_ROW_TLS) value = sTls ? "ON" : "OFF";
        else if (i == AP_ROW_POPUP) value = apGetPopup() ? "ON" : "OFF";
        else if (i == AP_ROW_PLACE) value = apPopupPosLabel();
        else if (i == AP_ROW_DEATH) value = apGetDeathLink() ? "ON" : "OFF";
        else if (i == AP_ROW_CONNECT) value = fieldsReady() ? "READY" : "NEED HOST, PORT, SLOT";
        else value = apSeedReady() ? "ENTER" : "LOCKED";

        snprintf(line, sizeof(line), "%s %-10s %s%s",
                 i == sRow ? ">" : " ",
                 labels[i],
                 value,
                 (i == sRow && i < AP_ROW_TLS && (sBlink & 16)) ? "_" : "");
        if (i == sRow)
        {
            s32 y = AP_ROW_Y0 + i * AP_ROW_PITCH;
            /* The bar is one text line, not a second line on top of the next row. */
            dl = microcode_constructor_related_to_menus(dl, 0x34, y - 1, 0x150, y + AP_ROW_PITCH - 3, 0x32);
        }
        dl = printLine(dl, 0x38, AP_ROW_Y0 + i * AP_ROW_PITCH, line);
    }
    dl = printLine(dl, 0x3C, AP_HINT_Y, "Arrows move. Left and right change a switch.");
    dl = frontDrawCursor(dl);
    return dl;
}
