#include <stdarg.h>
#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>

#include <dbus/dbus.h> 

#include "remote_driver.h"

typedef struct mpris2_driver_s * mpris2_driver;

struct mpris2_driver_s {
    struct pl_driver_s base;
    DBusConnection * connection;
    DBusError error;
    char *dest;       /* for example "org.mpris.MediaPlayer2.audacious" */
    const char *path; /* typically "/org/mpris/MediaPlayer2" */
};

#define OFFSET_OF(ptr, struct_name, member_name)    ((char const *)&(((struct_name const *)(ptr))->member_name) - (char const *)((struct_name const *)(ptr)))

#define CONTAINER_OF(ptr, struct_name, member_name) ((struct_name *)((char *)(ptr) - OFFSET_OF(ptr, struct_name, member_name)))

#define TO_MPRIS2_DRIVER(base_ptr)                   CONTAINER_OF(base_ptr, struct mpris2_driver_s, base)

typedef struct darg_s * darg;

struct darg_s {
    int type;
    union {
        double d;
        dbus_int64_t i64;
        dbus_uint64_t u64;
        char * str;
    };
};

void darg_unref(darg arg) {
    if (arg != NULL && arg->type == DBUS_TYPE_STRING)
    {
        free(arg->str);
        arg->str = NULL;
    }
}

static void diter_parse(DBusMessageIter *iter, darg result) {
    int type;
    
    do {
        result->type = type = dbus_message_iter_get_arg_type (iter);
        if (type == DBUS_TYPE_INVALID)
            break;
        switch (type) {
        case DBUS_TYPE_STRING:
	    {
	        const char * str = NULL;
	        dbus_message_iter_get_basic (iter, &str);
	        result->str = str ? strdup(str) : NULL;
	        break;
	    }
        case DBUS_TYPE_SIGNATURE:
        case DBUS_TYPE_OBJECT_PATH:
            break;
        case DBUS_TYPE_INT16:
	    {
	        int16_t val = 0;
	        dbus_message_iter_get_basic (iter, &val);
	        result->type = DBUS_TYPE_INT64;
	        result->i64 = val;
	        break;
	    }
        case DBUS_TYPE_UINT16:
	    {
	        uint16_t val = 0;
	        dbus_message_iter_get_basic (iter, &val);
	        result->type = DBUS_TYPE_UINT64;
	        result->u64 = val;
	        break;
	    }
        case DBUS_TYPE_INT32:
	    {
	        int32_t val = 0;
	        dbus_message_iter_get_basic (iter, &val);
	        result->type = DBUS_TYPE_INT64;
	        result->i64 = val;
	        break;
	    }
        case DBUS_TYPE_UINT32:
	    {
	        uint32_t val = 0;
	        dbus_message_iter_get_basic (iter, &val);
	        result->type = DBUS_TYPE_UINT64;
	        result->u64 = val;
	    }
            break;
        case DBUS_TYPE_INT64:
	    result->i64 = 0;
            dbus_message_iter_get_basic (iter, &result->i64);
            break;
        case DBUS_TYPE_UINT64:
	    result->u64 = 0;
            dbus_message_iter_get_basic (iter, &result->u64);
            break;
        case DBUS_TYPE_DOUBLE:
	    result->d = 0.0;
            dbus_message_iter_get_basic (iter, &result->d);
            break;
        case DBUS_TYPE_BYTE:
	    {
	        unsigned char val = 0;
	        dbus_message_iter_get_basic (iter, &val);
	        result->type = DBUS_TYPE_UINT64;
	        result->u64 = val;
	        break;
	    }
        case DBUS_TYPE_BOOLEAN:
	    {
	        dbus_bool_t val = FALSE;
	        dbus_message_iter_get_basic (iter, &val);
	        result->type = DBUS_TYPE_UINT64;
	        result->u64 = val == TRUE ? 1 : 0;
	        break;
	    }
        case DBUS_TYPE_VARIANT:
	    {
	        DBusMessageIter subiter;
	        dbus_message_iter_recurse (iter, &subiter);
	        diter_parse(&subiter, result);
	        break;
	    }
        case DBUS_TYPE_ARRAY:
        case DBUS_TYPE_DICT_ENTRY:
        case DBUS_TYPE_STRUCT:
#ifdef DBUS_UNIX
        case DBUS_TYPE_UNIX_FD:
#endif
            /* TODO */
            printf ("unsupported type=%d\n", type);
            break;
        default:
            printf ("unknown type=%d\n", type);
            break;
        }
    } while (0 /*dbus_message_iter_next (iter)*/);
}

