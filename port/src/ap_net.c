#include "ap_net.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AP_IN_SLOTS 8
/* One game's datapackage is one message. Two megabytes covers a large game
 * without holding sixteen full copies. */
#define AP_IN_CAP (2 * 1024 * 1024)
#define AP_OUT_SLOTS 8
#define AP_OUT_CAP 2048
#define AP_ST_SLOTS 8
#define AP_ST_CAP 180

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static CRITICAL_SECTION sMu;
#else
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
static pthread_mutex_t sMu = PTHREAD_MUTEX_INITIALIZER;
#endif

static int sMuReady;
static volatile int sStop;
static volatile int sOpen;
static int sJoinable;

static char *sInMem;
static char *sInPtr[AP_IN_SLOTS];
static int sInLen[AP_IN_SLOTS];
static int sInHead;
static int sInCount;
static volatile int sItemLoss;

#ifndef _WIN32
static char sOut[AP_OUT_SLOTS][AP_OUT_CAP];
static int sOutLen[AP_OUT_SLOTS];
static int sOutHead;
static int sOutCount;
#endif

static char sSt[AP_ST_SLOTS][AP_ST_CAP];
static int sStHead;
static int sStCount;

#ifdef _WIN32
static HANDLE sThread;
#else
static pthread_t sThread;
static volatile int sThreadDone;
#endif

static void muInit(void)
{
    if (sMuReady)
    {
        return;
    }
#ifdef _WIN32
    InitializeCriticalSection(&sMu);
#endif
    sMuReady = 1;
}

static void muLock(void)
{
    muInit();
#ifdef _WIN32
    EnterCriticalSection(&sMu);
#else
    pthread_mutex_lock(&sMu);
#endif
}

static void muUnlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&sMu);
#else
    pthread_mutex_unlock(&sMu);
#endif
}

static void statusPut(const char *msg)
{
    int slot;

    muLock();
    if (sStCount < AP_ST_SLOTS)
    {
        slot = (sStHead + sStCount) % AP_ST_SLOTS;
        snprintf(sSt[slot], AP_ST_CAP, "%s", msg);
        sStCount++;
    }
    muUnlock();
}

static void statusf(const char *fmt, ...)
{
    char buf[AP_ST_CAP];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    statusPut(buf);
}

static int containsText(const char *data, int len, const char *key)
{
    int k;
    int i;

    if (data == NULL || key == NULL)
    {
        return 0;
    }
    k = (int) strlen(key);
    if (k <= 0 || len < k)
    {
        return 0;
    }
    for (i = 0; i <= len - k; i++)
    {
        if (memcmp(data + i, key, (size_t) k) == 0)
        {
            return 1;
        }
    }
    return 0;
}

/* Prints can wait. A lost item list is what makes the local index fall behind. */
static int keepMessage(const char *data, int len)
{
    return containsText(data, len, "ReceivedItems")
        || containsText(data, len, "\"Bounced\"")
        || containsText(data, len, "\"Connected\"")
        || containsText(data, len, "RoomInfo")
        || containsText(data, len, "ConnectionRefused");
}

/* indexFromHead is the queued message to forget. Pointers swap, the bytes stay put. */
static void dropQueued(int indexFromHead)
{
    int i;

    if (indexFromHead <= 0)
    {
        sInHead = (sInHead + 1) % AP_IN_SLOTS;
        sInCount--;
        return;
    }
    for (i = indexFromHead; i < sInCount - 1; i++)
    {
        int dst = (sInHead + i) % AP_IN_SLOTS;
        int src = (sInHead + i + 1) % AP_IN_SLOTS;
        char *tmp = sInPtr[dst];
        int ln = sInLen[dst];

        sInPtr[dst] = sInPtr[src];
        sInLen[dst] = sInLen[src];
        sInPtr[src] = tmp;
        sInLen[src] = ln;
    }
    sInCount--;
}

