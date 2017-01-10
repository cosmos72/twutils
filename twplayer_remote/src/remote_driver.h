#ifndef TWPLAYER_REMOTE_DRIVER_H
#define TWPLAYER_REMOTE_DRIVER_H

#include <stddef.h> /* for size_t */

#include <Tw/Tw.h> /* for udat, tmenuitem */

typedef struct pl_driver_s * pl_driver;
typedef struct pl_trackinfo_s * pl_trackinfo;

struct pl_trackinfo_s {
    long duration, title_len, artist_len;
    char title[512], artist[512];
};

struct pl_driver_s {
    char * label;
    unsigned fatal_error;
    void (*del)(pl_driver pl);
    void (*play)(pl_driver pl);
    void (*pause)(pl_driver pl);
    void (*stop)(pl_driver pl);
    void (*next)(pl_driver pl);
    void (*prev)(pl_driver pl);
    long (*get_position)(pl_driver pl);
    void (*seek)(pl_driver pl, long offset);
    long (*get_volume)(pl_driver pl);
    long (*set_volume)(pl_driver pl, long volume); /* return actual volume */
    void (*get_trackinfo)(pl_driver pl, pl_trackinfo info);
};

typedef pl_driver (*pl_driver_constructor)(const char *arg);

typedef struct pl_factory_s   * pl_factory;
typedef struct pl_factories_s * pl_factories;

struct pl_factory_s {
    char *label, *constructor_arg;
    pl_driver_constructor constructor;
};

struct pl_factories_s {
    unsigned size, capacity;
    pl_factory * data;
};

static inline size_t min2(size_t a, size_t b) {
    return a < b ? a : b;
}

char * strdup_or_error(const char * str, int * error);

void init_driver(pl_driver pl, const char * name);
void clear_driver(pl_driver pl);

pl_factory new_factory(void);
void del_factory(pl_factory element);
pl_driver call_factory(pl_factory element);

pl_factories new_factories(void);
void append_new_factory(pl_factories collection, const char * label, const char * constructor_arg, pl_driver_constructor constructor);
void clear_factories(pl_factories collection);
void del_factories(pl_factories collection);


void mpris2_list_factories(pl_factories collection);

#endif /* TWPLAYER_REMOTE_DRIVER_H */