static void dmessage_parse(DBusMessage *message, darg result) {
    if (!result)
        return;
    
    result->type = DBUS_TYPE_INVALID;
    
    if (message != NULL) {
        DBusMessageIter iter;
        int message_type = dbus_message_get_type(message);
        
        switch (message_type) {
        case DBUS_MESSAGE_TYPE_METHOD_CALL:
        case DBUS_MESSAGE_TYPE_SIGNAL:
            break;
        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            dbus_message_iter_init (message, &iter);
            diter_parse(&iter, result); 
            break;
        case DBUS_MESSAGE_TYPE_ERROR:
            printf(" error_name=%s reply_serial=%u\n",
                   dbus_message_get_error_name (message),
                   dbus_message_get_reply_serial(message));
            break;
        default:
            printf(" unknown message type=%d\n", message_type);
            break;
	}
        fflush(stdout);
    }
}

static void dparse_keyvaluestr_trackinfo(const char * key, const char * value, pl_trackinfo info) {
    if (!strcmp(key, "xesam:title"))
        memcpy(info->title, value, info->title_len = min2(strlen(value), sizeof(info->title)));
    else if (!strcmp(key, "xesam:artist"))
        memcpy(info->artist, value, info->artist_len = min2(strlen(value), sizeof(info->artist)));
}

static void dparse_keyvaluei64_trackinfo(const char * key, int64_t value, pl_trackinfo info) {
    if (!strcmp(key, "mpris:length"))
        info->duration = value / 1000;
}

static void dparse_dictentry_trackinfo(DBusMessageIter * iter, pl_trackinfo info) {
    DBusMessageIter iter2, iter3, iter4;
    DBusMessageIter * value_iter;
    const char * key = NULL;
    int type;
    
    dbus_message_iter_recurse (iter, &iter2);
    if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_STRING)
        return;
    
    dbus_message_iter_get_basic(&iter2, &key);
    if (!key || !*key)
        return;
    
    dbus_message_iter_next (&iter2);
    type = dbus_message_iter_get_arg_type(value_iter = &iter2);
    
    if (type == DBUS_TYPE_VARIANT || type == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse (&iter2, &iter3);
        type = dbus_message_iter_get_arg_type (value_iter = &iter3);

        if (type == DBUS_TYPE_VARIANT || type == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse (&iter3, &iter4);
            type = dbus_message_iter_get_arg_type (value_iter = &iter4);
        }
    }
    
    switch (type) {
    case DBUS_TYPE_STRING:
        {
            const char * value = NULL;
            dbus_message_iter_get_basic(value_iter, &value);
            if (value && *value)
                dparse_keyvaluestr_trackinfo(key, value, info);
            break;
        }
    case DBUS_TYPE_INT64:
        {
            int64_t value = 0;
            dbus_message_iter_get_basic(value_iter, &value);
            dparse_keyvaluei64_trackinfo(key, value, info);
            break;
        }
    default:
        break;
    }
}


static void dparse_reply_trackinfo(DBusMessage *reply, pl_trackinfo info) {
    DBusMessageIter iter1, iter2, iter3;
    
    if (reply == NULL || dbus_message_get_type(reply) != DBUS_MESSAGE_TYPE_METHOD_RETURN)
        return;
    
    dbus_message_iter_init (reply, &iter1);
    if (dbus_message_iter_get_arg_type(&iter1) != DBUS_TYPE_VARIANT)
        return;

    dbus_message_iter_recurse (&iter1, &iter2);
    if (dbus_message_iter_get_arg_type(&iter2) != DBUS_TYPE_ARRAY)
        return;
    
    dbus_message_iter_recurse (&iter2, &iter3);
    while (dbus_message_iter_get_arg_type(&iter3) == DBUS_TYPE_DICT_ENTRY) {
        
        dparse_dictentry_trackinfo(&iter3, info);
        
        dbus_message_iter_next (&iter3);
    }
}

static double to_double(darg arg)
{
    double d;
    switch (arg->type)
    {
    case DBUS_TYPE_DOUBLE:
        d = arg->d;
        break;
    case DBUS_TYPE_UINT64:
        d = arg->u64 / 100.0;
        break;
    case DBUS_TYPE_INT64:
        d = arg->i64 / 100.0;
        break;
    default:
        d = 0.0;
        break;
    }
    return d;
}

static double to_int64(darg arg)
{
    int64_t i;
    switch (arg->type)
    {
    case DBUS_TYPE_DOUBLE:
        i = (int64_t)(arg->d * 100.0 + 0.5);
        break;
    case DBUS_TYPE_UINT64:
        i = (int64_t)arg->u64;
        break;
    case DBUS_TYPE_INT64:
        i = arg->i64;
        break;
    default:
        i = 0;
        break;
    }
    return i;
}

static void dcall_append_arg(DBusMessageIter * iter, int type, const void * arg)
{
    dbus_message_iter_append_basic(iter, type, arg);
}