static void deliver(const char *data, int len)
{
    int slot;
    int keep;
    int i;
    int droppedItems = 0;

    if (len < 0)
    {
        return;
    }
    if (len >= AP_IN_CAP)
    {
        if (containsText(data, len, "ReceivedItems"))
        {
            muLock();
            sItemLoss = 1;
            muUnlock();
        }
        statusPut("server message was too large and was dropped");
        return;
    }
    keep = keepMessage(data, len);
    muLock();
    if (sInMem != NULL && sInCount >= AP_IN_SLOTS)
    {
        int drop = -1;

        for (i = 0; i < sInCount; i++)
        {
            int s = (sInHead + i) % AP_IN_SLOTS;
            if (!keepMessage(sInPtr[s], sInLen[s]))
            {
                drop = i;
                break;
            }
        }
        if (drop < 0 && keep)
        {
            int s = sInHead;
            if (containsText(sInPtr[s], sInLen[s], "ReceivedItems"))
            {
                droppedItems = 1;
            }
            drop = 0;
        }
        if (drop >= 0)
        {
            dropQueued(drop);
        }
    }
    if (sInMem != NULL && sInCount < AP_IN_SLOTS)
    {
        slot = (sInHead + sInCount) % AP_IN_SLOTS;
        memcpy(sInPtr[slot], data, (size_t) len);
        sInPtr[slot][len] = '\0';
        sInLen[slot] = len;
        sInCount++;
    }
    else if (keep && containsText(data, len, "ReceivedItems"))
    {
        droppedItems = 1;
    }
    if (droppedItems)
    {
        sItemLoss = 1;
    }
    muUnlock();
}

#ifndef _WIN32
static int takeOut(char *buf, int cap)
{
    int n = 0;

    muLock();
    if (sOutCount > 0)
    {
        n = sOutLen[sOutHead];
        if (n > cap)
        {
            n = cap;
        }
        memcpy(buf, sOut[sOutHead], (size_t) n);
        sOutHead = (sOutHead + 1) % AP_OUT_SLOTS;
        sOutCount--;
    }
    muUnlock();
    return n;
}
#endif

static int hostOk(const char *host)
{
    int n = 0;

    if (host == NULL || host[0] == '\0')
    {
        return 0;
    }
    for (n = 0; host[n] != '\0'; n++)
    {
        char c = host[n];
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '.' || c == '-';
        if (!ok || n > 200)
        {
            return 0;
        }
    }
    return 1;
}

static void ensureBuffers(void)
{
    int i;

    if (sInMem != NULL)
    {
        return;
    }
    sInMem = (char *) malloc((size_t) AP_IN_SLOTS * AP_IN_CAP);
    if (sInMem == NULL)
    {
        return;
    }
    for (i = 0; i < AP_IN_SLOTS; i++)
    {
        sInPtr[i] = sInMem + (size_t) i * AP_IN_CAP;
    }
}

static void reap(void)
{
#ifdef _WIN32
    if (sThread != NULL && WaitForSingleObject(sThread, 0) == WAIT_OBJECT_0)
    {
        CloseHandle(sThread);
        sThread = NULL;
        sJoinable = 0;
        sOpen = 0;
    }
#else
    if (sJoinable && sThreadDone)
    {
        pthread_join(sThread, NULL);
        sJoinable = 0;
        sThreadDone = 0;
        sOpen = 0;
    }
#endif
}

#ifdef _WIN32

