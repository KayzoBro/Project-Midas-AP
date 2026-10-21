#include "ap_json.h"

#include <string.h>

static void skipWs(ApJson *j)
{
    while (j->p < j->end)
    {
        char c = *j->p;
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
        {
            return;
        }
        j->p++;
    }
}

void apJsonInit(ApJson *j, const char *text, int len)
{
    j->p = text;
    j->end = text + (len < 0 ? 0 : len);
}

int apJsonEat(ApJson *j, char c)
{
    skipWs(j);
    if (j->p < j->end && *j->p == c)
    {
        j->p++;
        return 1;
    }
    return 0;
}

static int hexNibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void pushByte(char *buf, int buflen, int *n, unsigned char b)
{
    if (*n < buflen - 1)
    {
        buf[*n] = (char) b;
    }
    (*n)++;
}

/* UTF-8 for a BMP code point. The HUD only wants ASCII; the console can show the rest. */
static void pushUtf8(char *buf, int buflen, int *n, unsigned int cp)
{
    if (cp < 0x80)
    {
        pushByte(buf, buflen, n, (unsigned char) cp);
    }
    else if (cp < 0x800)
    {
        pushByte(buf, buflen, n, (unsigned char) (0xC0 | (cp >> 6)));
        pushByte(buf, buflen, n, (unsigned char) (0x80 | (cp & 0x3F)));
    }
    else
    {
        pushByte(buf, buflen, n, (unsigned char) (0xE0 | (cp >> 12)));
        pushByte(buf, buflen, n, (unsigned char) (0x80 | ((cp >> 6) & 0x3F)));
        pushByte(buf, buflen, n, (unsigned char) (0x80 | (cp & 0x3F)));
    }
}

int apJsonString(ApJson *j, char *buf, int buflen)
{
    int n = 0;

    skipWs(j);
    if (buflen < 1 || j->p >= j->end || *j->p != '"')
    {
        return 0;
    }
    j->p++;
    while (j->p < j->end)
    {
        unsigned char c = (unsigned char) *j->p++;
        if (c == '"')
        {
            if (n >= buflen)
            {
                n = buflen - 1;
            }
            buf[n] = '\0';
            return 1;
        }
        if (c == '\\')
        {
            if (j->p >= j->end)
            {
                break;
            }
            c = (unsigned char) *j->p++;
            if (c == 'u')
            {
                int i;
                unsigned int cp = 0;
                if (j->p + 4 > j->end)
                {
                    break;
                }
                for (i = 0; i < 4; i++)
                {
                    int h = hexNibble(*j->p++);
                    if (h < 0)
                    {
                        buf[0] = '\0';
                        return 0;
                    }
                    cp = (cp << 4) | (unsigned int) h;
                }
                pushUtf8(buf, buflen, &n, cp);
            }
            else if (c == 'n') pushByte(buf, buflen, &n, '\n');
            else if (c == 'r') pushByte(buf, buflen, &n, '\r');
            else if (c == 't') pushByte(buf, buflen, &n, '\t');
            else if (c == 'b') pushByte(buf, buflen, &n, '\b');
            else if (c == 'f') pushByte(buf, buflen, &n, '\f');
            else pushByte(buf, buflen, &n, c);
        }
        else
        {
            pushByte(buf, buflen, &n, c);
        }
    }
    if (buflen > 0)
    {
        buf[0] = '\0';
    }
    return 0;
}

int apJsonInt(ApJson *j, int *out)
{
    int sign = 1;
    long v = 0;
    int any = 0;

    skipWs(j);
    if (j->p >= j->end)
    {
        return 0;
    }
    if (*j->p == '-')
    {
        sign = -1;
        j->p++;
    }
    while (j->p < j->end && *j->p >= '0' && *j->p <= '9')
    {
        any = 1;
        v = v * 10 + (*j->p - '0');
        if (v > 2147483647L)
        {
            v = 2147483647L;
        }
        j->p++;
    }
    if (!any)
    {
        return 0;
    }
    /* Archipelago ids are integers. A float still has to be consumed so the
     * cursor stays aligned, but only the whole part is kept. */
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
    *out = (int) (sign < 0 ? -v : v);
    return 1;
}

