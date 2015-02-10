/*-
 * Copyright 2012 Michal Nezerka
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "dictparser.h"

#include <ctype.h>
#include <string.h>

// lower four bits are used for state
// high four bits are used for semantic value of character
static unsigned char dictparser_pair_state[] = {
//     0     1     2     3     4     5
//     *    \t    \n    \r    ' '     :
    0x10,    5,    5,    5,    1,    2,  // state 0: start of pair line (reading key)
       5,    5,    5,    5,    1,    2,  // state 1: whitespace before pair delimiter ":"
    0x14,    2,    3,    2,    2, 0x14,  // state 2: leading whitespace before value
    0x10,    5,    3,    3, 0x10, 0x10,  // state 3: last CR before end of pair line
    0x14,    4, 0x13,    4, 0x14, 0x14,  // state 4: value
       5,    5,    5,    5,    5,    5,  // state 5: error
};

static void grow_scratch(struct dictparser_roundtripper* rt, int size)
{
    if (rt->nscratch >= size)
        return;

    if (size < 64)
        size = 64;
    int nsize = (rt->nscratch * 3) / 2;
    if (nsize < size)
        nsize = size;

	rt->scratch = (char*)rt->funcs.realloc_scratch(rt->opaque, rt->scratch, nsize);
    rt->nscratch = nsize;
}

void dictparser_init(struct dictparser_roundtripper* rt, struct dictparser_callbacks funcs, void* opaque)
{
    rt->funcs = funcs;
    rt->scratch = 0;
    rt->opaque = opaque;
    rt->parsestate = 0;
    rt->state = dictparser_status_key_character;
    rt->nscratch = 0;
    rt->nkey = 0;
    rt->nvalue = 0;
}

void dictparser_free(struct dictparser_roundtripper* rt)
{
    if (rt->scratch) {
        rt->funcs.realloc_scratch(rt->opaque, rt->scratch, 0);
        rt->scratch = 0;
    }
}

int dictparser_data(struct dictparser_roundtripper* rt, const char* data, int size)
{
    int newState = 0;
    while (size)
    {
        int code = 0;

        switch (*data)
        {
            case '\t': code = 1; break;
            case '\n': code = 2; break;
            case '\r': code = 3; break;
            case  ' ': code = 4; break;
            case  ':': code = 5; break;
        }

        newState = dictparser_pair_state[rt->parsestate * 6 + code];
        rt->parsestate = (newState & 0xF);

        switch (newState)
        {
            // collecting key characters
            case 0x10:
                grow_scratch(rt, rt->nkey + 1);
                rt->scratch[rt->nkey] = tolower(*data);
                ++rt->nkey;
                break;

            // collecting value characters
            case 0x14:
                grow_scratch(rt, rt->nkey + rt->nvalue + 1);
                rt->scratch[rt->nkey+rt->nvalue] = *data;
                ++rt->nvalue;
                break;

            // pair is complete
            case 0x13:
                rt->funcs.pair(rt->opaque, rt->scratch, rt->nkey, rt->scratch + rt->nkey, rt->nvalue);
                rt->nkey = 0;
                rt->nvalue = 0;
                break;
        }

        --size;
        ++data;
    }

    // process last key-value pair without newline after value
    if (newState == 0x14)
    {
        rt->funcs.pair(rt->opaque, rt->scratch, rt->nkey, rt->scratch + rt->nkey, rt->nvalue);
        rt->nkey = 0;
        rt->nvalue = 0;
    }

    if (rt->scratch)
    {
        rt->funcs.realloc_scratch(rt->opaque, rt->scratch, 0);
        rt->scratch = 0;
    }
    return 0;
}

int dictparser_iserror(struct dictparser_roundtripper* rt)
{
    return rt->state == dictparser_status_error;
}