/* WinHTTP websocket types, loaded from winhttp.dll so the exe does not link it. */
typedef void *ApInet;
typedef ApInet (WINAPI *FnOpen)(const wchar_t *, unsigned long, const wchar_t *, const wchar_t *, unsigned long);
typedef ApInet (WINAPI *FnConnect)(ApInet, const wchar_t *, unsigned short, unsigned long);
typedef ApInet (WINAPI *FnOpenReq)(ApInet, const wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *, const wchar_t **, unsigned long);
typedef int (WINAPI *FnSetOpt)(ApInet, unsigned long, void *, unsigned long);
typedef int (WINAPI *FnSendReq)(ApInet, const wchar_t *, unsigned long, void *, unsigned long, unsigned long, unsigned long);
typedef int (WINAPI *FnRecvResp)(ApInet, void *);
typedef ApInet (WINAPI *FnUpgrade)(ApInet, unsigned long);
typedef unsigned long (WINAPI *FnWsSend)(ApInet, unsigned long, void *, unsigned long);
typedef unsigned long (WINAPI *FnWsRecv)(ApInet, void *, unsigned long, unsigned long *, unsigned long *);
typedef unsigned long (WINAPI *FnWsClose)(ApInet, unsigned short, void *, unsigned long);
typedef int (WINAPI *FnClose)(ApInet);
typedef int (WINAPI *FnTimeouts)(ApInet, int, int, int, int);

#define AP_WS_UTF8 2
#define AP_WS_UTF8_FRAG 3
#define AP_WS_CLOSE 4
#define AP_OPT_UPGRADE 114
#define AP_FLAG_SECURE 0x00800000u
#define AP_ERR_TIMEOUT 12002u

static FnOpen pOpen;
static FnConnect pConnect;
static FnOpenReq pOpenReq;
static FnSetOpt pSetOpt;
static FnSendReq pSendReq;
static FnRecvResp pRecvResp;
static FnUpgrade pUpgrade;
static FnWsSend pWsSend;
static FnWsRecv pWsRecv;
static FnWsClose pWsClose;
static FnClose pClose;
static FnTimeouts pTimeouts;
static ApInet sWs;
static int sWinhttp;

static int loadWinhttp(void)
{
    HMODULE mod;

    if (sWinhttp)
    {
        return 1;
    }
    mod = LoadLibraryA("winhttp.dll");
    if (mod == NULL)
    {
        return 0;
    }
    pOpen = (FnOpen) GetProcAddress(mod, "WinHttpOpen");
    pConnect = (FnConnect) GetProcAddress(mod, "WinHttpConnect");
    pOpenReq = (FnOpenReq) GetProcAddress(mod, "WinHttpOpenRequest");
    pSetOpt = (FnSetOpt) GetProcAddress(mod, "WinHttpSetOption");
    pSendReq = (FnSendReq) GetProcAddress(mod, "WinHttpSendRequest");
    pRecvResp = (FnRecvResp) GetProcAddress(mod, "WinHttpReceiveResponse");
    pUpgrade = (FnUpgrade) GetProcAddress(mod, "WinHttpWebSocketCompleteUpgrade");
    pWsSend = (FnWsSend) GetProcAddress(mod, "WinHttpWebSocketSend");
    pWsRecv = (FnWsRecv) GetProcAddress(mod, "WinHttpWebSocketReceive");
    pWsClose = (FnWsClose) GetProcAddress(mod, "WinHttpWebSocketClose");
    pClose = (FnClose) GetProcAddress(mod, "WinHttpCloseHandle");
    pTimeouts = (FnTimeouts) GetProcAddress(mod, "WinHttpSetTimeouts");
    if (pOpen == NULL || pConnect == NULL || pOpenReq == NULL || pSetOpt == NULL ||
        pSendReq == NULL || pRecvResp == NULL || pUpgrade == NULL || pWsSend == NULL ||
        pWsRecv == NULL || pWsClose == NULL || pClose == NULL)
    {
        return 0;
    }
    sWinhttp = 1;
    return 1;
}

static void toWide(const char *s, wchar_t *w, int n)
{
    int i;
    for (i = 0; i < n - 1 && s[i] != '\0'; i++)
    {
        w[i] = (unsigned char) s[i];
    }
    w[i] = 0;
}

