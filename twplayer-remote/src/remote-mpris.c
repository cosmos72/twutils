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

typedef struct dbus_arg_s * dbus_arg;

struct dbus_arg_s {
    int type;
    union {
        double d;
        dbus_int64_t i64;
        dbus_uint64_t u64;
        char * str;
    };
};

void dbus_arg_unref(dbus_arg arg) {
   if (arg != NULL && arg->type == DBUS_TYPE_STRING)
   {
       free(arg->str);
       arg->str = NULL;
   }
}

static void parse_iter(DBusMessageIter *iter, dbus_arg result) {
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
	        parse_iter(&subiter, result);
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

static void parse_message(DBusMessage *message, dbus_arg result) {
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
	   parse_iter(&iter, result); 
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

static double to_double(dbus_arg arg)
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

static double to_int64(dbus_arg arg)
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

static void dbus_call(pl_driver pl_base, dbus_arg result, const char * interface, const char * method, ...) {
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
    if (result)
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
    if (result)
        parse_message(reply, result);
cleanup:
    if (reply)
        dbus_message_unref(reply); 
    if (message)
        dbus_message_unref(message); 
}

static inline void mpris_driver_get_property(pl_driver pl, dbus_arg arg, const char * property_name) {
    dbus_call(pl, arg, props_interface, "Get", DBUS_TYPE_STRING, & mpris_interface, DBUS_TYPE_STRING, & property_name, DBUS_TYPE_INVALID);
}

static inline void mpris_driver_set_property(pl_driver pl, const char * property_name, int property_type, const void * property_value) {
    dbus_call(pl, NULL, props_interface, "Set", DBUS_TYPE_STRING, & mpris_interface, DBUS_TYPE_STRING, & property_name, DBUS_TYPE_VARIANT, property_type, property_value, DBUS_TYPE_INVALID);
}

static void mpris_driver_play(pl_driver pl) {
    dbus_call(pl, NULL, NULL, "Play", DBUS_TYPE_INVALID);
}

static void mpris_driver_pause(pl_driver pl) {
    dbus_call(pl, NULL, NULL, "Pause", DBUS_TYPE_INVALID);
}

static void mpris_driver_stop(pl_driver pl) {
    dbus_call(pl, NULL, NULL, "Stop", DBUS_TYPE_INVALID);
}

static void mpris_driver_next(pl_driver pl) {
    dbus_call(pl, NULL, NULL, "Next", DBUS_TYPE_INVALID);
}

static void mpris_driver_prev(pl_driver pl) {
    dbus_call(pl, NULL, NULL, "Previous", DBUS_TYPE_INVALID);
}

static int64_t mpris_driver_position(pl_driver pl) {
    struct dbus_arg_s arg;
    mpris_driver_get_property(pl, &arg, "Position");
    dbus_arg_unref(&arg);
    return to_int64(&arg) / 1000;
}

static void mpris_driver_seek(pl_driver pl, int64_t offset) {
    offset *= 1000;
    dbus_call(pl, NULL, NULL, "Seek", DBUS_TYPE_INT64, &offset, DBUS_TYPE_INVALID);
}

static int64_t mpris_driver_volume(pl_driver pl) {
    struct dbus_arg_s arg;
    mpris_driver_get_property(pl, &arg, "Volume");
    dbus_arg_unref(&arg);
    return (int64_t)(to_double(&arg) * 100.0 + 0.5);
}

static void mpris_driver_set_volume(pl_driver pl, int64_t volume) {
    double vol_double = volume / 100.0;
    mpris_driver_set_property(pl, "Volume", DBUS_TYPE_DOUBLE, &vol_double);
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
