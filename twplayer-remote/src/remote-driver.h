#ifndef TWPLAYER_REMOTE_DRIVER_H
#define TWPLAYER_REMOTE_DRIVER_H

#include <stdint.h> /* for int64_t */

typedef struct pl_driver_s * pl_driver;

struct pl_driver_s {
    void (*del)(pl_driver pl);
    void (*play)(pl_driver pl);
    void (*pause)(pl_driver pl);
    void (*stop)(pl_driver pl);
    void (*next)(pl_driver pl);
    void (*prev)(pl_driver pl);
    int64_t (*position)(pl_driver pl);
    void (*seek)(pl_driver pl, int64_t offset);
    int64_t (*volume)(pl_driver pl);
    void (*set_volume)(pl_driver pl, int64_t volume);
};

pl_driver mpris_driver_new(const char *remote);



#endif /* TWPLAYER_REMOTE_DRIVER_H */