static DWORD WINAPI winThread(LPVOID arg)
{
    const char *host = ((const char **) arg)[0];
    int port = *(const int *) ((const char **) arg)[1];
    int tls = *(const int *) ((const char **) arg)[2];
    wchar_t whost[256];
    ApInet session = NULL;
    ApInet connect = NULL;
    ApInet request = NULL;
    ApInet ws = NULL;
    char *acc = NULL;
    int used = 0;
    int overflow = 0;
    unsigned long flags;

    (void) arg;
    acc = (char *) malloc(AP_IN_CAP);
    if (acc == NULL)
    {
        statusPut("out of memory for the archipelago socket");
        goto done;
    }
    toWide(host, whost, 256);
    session = pOpen(L"ProjectMidas", 0, NULL, NULL, 0);
    if (session == NULL)
    {
        statusf("WinHTTP open failed (%lu)", (unsigned long) GetLastError());
        goto done;
    }
    if (pTimeouts != NULL)
    {
        /* Receive wakes about once a second so disconnect does not sit in a blocking read. */
        pTimeouts(session, 8000, 8000, 8000, 1000);
    }
    connect = pConnect(session, whost, (unsigned short) port, 0);
    if (connect == NULL)
    {
        statusf("could not resolve %s (%lu)", host, (unsigned long) GetLastError());
        goto done;
    }
    flags = tls ? AP_FLAG_SECURE : 0;
    request = pOpenReq(connect, L"GET", L"/", NULL, NULL, NULL, flags);
    if (request == NULL)
    {
        statusf("websocket request failed (%lu)", (unsigned long) GetLastError());
        goto done;
    }
    if (!pSetOpt(request, AP_OPT_UPGRADE, NULL, 0))
    {
        statusf("websocket upgrade was refused by WinHTTP (%lu)", (unsigned long) GetLastError());
        goto done;
    }
    if (!pSendReq(request, NULL, 0, NULL, 0, 0, 0) || !pRecvResp(request, NULL))
    {
        statusf("could not connect to %s:%d (%lu)", host, port, (unsigned long) GetLastError());
        goto done;
    }
    ws = pUpgrade(request, 0);
    pClose(request);
    request = NULL;
    if (ws == NULL)
    {
        statusf("websocket upgrade to %s:%d failed (%lu)", host, port, (unsigned long) GetLastError());
        goto done;
    }
    muLock();
    sWs = ws;
    sOpen = 1;
    muUnlock();
    statusf("websocket open to %s:%d", host, port);

    while (!sStop)
    {
        unsigned long n = 0;
        unsigned long kind = 0;
        unsigned long err;
        int space;

        space = AP_IN_CAP - used - 1;
        if (space < 1)
        {
            overflow = 1;
            used = 0;
            space = AP_IN_CAP - 1;
        }
        err = pWsRecv(ws, acc + used, (unsigned long) space, &n, &kind);
        if (err == AP_ERR_TIMEOUT)
        {
            continue;
        }
        if (err != 0)
        {
            if (!sStop)
            {
                statusf("connection closed (%lu)", err);
            }
            break;
        }
        if (kind == AP_WS_CLOSE)
        {
            statusPut("server closed the connection");
            break;
        }
        used += (int) n;
        if (kind == AP_WS_UTF8_FRAG)
        {
            continue;
        }
        if (kind == AP_WS_UTF8)
        {
            if (!overflow && used > 0)
            {
                deliver(acc, used);
            }
            else if (overflow)
            {
                statusPut("server message was too large and was dropped");
            }
            used = 0;
            overflow = 0;
        }
        else
        {
            used = 0;
            overflow = 0;
        }
    }

done:
    muLock();
    sOpen = 0;
    sWs = NULL;
    muUnlock();
    if (ws != NULL)
    {
        pWsClose(ws, 1000, NULL, 0);
        pClose(ws);
    }
    if (request != NULL)
    {
        pClose(request);
    }
    if (connect != NULL)
    {
        pClose(connect);
    }
    if (session != NULL)
    {
        pClose(session);
    }
    free(acc);
    return 0;
}

