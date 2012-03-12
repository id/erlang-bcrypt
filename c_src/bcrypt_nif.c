/*
 * Copyright (c) 2011-2012 Hunter Morris <hunter.morris@smarkets.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "erl_nif.h"
#include "erl_blf.h"
#include "bcrypt_nif.h"

static ERL_NIF_TERM erl_encode_salt(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ErlNifBinary csalt, bin;
    unsigned long log_rounds;

    if (!enif_inspect_binary(env, argv[0], &csalt) || 16 != csalt.size) {
        return enif_make_badarg(env);
    }

    if (!enif_get_ulong(env, argv[1], &log_rounds)) {
        enif_release_binary(&csalt);
        return enif_make_badarg(env);
    }

    if (!enif_alloc_binary(64, &bin)) {
        enif_release_binary(&csalt);
        return enif_make_badarg(env);
    }

    encode_salt((char *)bin.data, (u_int8_t*)csalt.data, csalt.size, log_rounds);
    enif_release_binary(&csalt);

    return enif_make_string(env, (char *)bin.data, ERL_NIF_LATIN1);
}

static ERL_NIF_TERM hashpw(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    char pw[1024];
    char salt[1024];
    char *ret = NULL;

    (void)memset(&pw, '\0', sizeof(pw));
    (void)memset(&salt, '\0', sizeof(salt));

    if (enif_get_string(env, argv[0], pw, sizeof(pw), ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (enif_get_string(env, argv[1], salt, sizeof(salt), ERL_NIF_LATIN1) < 1)
        return enif_make_badarg(env);

    if (NULL == (ret = bcrypt(pw, salt)) || 0 == strcmp(ret, ":")) {
        return enif_make_badarg(env);
    }

    return enif_make_string(env, ret, ERL_NIF_LATIN1);
}

static ErlNifFunc bcrypt_nif_funcs[] =
{
    {"encode_salt", 2, erl_encode_salt},
    {"hashpw", 2, hashpw}
};

static void bcrypt_rt_dtor(ErlNifEnv* env, void* obj)
{
}

static int on_load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info)
{
    const char *mod = "bcrypt_nif";
    const char *name = "nif_resource";

    ErlNifResourceFlags flags = (ErlNifResourceFlags)(ERL_NIF_RT_CREATE | ERL_NIF_RT_TAKEOVER);

    bcrypt_privdata_t *priv = (bcrypt_privdata_t*)enif_alloc(sizeof(bcrypt_privdata_t));
    priv->bcrypt_rt = enif_open_resource_type(env, mod, name, bcrypt_rt_dtor, flags, NULL);
    if (priv->bcrypt_rt == NULL)
        return -1;
    *priv_data = priv;
    return 0;
}

ERL_NIF_INIT(bcrypt_nif, bcrypt_nif_funcs, &on_load, NULL, NULL, NULL)
