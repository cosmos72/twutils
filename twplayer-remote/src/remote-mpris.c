#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbus/dbus.h> 

#include "remote-driver.h"

typedef struct mpris_driver_s * mpris_driver;

struct mpris_driver_s {
    struct pl_driver_s base;
    DBusConnection * connection;
    DBusError error;
    const char *dest; /* "org.mpris.MediaPlayer2.audacious" */
    const char *path; /* "/org/mpris/MediaPlayer2" */
};

#define OFFSET_OF(ptr, struct_name, member_name)    ((char const *)&(((struct_name const *)(ptr))->member_name) - (char const *)((struct_name const *)(ptr)))

#define CONTAINER_OF(ptr, struct_name, member_name) ((struct_name *)((char *)(ptr) - OFFSET_OF(ptr, struct_name, member_name)))

#define TO_MPRIS_DRIVER(base_ptr)                   CONTAINER_OF(base_ptr, struct mpris_driver_s, base)

typedef struct mpris_arg_s * mpris_arg;

struct mpris_arg_s {
    int type;
    union {
        double d;
        dbus_int64_t i64;
        dbus_uint64_t u64;
        dbus_int32_t i32;
        dbus_uint32_t u32;
        dbus_int16_t i16;
        dbus_uint16_t u16;
        char * str;
        unsigned char byte;
        dbus_bool_t boolean;
    };
};

static void parse_iter(DBusMessageIter *iter) {
    char * val_str;
    union {
        DBusMessageIter subiter;
        double d;
        dbus_int64_t i64;
        dbus_uint64_t u64;
        dbus_int32_t i32;
        dbus_uint32_t u32;
        dbus_int16_t i16;
        dbus_uint16_t u16;
        char * str;
        unsigned char byte;
        dbus_bool_t boolean;
    } val;
    int type;
    
    do {
        type = dbus_message_iter_get_arg_type (iter);
        if (type == DBUS_TYPE_INVALID)
            break;
        switch (type) {
        case DBUS_TYPE_STRING:
            dbus_message_iter_get_basic (iter, &val.str);
            printf("string: %s\n", val.str);
            break;
        case DBUS_TYPE_SIGNATURE:
        case DBUS_TYPE_OBJECT_PATH:
            break;
        case DBUS_TYPE_INT16:
            dbus_message_iter_get_basic (iter, &val.i16);
            printf("int16: %d\n", (int)val.i16);
            break;
        case DBUS_TYPE_UINT16:
            dbus_message_iter_get_basic (iter, &val.u16);
            printf("uint16: %u\n", (unsigned)val.u16);
            break;
        case DBUS_TYPE_INT32:
            dbus_message_iter_get_basic (iter, &val.i32);
            printf("int32: %u\n", (int)val.i32);
            break;
        case DBUS_TYPE_UINT32:
            dbus_message_iter_get_basic (iter, &val.u32);
            printf("uint32: %u\n", (unsigned)val.u32);
            break;
        case DBUS_TYPE_INT64:
            dbus_message_iter_get_basic (iter, &val.i64);
#ifdef DBUS_INT64_PRINTF_MODIFIER
            printf("int64: %" DBUS_INT64_PRINTF_MODIFIER "d\n", val.i64);
#else 
            printf("int64: %lld\n", (long long)val.i64);
#endif
            break;
        case DBUS_TYPE_UINT64:
            dbus_message_iter_get_basic (iter, &val.u64);
#ifdef DBUS_INT64_PRINTF_MODIFIER
            printf("uint64: %" DBUS_INT64_PRINTF_MODIFIER "u\n", val.i64);
#else 
            printf("uint64: %llu\n", (unsigned long long)val.i64);
#endif
            break;
        case DBUS_TYPE_DOUBLE:
            dbus_message_iter_get_basic (iter, &val.d);
            printf("double: %g\n", val); 
            break;
        case DBUS_TYPE_BYTE:
            dbus_message_iter_get_basic (iter, &val.byte);
            printf("byte %u\n", (unsigned)val.byte);
            break;
        case DBUS_TYPE_BOOLEAN:
            dbus_message_iter_get_basic (iter, &val.boolean);
            printf("boolean: %s\n", val.boolean ? "true" : "false");
            break;
        case DBUS_TYPE_VARIANT:
            dbus_message_iter_recurse (iter, &val.subiter);
            printf("variant:\n");
            parse_iter(&val.subiter);
            break;
        case DBUS_TYPE_ARRAY:
        case DBUS_TYPE_DICT_ENTRY:
        case DBUS_TYPE_STRUCT:
#ifdef DBUS_UNIX
        case DBUS_TYPE_UNIX_FD:
#endif
            /* TODO */
            printf ("not-yet supported type=%d\n", type);
            break;
        default:
            printf ("unknown type=%d\n", type);
            break;
        }
    } while (dbus_message_iter_next (iter));
}