/* The thread reads these until it has copied them. They stay valid: the
 * game thread does not start another connection until this one is reaped,
 * and reap happens only after the thread has returned. */
static char sHostCopy[256];
static int sPortCopy;
static int sTlsCopy;
static const void *sThreadArgs[3];

static int startWindows(const char *host, int port, int tls)
{
    if (!loadWinhttp())
    {
        statusPut("winhttp.dll is missing, so the archipelago socket cannot open");
        return -1;
    }
    snprintf(sHostCopy, sizeof(sHostCopy), "%s", host);
    sPortCopy = port;
    sTlsCopy = tls;
    sThreadArgs[0] = sHostCopy;
    sThreadArgs[1] = &sPortCopy;
    sThreadArgs[2] = &sTlsCopy;
    sStop = 0;
    sOpen = 0;
    sJoinable = 1;
    sThread = CreateThread(NULL, 0, winThread, (void *) sThreadArgs, 0, NULL);
    if (sThread == NULL)
    {
        sJoinable = 0;
        statusPut("could not start the archipelago thread");
        return 0;
    }
    return 1;
}

static int sendWindows(const char *text, int len)
{
    ApInet ws;
    unsigned long err = 0;

    muLock();
    ws = sWs;
    if (ws != NULL && pWsSend != NULL)
    {
        err = pWsSend(ws, AP_WS_UTF8, (void *) text, (unsigned long) len);
    }
    else
    {
        ws = NULL;
    }
    muUnlock();
    if (ws == NULL)
    {
        return 0;
    }
    if (err != 0)
    {
        statusf("send failed (%lu)", err);
        sStop = 1;
        return 0;
    }
    return 1;
}

#else

static int sFd = -1;

static int setNonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int waitFd(int fd, int wantWrite, int ms)
{
    fd_set fds;
    struct timeval tv;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    if (wantWrite)
    {
        return select(fd + 1, NULL, &fds, NULL, &tv);
    }
    return select(fd + 1, &fds, NULL, NULL, &tv);
}

static int writeAll(int fd, const unsigned char *p, int n)
{
    int off = 0;
    while (off < n)
    {
        int w;
        if (sStop)
        {
            return -1;
        }
        w = (int) send(fd, (const char *) p + off, (size_t) (n - off), MSG_NOSIGNAL);
        if (w < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (waitFd(fd, 1, 200) < 0)
                {
                    return -1;
                }
                continue;
            }
            return -1;
        }
        if (w == 0)
        {
            return -1;
        }
        off += w;
    }
    return 0;
}

static int sendFrame(int fd, unsigned char opcode, const unsigned char *data, int len)
{
    unsigned char hdr[8];
    unsigned char mask[4];
    unsigned char stack[AP_OUT_CAP];
    unsigned char *payload;
    int hlen;
    int i;
    int own = 0;

    mask[0] = (unsigned char) (rand() & 0xFF);
    mask[1] = (unsigned char) (rand() & 0xFF);
    mask[2] = (unsigned char) (rand() & 0xFF);
    mask[3] = (unsigned char) (rand() & 0xFF);
    hdr[0] = (unsigned char) (0x80 | opcode);
    if (len < 126)
    {
        hdr[1] = (unsigned char) (0x80 | len);
        hlen = 2;
    }
    else if (len < 65536)
    {
        hdr[1] = (unsigned char) (0x80 | 126);
        hdr[2] = (unsigned char) ((len >> 8) & 0xFF);
        hdr[3] = (unsigned char) (len & 0xFF);
        hlen = 4;
    }
    else
    {
        return -1;
    }
    memcpy(hdr + hlen, mask, 4);
    hlen += 4;
    if (writeAll(fd, hdr, hlen) != 0)
    {
        return -1;
    }
    if (len <= 0)
    {
        return 0;
    }
    if (len <= AP_OUT_CAP)
    {
        payload = stack;
    }
    else
    {
        payload = (unsigned char *) malloc((size_t) len);
        if (payload == NULL)
        {
            return -1;
        }
        own = 1;
    }
    for (i = 0; i < len; i++)
    {
        payload[i] = data[i] ^ mask[i & 3];
    }
    i = writeAll(fd, payload, len);
    if (own)
    {
        free(payload);
    }
    return i;
}

