/**
 * Discord rich presence over the local IPC pipe.
 *
 * Discord listens on \\.\pipe\discord-ipc-N. The handshake and SET_ACTIVITY
 * frames are the public IPC protocol (opcode, little-endian length, JSON).
 * The pipe is non-blocking so a missing Discord client cannot stall the game.
 * The application id is compiled in. It is public and only names the presence.
 */
#include "ap_discord.h"
#include "ap_client.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32

void apDiscordTick(void) {}

#else

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DISCORD_OP_HANDSHAKE 0
#define DISCORD_OP_FRAME 1
#define DISCORD_OP_PING 3
#define DISCORD_OP_PONG 4

static HANDLE sPipe = INVALID_HANDLE_VALUE;
static int sReady;
static int sWaitUntil;
static unsigned int sNonce;
static char sLast[160];
static time_t sStarted;

static void pipeClose(void)
{
    if (sPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(sPipe);
        sPipe = INVALID_HANDLE_VALUE;
    }
    sReady = 0;
}

static int pipeWrite(const void *buf, int n)
{
    DWORD wrote = 0;

    if (sPipe == INVALID_HANDLE_VALUE || n <= 0)
    {
        return 0;
    }
    if (!WriteFile(sPipe, buf, (DWORD) n, &wrote, NULL) || (int) wrote != n)
    {
        pipeClose();
        return 0;
    }
    return 1;
}

static int sendJson(int op, const char *json)
{
    unsigned char frame[8 + 512];
    unsigned int len;

    len = (unsigned int) strlen(json);
    if (len > 512)
    {
        return 0;
    }
    frame[0] = (unsigned char) op;
    frame[1] = 0;
    frame[2] = 0;
    frame[3] = 0;
    frame[4] = (unsigned char) len;
    frame[5] = (unsigned char) (len >> 8);
    frame[6] = (unsigned char) (len >> 16);
    frame[7] = (unsigned char) (len >> 24);
    memcpy(frame + 8, json, len);
    return pipeWrite(frame, (int) (8 + len));
}

static void jsonEscape(const char *src, char *dst, int n)
{
    int i = 0;

    if (n < 2)
    {
        return;
    }
    for (; src != NULL && *src != '\0' && i < n - 2; src++)
    {
        unsigned char c = (unsigned char) *src;
        if (c == '"' || c == '\\')
        {
            if (i >= n - 3)
            {
                break;
            }
            dst[i++] = '\\';
        }
        if (c < 0x20 || c >= 0x7F)
        {
            dst[i++] = ' ';
            continue;
        }
        dst[i++] = (char) c;
    }
    dst[i] = '\0';
}

static int pipeOpen(void)
{
    int i;
    DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;

    for (i = 0; i < 10; i++)
    {
        char name[64];

        snprintf(name, sizeof(name), "\\\\.\\pipe\\discord-ipc-%d", i);
        sPipe = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (sPipe != INVALID_HANDLE_VALUE)
        {
            SetNamedPipeHandleState(sPipe, &mode, NULL, NULL);
            return 1;
        }
    }
    sPipe = INVALID_HANDLE_VALUE;
    return 0;
}

/* Drain whatever Discord already queued. A ping has to be ponged or the
 * client drops the pipe. */
static void pump(void)
{
    unsigned char buf[1024];
    DWORD got = 0;

    for (;;)
    {
        got = 0;
        if (!ReadFile(sPipe, buf, sizeof(buf), &got, NULL))
        {
            if (GetLastError() == ERROR_NO_DATA || GetLastError() == ERROR_PIPE_LISTENING)
            {
                return;
            }
            pipeClose();
            return;
        }
        if (got < 8)
        {
            return;
        }
        if (buf[0] == DISCORD_OP_PING)
        {
            unsigned int len = (unsigned int) buf[4] | ((unsigned int) buf[5] << 8)
                | ((unsigned int) buf[6] << 16) | ((unsigned int) buf[7] << 24);
            char payload[256];

            if (len >= sizeof(payload))
            {
                len = (unsigned int) sizeof(payload) - 1;
            }
            if (8 + len > got)
            {
                len = got - 8;
            }
            memcpy(payload, buf + 8, len);
            payload[len] = '\0';
            sendJson(DISCORD_OP_PONG, payload);
        }
        else if (buf[0] == DISCORD_OP_FRAME)
        {
            sReady = 1;
        }
        else if (buf[0] == 2)
        {
            pipeClose();
            return;
        }
    }
}

static void publish(const char *app, const char *details, const char *state)
{
    char d[96];
    char st[96];
    char json[480];
    char key[160];

    jsonEscape(details, d, (int) sizeof(d));
    jsonEscape(state, st, (int) sizeof(st));
    snprintf(key, sizeof(key), "%s|%s", d, st);
    if (strcmp(key, sLast) == 0)
    {
        return;
    }
    snprintf(json, sizeof(json),
             "{\"cmd\":\"SET_ACTIVITY\",\"nonce\":\"%u\",\"args\":{\"pid\":%lu,\"activity\":{"
             "\"details\":\"%s\",\"state\":\"%s\",\"timestamps\":{\"start\":%ld}}}}",
             ++sNonce, (unsigned long) GetCurrentProcessId(), d, st, (long) sStarted);
    if (sendJson(DISCORD_OP_FRAME, json))
    {
        snprintf(sLast, sizeof(sLast), "%s", key);
    }
}

void apDiscordTick(void)
{
    const char *app;
    char details[96];
    char state[96];
    char hello[96];
    int now;

    app = apDiscordApp();
    if (app == NULL || app[0] == '\0')
    {
        return;
    }
    now = (int) time(NULL);
    if (sPipe == INVALID_HANDLE_VALUE)
    {
        if (now < sWaitUntil)
        {
            return;
        }
        sWaitUntil = now + 15;
        if (!pipeOpen())
        {
            return;
        }
        sReady = 0;
        sLast[0] = '\0';
        if (sStarted == 0)
        {
            sStarted = time(NULL);
        }
        snprintf(hello, sizeof(hello), "{\"v\":1,\"client_id\":\"%s\"}", app);
        if (!sendJson(DISCORD_OP_HANDSHAKE, hello))
        {
            return;
        }
    }
    pump();
    if (!sReady)
    {
        return;
    }
    /* publish() itself skips a repeat, so a mission change is the only write. */
    apPresenceFill(details, (int) sizeof(details), state, (int) sizeof(state));
    publish(app, details, state);
}

#endif