static void parse_message(DBusMessage *message) {
    DBusMessageIter iter;
    /*
     const char * sender = dbus_message_get_sender(message);
     const char *  destination = dbus_message_get_destination(message);
     */
    int message_type = dbus_message_get_type(message);
    switch (message_type) {
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
    case DBUS_MESSAGE_TYPE_SIGNAL:
        printf(" serial=%u path=%s; interface=%s; member=%s\n",
               dbus_message_get_serial(message),
               dbus_message_get_path(message),
               dbus_message_get_interface(message),
               dbus_message_get_member(message)); 
        break;
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
        printf(" serial=%u reply_serial=%u\n",
               dbus_message_get_serial(message),
               dbus_message_get_reply_serial(message));
        break;
    case DBUS_MESSAGE_TYPE_ERROR:
        printf(" error_name=%s reply_serial=%u\n",
               dbus_message_get_error_name (message),
               dbus_message_get_reply_serial(message));
        break;
    default:
        printf(" unknown message type=%d\n",
               message_type);
        break;
    }
    dbus_message_iter_init (message, &iter);
    parse_iter(&iter); 
    
    fflush(stdout);
}

static void dbus_append_arg(DBusMessageIter * iter, int type, const void * arg)
{
    dbus_message_iter_append_basic(iter, type, arg);
}
    
static void dbus_append_args(DBusMessage *message, DBusMessageIter * iter, va_list va)
{
    DBusMessageIter container_iter, *target_iter;
    const void * arg;
    int container_type, type;
    
    dbus_message_iter_init_append(message, iter);
    
    while ((container_type = va_arg(va, int)) != DBUS_TYPE_INVALID) {
        if (container_type == DBUS_TYPE_VARIANT) {
            char sig[2];
            sig[0] = type = va_arg(va, int);
            sig[1] = '\0';
            dbus_message_iter_open_container (iter, container_type, sig, &container_iter);
            target_iter = &container_iter;
        } else {
            target_iter = iter;
            type = container_type;
            container_type = DBUS_TYPE_INVALID;
        }
            
        arg = va_arg(va, const void *);
        dbus_append_arg(target_iter, type, arg);
        
        if (container_type != DBUS_TYPE_INVALID)
            dbus_message_iter_close_container(iter, &container_iter);
    }
}

static const char * const mpris_interface = "org.mpris.MediaPlayer2.Player";
static const char * const props_interface = "org.freedesktop.DBus.Properties";

static void dbus_call(pl_driver pl_base, int parse_reply, const char * interface, const char * method, ...) {
    mpris_driver pl = TO_MPRIS_DRIVER(pl_base);
    DBusMessage *message = dbus_message_new_method_call(NULL, pl->path, (interface = interface ? interface : mpris_interface), method);
    DBusMessageIter iter;
    DBusMessage *reply = NULL;
    int reply_timeout = -1;

    // parse_reply = 1;
    
    if (message == NULL) {
        fprintf(stderr, "Failed to allocate D-Bus message\n");
        return;
    }
    dbus_message_set_auto_start(message, TRUE);
    if (pl->dest && !dbus_message_set_destination (message, pl->dest)) {
        fprintf(stderr, "D-Bus error: dbus_message_set_destination() failed! out of memory?\n");
        goto cleanup;
    }
    /* prepare args */
    {
        va_list va;
        va_start(va, method);
        dbus_append_args(message, &iter, va);
        va_end(va);
    }
    dbus_error_init(&pl->error);
    if (parse_reply)
        reply = dbus_connection_send_with_reply_and_block(pl->connection, message, reply_timeout, &pl->error);
    else {
        dbus_connection_send(pl->connection, message, NULL);
        dbus_connection_flush(pl->connection);
    }
    if (dbus_error_is_set(&pl->error)) {
        fprintf(stderr, "D-Bus: method invocation %s %s %s.%s returned error %s: %s\n",
                 pl->dest, pl->path, interface, method, pl->error.name, pl->error.message);
        goto cleanup;
    }
    if (reply)
        parse_message(reply);
cleanup:
    if (reply)
        dbus_message_unref(reply); 
    if (message)
        dbus_message_unref(message); 
}