static int handshake(int fd, const char *host, int port)
{
    char req[512];
    char resp[1024];
    int n = 0;
    int reqlen;

    reqlen = snprintf(req, sizeof(req),
                      "GET / HTTP/1.1\r\n"
                      "Host: %s:%d\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                      "Sec-WebSocket-Version: 13\r\n"
                      "\r\n",
                      host, port);
    if (reqlen <= 0 || reqlen >= (int) sizeof(req))
    {
        return -1;
    }
    if (writeAll(fd, (const unsigned char *) req, reqlen) != 0)
    {
        return -1;
    }
    while (n < (int) sizeof(resp) - 1)
    {
        int r;
        if (sStop)
        {
            return -1;
        }
        r = (int) recv(fd, resp + n, (size_t) ((int) sizeof(resp) - 1 - n), 0);
        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (waitFd(fd, 0, 200) <= 0 && sStop)
                {
                    return -1;
                }
                continue;
            }
            return -1;
        }
        if (r == 0)
        {
            return -1;
        }
        n += r;
        resp[n] = '\0';
        if (strstr(resp, "\r\n\r\n") != NULL)
        {
            break;
        }
    }
    if (strstr(resp, " 101 ") == NULL)
    {
        statusPut("server did not accept the websocket upgrade");
        return -1;
    }
    return 0;
}