static void dcall_append_args_v(DBusMessage *message, DBusMessageIter * iter, va_list va)
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
        dcall_append_arg(target_iter, type, arg);
        
        if (container_type != DBUS_TYPE_INVALID)
            dbus_message_iter_close_container(iter, &container_iter);
    }
}

static void dcall_append_args(DBusMessage *message, DBusMessageIter * iter, ...) {
    va_list va;
    va_start(va, iter);
    dcall_append_args_v(message, iter, va);
    va_end(va);
}


static const char * const mpris2_interface = "org.mpris.MediaPlayer2.Player";
static const char * const props_interface = "org.freedesktop.DBus.Properties";

static DBusMessage * dcall_prepare(mpris2_driver pl, const char * interface, const char * method) {
    DBusMessage *message = dbus_message_new_method_call(NULL, pl->path, (interface = interface ? interface : mpris2_interface), method);
    if (message == NULL) {
        fprintf(stderr, "Failed to allocate D-Bus message\n");
        goto cleanup;
    }
    dbus_message_set_auto_start(message, TRUE);
    if (pl->dest && !dbus_message_set_destination (message, pl->dest)) {
        fprintf(stderr, "D-Bus error: dbus_message_set_destination() failed! out of memory?\n");
        goto cleanup;
    }
    return message;
    
cleanup:
    if (message)
        dbus_message_unref(message);
    return NULL;
}

static DBusMessage * dcall_exec_get_reply(mpris2_driver pl, DBusMessage * message) {
    DBusMessage *reply = NULL;
    int reply_timeout = -1;
    
    dbus_error_init(&pl->error);
    reply = dbus_connection_send_with_reply_and_block(pl->connection, message, reply_timeout, &pl->error);
    
    if (dbus_error_is_set(&pl->error)) {
        fprintf(stderr, "D-Bus: method invocation %s %s %s.%s returned error %s: %s\n",
                pl->dest, pl->path, dbus_message_get_interface(message), dbus_message_get_member(message), pl->error.name, pl->error.message);
        if (reply)
            dbus_message_unref(reply); 
        reply = NULL;
    }
    return reply;
}

static void dcall_exec_no_reply(mpris2_driver pl, DBusMessage * message) {
    dbus_connection_send(pl->connection, message, NULL);
    dbus_connection_flush(pl->connection);
}

static void dcall_unref(DBusMessage * message, DBusMessage * reply) {
    if (reply)
        dbus_message_unref(reply);
    if (message)
        dbus_message_unref(message);
}


static void dcall(pl_driver pl_base, darg result, const char * interface, const char * method, ...) {
    mpris2_driver pl = TO_MPRIS2_DRIVER(pl_base);
    DBusMessage *message = dcall_prepare(pl, interface, method);
    
    if (message != NULL) {
        DBusMessageIter iter;
        DBusMessage *reply = NULL;
        va_list va;
        
        va_start(va, method);
        dcall_append_args_v(message, &iter, va);
        va_end(va);
        
        if (result)
            reply = dcall_exec_get_reply(pl, message);
        else
            dcall_exec_no_reply(pl, message);
        
        if (result)
            dmessage_parse(reply, result);
        
        dcall_unref(message, reply);
    }
}

static inline void mpris2_get_property(pl_driver pl_base, darg arg, const char * property_name) {
    dcall(pl_base, arg, props_interface, "Get", DBUS_TYPE_STRING, & mpris2_interface, DBUS_TYPE_STRING, & property_name, DBUS_TYPE_INVALID);
}

static inline void mpris2_set_property(pl_driver pl_base, dbus_bool_t blocking, const char * property_name, int property_type, const void * property_value) {
    struct darg_s arg;
    dcall(pl_base, (blocking ? &arg : NULL), props_interface, "Set", DBUS_TYPE_STRING, & mpris2_interface, DBUS_TYPE_STRING, & property_name, DBUS_TYPE_VARIANT, property_type, property_value, DBUS_TYPE_INVALID);
}

static void mpris2_play(pl_driver pl_base) {
    dcall(pl_base, NULL, NULL, "Play", DBUS_TYPE_INVALID);
}

static void mpris2_pause(pl_driver pl_base) {
    dcall(pl_base, NULL, NULL, "Pause", DBUS_TYPE_INVALID);
}

static void mpris2_stop(pl_driver pl_base) {
    dcall(pl_base, NULL, NULL, "Stop", DBUS_TYPE_INVALID);
}

static void mpris2_next(pl_driver pl_base) {
    dcall(pl_base, NULL, NULL, "Next", DBUS_TYPE_INVALID);
}

static void mpris2_prev(pl_driver pl_base) {
    dcall(pl_base, NULL, NULL, "Previous", DBUS_TYPE_INVALID);
}

static long mpris2_get_position(pl_driver pl_base) {
    struct darg_s arg;
    mpris2_get_property(pl_base, &arg, "Position");
    darg_unref(&arg);
    return to_int64(&arg) / 1000;
}