static inline void mpris_driver_set_property(pl_driver pl, const char * property_name, int property_type, const void * property_value) {
    dbus_call(pl, 0, props_interface, "Set", DBUS_TYPE_STRING, & mpris_interface, DBUS_TYPE_STRING, & property_name, DBUS_TYPE_VARIANT, property_type, property_value, DBUS_TYPE_INVALID);
}

static void mpris_driver_play(pl_driver pl) {
    dbus_call(pl, 0, NULL, "Play", DBUS_TYPE_INVALID);
}

static void mpris_driver_pause(pl_driver pl) {
    dbus_call(pl, 0, NULL, "Pause", DBUS_TYPE_INVALID);
}

static void mpris_driver_stop(pl_driver pl) {
    dbus_call(pl, 0, NULL, "Stop", DBUS_TYPE_INVALID);
}

static void mpris_driver_next(pl_driver pl) {
    dbus_call(pl, 0, NULL, "Next", DBUS_TYPE_INVALID);
}

static void mpris_driver_prev(pl_driver pl) {
    dbus_call(pl, 0, NULL, "Previous", DBUS_TYPE_INVALID);
}

static int64_t mpris_driver_position(pl_driver pl) {
    /* TODO */
    return 0;
}

static void mpris_driver_seek(pl_driver pl, int64_t offset) {
    dbus_call(pl, 0, NULL, "Seek", DBUS_TYPE_INT64, &offset, DBUS_TYPE_INVALID);
}

static double mpris_driver_volume(pl_driver pl) {
    /* TODO */
    return 0.0;   
}

static void mpris_driver_set_volume(pl_driver pl, double volume) {
    mpris_driver_set_property(pl, "Volume", DBUS_TYPE_DOUBLE, &volume);
}


static void mpris_driver_del(pl_driver pl_base)
{
    if (pl_base) {
        mpris_driver pl = TO_MPRIS_DRIVER(pl_base);
        if (pl->connection)
            dbus_connection_unref(pl->connection);
        free(pl);
    }
}

static const struct pl_driver_s mpris_funcs = {
    mpris_driver_del,
    mpris_driver_play,
    mpris_driver_pause,
    mpris_driver_stop,
    mpris_driver_next,
    mpris_driver_prev,
    mpris_driver_position,
    mpris_driver_seek,
    mpris_driver_volume,
    mpris_driver_set_volume,
};
    
pl_driver mpris_driver_new(const char *dest) {
    mpris_driver pl = (mpris_driver)calloc(1, sizeof(struct mpris_driver_s));
    if (!pl)
        return NULL;
    
    dbus_error_init(&pl->error); 
    
    pl->connection = dbus_bus_get(DBUS_BUS_SESSION, &pl->error);
    if (pl->connection == NULL) {
        fprintf(stderr, "Failed to open connection to D-Bus session message bus: %s\n",
                pl->error.message);
        goto fail;
    }

    if (!dbus_validate_bus_name(dest, &pl->error)) {
        fprintf(stderr, "invalid D-Bus destination '%s'\n", dest);
        goto fail;
    }

    pl->base = mpris_funcs; /* struct copy */
    pl->dest = dest; /* "org.mpris.MediaPlayer2.audacious" */
    pl->path = "/org/mpris/MediaPlayer2";
            
    return & pl->base;
    
fail:
    mpris_driver_del(& pl->base);
    return NULL;
}