static void *posixThread(void *arg)
{
    const char *host = ((const char **) arg)[0];
    int port = *(const int *) ((const char **) arg)[1];
    char portstr[16];
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *ai;
    int fd = -1;
    unsigned char *acc = NULL;
    int accLen = 0;
    unsigned char *msg = NULL;
    int msgLen = 0;
    int msgCap;
    int msgKind = -1;

    acc = (unsigned char *) malloc(AP_IN_CAP);
    msgCap = AP_IN_CAP;
    msg = (unsigned char *) malloc((size_t) msgCap);
    if (acc == NULL || msg == NULL)
    {
        statusPut("out of memory for the archipelago socket");
        goto done;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if (getaddrinfo(host, portstr, &hints, &res) != 0)
    {
        statusf("could not resolve %s", host);
        goto done;
    }
    for (ai = res; ai != NULL; ai = ai->ai_next)
    {
        int rc;
        fd = (int) socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0)
        {
            continue;
        }
        if (setNonblock(fd) != 0)
        {
            close(fd);
            fd = -1;
            continue;
        }
        rc = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (rc != 0 && errno != EINPROGRESS)
        {
            close(fd);
            fd = -1;
            continue;
        }
        while (!sStop)
        {
            int sel = waitFd(fd, 1, 200);
            if (sel > 0)
            {
                int err = 0;
                socklen_t elen = sizeof(err);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen);
                if (err != 0)
                {
                    close(fd);
                    fd = -1;
                }
                break;
            }
            if (sel < 0 && errno != EINTR)
            {
                close(fd);
                fd = -1;
                break;
            }
        }
        if (fd >= 0)
        {
            break;
        }
    }
    freeaddrinfo(res);
    if (fd < 0 || sStop)
    {
        if (!sStop)
        {
            statusf("could not connect to %s:%d", host, port);
        }
        goto done;
    }
    if (handshake(fd, host, port) != 0)
    {
        if (!sStop)
        {
            statusf("websocket handshake with %s:%d failed", host, port);
        }
        goto done;
    }
    sFd = fd;
    sOpen = 1;
    statusf("websocket open to %s:%d", host, port);

    while (!sStop)
    {
        char outgoing[AP_OUT_CAP];
        int outn;
        int r;
        int sel;

        outn = takeOut(outgoing, AP_OUT_CAP);
        if (outn > 0 && sendFrame(fd, 0x1, (unsigned char *) outgoing, outn) != 0)
        {
            statusPut("send failed");
            break;
        }
        sel = waitFd(fd, 0, 200);
        if (sel < 0 && errno != EINTR)
        {
            break;
        }
        if (sel <= 0)
        {
            continue;
        }
        if (accLen >= AP_IN_CAP)
        {
            statusPut("server message was too large and was dropped");
            break;
        }
        r = (int) recv(fd, (char *) acc + accLen, (size_t) (AP_IN_CAP - accLen), 0);
        if (r == 0)
        {
            statusPut("server closed the connection");
            break;
        }
        if (r < 0)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            statusPut("connection closed");
            break;
        }
        accLen += r;
        while (accLen >= 2)
        {
            int fin = (acc[0] & 0x80) != 0;
            int opcode = acc[0] & 0x0F;
            int masked = (acc[1] & 0x80) != 0;
            unsigned long long plen = (unsigned long long) (acc[1] & 0x7F);
            int hlen = 2;
            unsigned char mask[4];
            unsigned long long i;
            if (plen == 126)
            {
                hlen = 4;
            }
            else if (plen == 127)
            {
                hlen = 10;
            }
            if (accLen < hlen + (masked ? 4 : 0))
            {
                break;
            }
            if (plen == 126)
            {
                plen = ((unsigned long long) acc[2] << 8) | acc[3];
            }
            else if (plen == 127)
            {
                plen = 0;
                for (i = 0; i < 8; i++)
                {
                    plen = (plen << 8) | acc[2 + i];
                }
            }
            if (masked)
            {
                memcpy(mask, acc + hlen, 4);
                hlen += 4;
            }
            if ((unsigned long long) accLen < (unsigned long long) hlen + plen)
            {
                break;
            }
            if (plen > (unsigned long long) AP_IN_CAP)
            {
                statusPut("server message was too large and was dropped");
                goto done;
            }
            if (masked)
            {
                for (i = 0; i < plen; i++)
                {
                    acc[hlen + i] ^= mask[i & 3];
                }
            }
            if (opcode == 0x8)
            {
                statusPut("server closed the connection");
                goto done;
            }
            if (opcode == 0x9)
            {
                sendFrame(fd, 0xA, acc + hlen, (int) plen);
            }
            else if (opcode == 0x1 || opcode == 0x2 || opcode == 0x0)
            {
                if (opcode == 0x1 || opcode == 0x2)
                {
                    msgLen = 0;
                    msgKind = opcode;
                }
                if (msgLen + (int) plen < msgCap && msgKind == 0x1)
                {
                    memcpy(msg + msgLen, acc + hlen, (size_t) plen);
                    msgLen += (int) plen;
                    if (fin)
                    {
                        deliver((char *) msg, msgLen);
                        msgLen = 0;
                    }
                }
                else if (fin)
                {
                    msgLen = 0;
                }
            }
            {
                int rest = accLen - (hlen + (int) plen);
                memmove(acc, acc + hlen + (int) plen, (size_t) rest);
                accLen = rest;
            }
        }
    }

done:
    sOpen = 0;
    sFd = -1;
    if (fd >= 0)
    {
        close(fd);
    }
    free(acc);
    free(msg);
    sThreadDone = 1;
    return NULL;
}

static char sHostCopy[256];
static int sPortCopy;
static int sTlsCopy;
static const void *sThreadArgs[3];