static void mpris2_seek(pl_driver pl_base, long offset) {
    int64_t offset64 = (int64_t)offset * 1000;
    dcall(pl_base, NULL, NULL, "Seek", DBUS_TYPE_INT64, &offset64, DBUS_TYPE_INVALID);
}

static long mpris2_get_volume(pl_driver pl_base) {
    struct darg_s arg;
    mpris2_get_property(pl_base, &arg, "Volume");
    darg_unref(&arg);
    return (long)(to_double(&arg) * 100.0 + 0.5);
}

static long mpris2_set_volume(pl_driver pl_base, long volume) {
    double vol_double = volume / 100.0;
    mpris2_set_property(pl_base, FALSE, "Volume", DBUS_TYPE_DOUBLE, &vol_double);
    
    return mpris2_get_volume(pl_base);
}

    
static void mpris2_get_trackinfo(pl_driver pl_base, pl_trackinfo info) {
    mpris2_driver pl = TO_MPRIS2_DRIVER(pl_base);
    DBusMessage *message = dcall_prepare(pl, props_interface, "Get");
    
    if (message != NULL) {
        DBusMessageIter iter;
        DBusMessage *reply = NULL;
        const char * const property_name = "Metadata";

        dcall_append_args(message, & iter, DBUS_TYPE_STRING, & mpris2_interface, DBUS_TYPE_STRING, & property_name, DBUS_TYPE_INVALID);

        reply = dcall_exec_get_reply(pl, message);

        dparse_reply_trackinfo(reply, info);
        
        dcall_unref(message, reply);
    }
}


static void mpris2_del(pl_driver pl_base)
{
    if (pl_base) {
        mpris2_driver pl = TO_MPRIS2_DRIVER(pl_base);
        if (pl->connection)
            dbus_connection_unref(pl->connection);
        free(pl->dest); /* we made our own copy of pl->dest in ddriver_init() below */
        clear_driver(pl_base);
        free(pl);
    }
}

static const struct pl_driver_s mpris2_funcs = {
    NULL,
    mpris2_del,
    mpris2_play,
    mpris2_pause,
    mpris2_stop,
    mpris2_next,
    mpris2_prev,
    mpris2_get_position,
    mpris2_seek,
    mpris2_get_volume,
    mpris2_set_volume,
    mpris2_get_trackinfo,
};

static int ddriver_init(mpris2_driver pl, const char *dest) {
    int error = 0;
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
    pl->dest = strdup_or_error(dest, & error); /* safety: make our own copy of dest */
    return error;
    
fail:
    return -1;
}

pl_driver mpris2_new(const char *dest) {
    mpris2_driver pl = (mpris2_driver)calloc(1, sizeof(struct mpris2_driver_s));
    if (!pl)
        return NULL;

    if (ddriver_init(pl, dest) != 0)
        goto fail;
    
    pl->base = mpris2_funcs; /* struct copy */
    pl->path = "/org/mpris/MediaPlayer2";
    
    return & pl->base;
    
fail:
    mpris2_del(& pl->base);
    return NULL;
}

/***************************** factory *******************************/

static void dparse_reply_factories(DBusMessage *reply, pl_factories collection) {
    DBusMessageIter iter1, iter2;
    const char * dest;
    
    if (reply == NULL || dbus_message_get_type(reply) != DBUS_MESSAGE_TYPE_METHOD_RETURN)
        return;
    
    dbus_message_iter_init (reply, &iter1);
    if (dbus_message_iter_get_arg_type(&iter1) != DBUS_TYPE_ARRAY)
        return;

    dbus_message_iter_recurse (&iter1, &iter2);
    while (dbus_message_iter_get_arg_type(&iter2) == DBUS_TYPE_STRING) {
        dest = NULL;
        dbus_message_iter_get_basic (&iter2, &dest);
        
        if (dest && !strncmp("org.mpris.MediaPlayer2.", dest, 23))
            append_new_factory(collection, dest + 23, dest, mpris2_new);
        
        dbus_message_iter_next (&iter2);
    }
}

static struct mpris2_driver_s pl_scanner;

static int mpris2_init_scanner(void) {
    pl_scanner.path = "/org/freedesktop/DBus";
    return ddriver_init(&pl_scanner, "org.freedesktop.DBus");
}

void mpris2_list_factories(pl_factories collection) {
    if (!pl_scanner.connection && mpris2_init_scanner() != 0)
        return;

    DBusMessage *message = dcall_prepare(&pl_scanner, "org.freedesktop.DBus", "ListNames");
    if (message != NULL) {
        DBusMessage *reply = dcall_exec_get_reply(&pl_scanner, message);

        dparse_reply_factories(reply, collection);
        
        dcall_unref(message, reply);
    }
}
