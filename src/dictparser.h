/*-
 * Copyright 2014 Michal Nezerka
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

#ifndef DICTPARSER_H
#define DICTPARSER_H

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Callbacks for handling response data.
 *  realloc_scratch - reallocate memory, cannot fail. There will only
 *                    be one scratch buffer. Implemnentation may take
 *                    advantage of this fact.
 *  header - handle an dict key/value pair
 */
struct dictparser_callbacks {
    void* (*realloc_scratch)(void* opaque, void* ptr, int size);
    void (*pair)(void* opaque, const char* key, int nkey, const char* value, int nvalue);
};

struct dictparser_roundtripper {
    struct dictparser_callbacks funcs;
    void *opaque;
    char *scratch;
    int code;
    int parsestate;
    int state;
    int nscratch;
    int nkey;
    int nvalue;
};

enum dictparser_pair_status
{
    dictparser_pair_status_done,
    dictparser_pair_status_continue,
    dictparser_pair_status_version_character,
    dictparser_pair_status_code_character,
    dictparser_pair_status_status_character,
    dictparser_pair_status_key_character,
    dictparser_pair_status_value_character,
    dictparser_pair_status_store_keyvalue
};

/**
 * Initializes a rountripper with the specified callback functions. This must
 * be called before the rt object is used.
 */
void dictparser_init(struct dictparser_roundtripper* rt, struct dictparser_callbacks, void* opaque);

/**
 * Frees any scratch memory allocated during parsing.
 */
void dictparsser_free(struct dictparser_roundtripper* rt);

/**
 * Parses a block of raw data. Returns zero if the parser reached the
 * end of the data, or an error was encountered. Use dictparser_iserror to check
 * for the presence of an error. Returns non-zero if more data is required for
 * the response.
 */
int dictparser_data(struct dictparser_roundtripper* rt, const char* data, int size, int* read);

/**
 * Returns non-zero if a completed parser encounted an error. If dictparser_data did
 * not return non-zero, the results of this function are undefined.
 */
int dictparser_iserror(struct http_roundtripper* rt);



/**
 * Parses a single character of an HTTP header stream. The state parameter is
 * used as internal state and should be initialized to zero for the first call.
 * Return value is a value from the http_header_status enuemeration specifying
 * the semantics of the character. If an error is encountered,
 * http_header_status_done will be returned with a non-zero state parameter. On
 * success http_header_status_done is returned with the state parameter set to
 * zero.
 */
int http_parse_header_char(int* state, char ch);


#if defined(__cplusplus)
}
#endif

#endif
