#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dictparser.h"
void onPair(void* opaque, const char* key, int nkey, const char* value, int nvalue)
{
    printf("onPair - key: <%.*s> value: <%.*s>\n", nkey, key, nvalue, value);
}

static void* onRealloc(void* opaque, void* ptr, int size)
{
    return realloc(ptr, size);
}

int main()
{
    char test1[] = "a: 2\nage:23";

    struct dictparser_callbacks callbacks;
    callbacks.pair = onPair;
    callbacks.realloc_scratch = onRealloc;

    struct dictparser_roundtripper rt;
    dictparser_init(&rt, callbacks, NULL);

    dictparser_data(&rt, test1, strlen(test1));

    dictparser_free(&rt);

    return 0;
}
