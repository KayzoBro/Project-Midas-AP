#ifndef AP_JSON_H
#define AP_JSON_H

/* Tiny cursor over one JSON value. No allocation. Enough for Archipelago packets. */

typedef struct ApJson {
    const char *p;
    const char *end;
} ApJson;

void apJsonInit(ApJson *j, const char *text, int len);

/* 1 if the next non-space char was c and it was consumed. */
int apJsonEat(ApJson *j, char c);

/* Decode one JSON string into buf (always NUL-terminated). Truncates. 1 on success. */
int apJsonString(ApJson *j, char *buf, int buflen);

/* JSON number as a 32-bit int. Fraction and exponent are consumed and truncated. */
int apJsonInt(ApJson *j, int *out);

/* Skip one full value. 1 on success. */
int apJsonSkip(ApJson *j);

/*
 * Object walk. *started must be 0 and j must point at '{'.
 * 1 = key written, j now at the value.
 * 0 = object ended.
 * -1 = malformed.
 */
int apJsonObjectNext(ApJson *j, char *key, int keylen, int *started);

/* Array walk. Same return codes. On 1, j is at the next value. */
int apJsonArrayNext(ApJson *j, int *started);

#endif
