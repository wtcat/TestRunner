/*
 * Copyright 2022 wtcat
 */
#include <errno.h>
#include <string.h>

#include "basework/lib/env_core.h"

struct env_ram {
    char *buffer;
    size_t offset;
    size_t maxlen;
    size_t total;
};
#define MAX_KEY_SIZE 64

static _ENV_DEFINE(sysenv, NULL, NULL, NULL);

int env_set(const char *name, const char *value, int overwrite) {
    return _env_set(&sysenv, name, value, overwrite);
}

int env_unset(const char *name) {
    return _env_unset(&sysenv, name);
}

char *env_get(const char *name) {
    return _env_get(&sysenv, name);
}

int env_load(int (*readline_cb)(void *ctx, void *buffer, size_t max_size),
    void *ctx) {
    return _env_load(&sysenv, readline_cb, ctx);
}

int env_flush(int (*write_cb)(void *ctx, void *buffer, size_t size),
    void *ctx) {
    return _env_flush(&sysenv, write_cb, ctx);
}

void env_reset(void) {
    _env_free(&sysenv);
}

static int env_ram_write(void *ctx, void *buffer, size_t size) {
    struct env_ram *ram = (struct env_ram *)ctx;
    size_t remain = ram->maxlen - ram->offset;
    size_t bytes;

    if (remain == 0)
        return -EINVAL;
    bytes = remain > size? size: remain;
    memcpy(ram->buffer + ram->offset, buffer, bytes);
    ram->offset += bytes;
    ram->total += bytes;
    return 0;
}

static int env_ram_readline(void *ctx, void *buffer, size_t max_size) {
    struct env_ram *ram = (struct env_ram *)ctx;
    size_t ofs = ram->offset;
    char *start;
    char *p;

    if (ofs >= ram->maxlen)
        return -EINVAL;

    start = ram->buffer + ofs;
    p = strchr(start, '=');
    if (p) {
        size_t len;
        if (p - start > MAX_KEY_SIZE)
            return -EINVAL;
        p = strchr(p, '\n');
        if (!p)
            return -EINVAL;

        len = p - start + 1;
        if (len < ENV_MAX && (ofs + len) < ram->maxlen) {
            memcpy(buffer, start, len);
            ram->offset = ofs + len;
            return len;
        }
    }  

    return -EINVAL;
}

int env_load_ram(void *input, size_t size) {
    struct env_ram ram;

    if (!input || !size)
        return 0;

    ram.buffer = input;
    ram.maxlen = size;
    ram.offset = 0;
    ram.total = 0;
    return env_load(env_ram_readline, &ram);
}

int env_flush_ram(void *buffer, size_t maxlen) {
    struct env_ram ram;
    int err;

    if (!buffer || !maxlen)
        return -EINVAL;

    ram.buffer = (char *)buffer;
    ram.maxlen = maxlen;
    ram.offset = 0;
    ram.total = 0;
    err = env_flush(env_ram_write, &ram);
    if (err) 
        return err;
    return ram.total;
}