static int skipString(ApJson *j)
{
    char sink[8];
    /* apJsonString already walks the whole string; the sink only truncates. */
    return apJsonString(j, sink, (int) sizeof(sink));
}

static int skipValue(ApJson *j, int depth)
{
    char c;

    if (depth > 48)
    {
        return 0;
    }
    skipWs(j);
    if (j->p >= j->end)
    {
        return 0;
    }
    c = *j->p;
    if (c == '"')
    {
        return skipString(j);
    }
    if (c == '{')
    {
        int started = 0;
        char key[8];
        for (;;)
        {
            int step = apJsonObjectNext(j, key, (int) sizeof(key), &started);
            if (step == 0)
            {
                return 1;
            }
            if (step < 0 || !skipValue(j, depth + 1))
            {
                return 0;
            }
        }
    }
    if (c == '[')
    {
        int started = 0;
        for (;;)
        {
            int step = apJsonArrayNext(j, &started);
            if (step == 0)
            {
                return 1;
            }
            if (step < 0 || !skipValue(j, depth + 1))
            {
                return 0;
            }
        }
    }
    if (c == 't')
    {
        if (j->end - j->p >= 4 && memcmp(j->p, "true", 4) == 0)
        {
            j->p += 4;
            return 1;
        }
        return 0;
    }
    if (c == 'f')
    {
        if (j->end - j->p >= 5 && memcmp(j->p, "false", 5) == 0)
        {
            j->p += 5;
            return 1;
        }
        return 0;
    }
    if (c == 'n')
    {
        if (j->end - j->p >= 4 && memcmp(j->p, "null", 4) == 0)
        {
            j->p += 4;
            return 1;
        }
        return 0;
    }
    if (c == '-' || (c >= '0' && c <= '9'))
    {
        int dummy;
        return apJsonInt(j, &dummy);
    }
    return 0;
}

int apJsonSkip(ApJson *j)
{
    return skipValue(j, 0);
}

/* After a comma, a closing bracket is a trailing comma, not another value. */
static int endAfterComma(ApJson *j, char close)
{
    const char *s = j->p + 1;
    while (s < j->end && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'))
    {
        s++;
    }
    if (s < j->end && *s == close)
    {
        j->p = s + 1;
        return 1;
    }
    j->p++;
    return 0;
}

int apJsonObjectNext(ApJson *j, char *key, int keylen, int *started)
{
    skipWs(j);
    if (!*started)
    {
        if (j->p >= j->end || *j->p != '{')
        {
            return -1;
        }
        j->p++;
        *started = 1;
    }
    else
    {
        skipWs(j);
        if (j->p < j->end && *j->p == ',')
        {
            if (endAfterComma(j, '}'))
            {
                return 0;
            }
        }
        else if (j->p >= j->end || *j->p != '}')
        {
            return -1;
        }
    }
    skipWs(j);
    if (j->p < j->end && *j->p == '}')
    {
        j->p++;
        return 0;
    }
    if (!apJsonString(j, key, keylen))
    {
        return -1;
    }
    if (!apJsonEat(j, ':'))
    {
        return -1;
    }
    return 1;
}

int apJsonArrayNext(ApJson *j, int *started)
{
    skipWs(j);
    if (!*started)
    {
        if (j->p >= j->end || *j->p != '[')
        {
            return -1;
        }
        j->p++;
        *started = 1;
    }
    else
    {
        skipWs(j);
        if (j->p < j->end && *j->p == ',')
        {
            if (endAfterComma(j, ']'))
            {
                return 0;
            }
        }
        else if (j->p >= j->end || *j->p != ']')
        {
            return -1;
        }
    }
    skipWs(j);
    if (j->p < j->end && *j->p == ']')
    {
        j->p++;
        return 0;
    }
    if (j->p >= j->end)
    {
        return -1;
    }
    return 1;
}
