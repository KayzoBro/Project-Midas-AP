/* Standalone checks for the Archipelago JSON reader and the shared ids. */
#include "ap_json.h"
#include "ap_ids.h"

#include <stdio.h>
#include <string.h>

static int gFails;

static void expect(int cond, const char *msg)
{
    if (!cond)
    {
        fprintf(stderr, "FAIL %s\n", msg);
        gFails++;
    }
}

static int findKey(const char *start, const char *end, const char *want, ApJson *out)
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

static void testStrings(void)
{
    ApJson j;
    char buf[32];
    int n;
    const char *s = "\"A\\nB\\\"C\\u0041\"";

    apJsonInit(&j, s, (int) strlen(s));
    expect(apJsonString(&j, buf, (int) sizeof(buf)) == 1, "string decodes");
    expect(strcmp(buf, "A\nB\"CA") == 0, "escapes and unicode");

    s = "  -12 ";
    apJsonInit(&j, s, (int) strlen(s));
    expect(apJsonInt(&j, &n) == 1 && n == -12, "negative int");

    s = "641010";
    apJsonInit(&j, s, (int) strlen(s));
    expect(apJsonInt(&j, &n) == 1 && n == 641010, "location id int");
}

static void testSkip(void)
{
    ApJson j;
    const char *s = "{\"a\":[1,{\"b\":\"x\"},true],\"c\":null}";
    apJsonInit(&j, s, (int) strlen(s));
    expect(apJsonSkip(&j) == 1, "skip object");
    expect(j.p == j.end, "skip consumed the object");
}

static void testPacket(void)
{
    const char *msg =
        "[{\"cmd\":\"RoomInfo\",\"version\":{\"major\":0,\"minor\":6,\"build\":4,\"class\":\"Version\"},"
        "\"seed_name\":\"99\"},{\"cmd\":\"ReceivedItems\",\"index\":0,\"items\":["
        "{\"item\":640001,\"location\":0,\"player\":1,\"flags\":1},"
        "{\"item\":640101,\"location\":641000,\"player\":2,\"flags\":2}]}]";
    ApJson j;
    int started = 0;
    int cmds = 0;
    int item = 0;
    int index = -1;

    apJsonInit(&j, msg, (int) strlen(msg));
    for (;;)
    {
        int step = apJsonArrayNext(&j, &started);
        const char *obj;
        ApJson scan, field;
        char cmd[32];
        if (step == 0)
        {
            break;
        }
        expect(step == 1, "packet element");
        if (step != 1)
        {
            break;
        }
        obj = j.p;
        scan = j;
        expect(apJsonSkip(&scan) == 1, "skip packet");
        cmd[0] = '\0';
        expect(findKey(obj, scan.p, "cmd", &field) == 1, "cmd key");
        expect(apJsonString(&field, cmd, (int) sizeof(cmd)) == 1, "cmd string");
        if (strcmp(cmd, "RoomInfo") == 0)
        {
            int major = -1;
            expect(findKey(obj, scan.p, "version", &field) == 1, "version");
            expect(findKey(field.p, scan.p, "major", &field) == 1, "major");
            expect(apJsonInt(&field, &major) == 1 && major == 0, "major 0");
            cmds++;
        }
        else if (strcmp(cmd, "ReceivedItems") == 0)
        {
            int arr = 0;
            expect(findKey(obj, scan.p, "index", &field) == 1, "index key");
            expect(apJsonInt(&field, &index) == 1 && index == 0, "index 0");
            expect(findKey(obj, scan.p, "items", &field) == 1, "items");
            arr = 0;
            for (;;)
            {
                int a = apJsonArrayNext(&field, &arr);
                const char *one;
                ApJson end, num;
                if (a == 0)
                {
                    break;
                }
                expect(a == 1, "item element");
                one = field.p;
                end = field;
                expect(apJsonSkip(&end) == 1, "skip item");
                expect(findKey(one, end.p, "item", &num) == 1, "item id key");
                expect(apJsonInt(&num, &item) == 1, "item id");
                field.p = end.p;
            }
            cmds++;
        }
        j = scan;
    }
    expect(cmds == 2, "two packets");
    expect(item == 640101, "last item is the PP7");
    expect(AP_LOC(0, 0) == 641000, "Dam Agent id");
    expect(AP_LOC(1, 2) == 641012, "Facility 00 id");
    expect(AP_LOC(19, 2) == 641192, "Egyptian 00 id");
    expect(AP_ITEM_DOSSIER(0) == 640001, "Dam dossier id");
    expect(AP_ITEM_WEAPON(23) == 640124, "last weapon id");
}

int main(void)
{
    testStrings();
    testSkip();
    testPacket();
    if (gFails != 0)
    {
        fprintf(stderr, "%d failed\n", gFails);
        return 1;
    }
    printf("ap_json_test ok\n");
    return 0;
}
