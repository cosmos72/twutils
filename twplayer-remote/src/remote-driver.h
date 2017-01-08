#ifndef TWPLAYER_REMOTE_DRIVER_H
#define TWPLAYER_REMOTE_DRIVER_H

typedef struct pl_driver_s * pl_driver;
typedef struct pl_trackinfo_s * pl_trackinfo;

struct pl_trackinfo_s {
    long duration, title_len, artist_len;
    char title[512], artist[512];
};

struct pl_driver_s {
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

pl_driver mpris2_driver_new(const char *remote);



#endif /* TWPLAYER_REMOTE_DRIVER_H */