static int startPosix(const char *host, int port, int tls)
{
    if (tls)
    {
        statusPut("this build speaks ws:// only; set tls=0 and point it at a local Archipelago server");
        return -1;
    }
    snprintf(sHostCopy, sizeof(sHostCopy), "%s", host);
    sPortCopy = port;
    sTlsCopy = tls;
    sThreadArgs[0] = sHostCopy;
    sThreadArgs[1] = &sPortCopy;
    sThreadArgs[2] = &sTlsCopy;
    sStop = 0;
    sOpen = 0;
    sThreadDone = 0;
    sJoinable = 1;
    if (pthread_create(&sThread, NULL, posixThread, (void *) sThreadArgs) != 0)
    {
        sJoinable = 0;
        statusPut("could not start the archipelago thread");
        return 0;
    }
    return 1;
}

static int sendPosix(const char *text, int len)
{
    int slot;

    if (len < 0 || len >= AP_OUT_CAP || !sOpen)
    {
        return 0;
    }
    muLock();
    if (sOutCount >= AP_OUT_SLOTS)
    {
        muUnlock();
        return 0;
    }
    slot = (sOutHead + sOutCount) % AP_OUT_SLOTS;
    memcpy(sOut[slot], text, (size_t) len);
    sOutLen[slot] = len;
    sOutCount++;
    muUnlock();
    return 1;
}

#endif

int apNetStart(const char *host, int port, int tls)
{
    /* Before the worker exists, so its first status line cannot race the init. */
    muInit();
    /* The previous worker has to be joined before its host buffer can be reused. */
    if (apNetBusy())
    {
        return 0;
    }
    if (!hostOk(host) || port < 1 || port > 65535)
    {
        statusPut("archipelago host or port is not usable");
        return -1;
    }
    ensureBuffers();
    if (sInMem == NULL)
    {
        statusPut("out of memory for the archipelago socket");
        return 0;
    }
    muLock();
    sInHead = sInCount = 0;
    sItemLoss = 0;
    muUnlock();
#ifndef _WIN32
    sOutHead = sOutCount = 0;
#endif
#ifdef _WIN32
    return startWindows(host, port, tls);
#else
    return startPosix(host, port, tls);
#endif
}

void apNetStop(void)
{
    sStop = 1;
    sOpen = 0;
}

int apNetBusy(void)
{
    reap();
#ifdef _WIN32
    return sThread != NULL;
#else
    return sJoinable && !sThreadDone;
#endif
}

int apNetIsOpen(void)
{
    return sOpen;
}

int apNetSend(const char *text, int len)
{
    if (text == NULL || len <= 0)
    {
        return 0;
    }
#ifdef _WIN32
    return sendWindows(text, len);
#else
    return sendPosix(text, len);
#endif
}

int apNetPoll(char *buf, int buflen)
{
    int n;

    reap();
    if (buf == NULL || buflen < 2)
    {
        return 0;
    }
    muLock();
    if (sInCount <= 0)
    {
        muUnlock();
        return 0;
    }
    n = sInLen[sInHead];
    if (n >= buflen)
    {
        if (containsText(sInPtr[sInHead], n, "ReceivedItems"))
        {
            sItemLoss = 1;
        }
        sInHead = (sInHead + 1) % AP_IN_SLOTS;
        sInCount--;
        muUnlock();
        return -1;
    }
    memcpy(buf, sInPtr[sInHead], (size_t) n);
    buf[n] = '\0';
    sInHead = (sInHead + 1) % AP_IN_SLOTS;
    sInCount--;
    muUnlock();
    return n;
}

int apNetTakeLoss(void)
{
    int lost;

    muLock();
    lost = sItemLoss;
    sItemLoss = 0;
    muUnlock();
    return lost;
}

int apNetTakeStatus(char *buf, int buflen)
{
    if (buf == NULL || buflen < 2)
    {
        return 0;
    }
    muLock();
    if (sStCount <= 0)
    {
        muUnlock();
        buf[0] = '\0';
        return 0;
    }
    snprintf(buf, (size_t) buflen, "%s", sSt[sStHead]);
    sStHead = (sStHead + 1) % AP_ST_SLOTS;
    sStCount--;
    muUnlock();
    return 1;
}
