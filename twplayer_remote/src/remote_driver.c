#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "remote_driver.h"

static void out_of_memory(void) {
    fputs("Out of memory!\n", stderr);
    fflush(stderr);
}

char * strdup_or_error(const char * str, int * error) {
    char * copy = NULL;
    if (str && !(copy = strdup(str))) {
        out_of_memory();
        if (error)
            *error = -1;
    }
    return copy;
}

void init_driver(pl_driver pl, const char * label)
{
    pl->label = strdup_or_error(label, NULL);
}

void clear_driver(pl_driver pl) {
    free(pl->label);
    pl->label = NULL;
}

pl_factory new_factory(void)
{
    pl_factory element = (pl_factory) calloc(1, sizeof(struct pl_factory_s)); 
    if (!element)
        out_of_memory();
    return element;
}

pl_driver call_factory(pl_factory element)
{
    pl_driver pl = NULL;
    if (element && element->constructor) {
        pl = element->constructor(element->constructor_arg);
        init_driver(pl, element->label);
    }
    return pl;
}

void del_factory(pl_factory element)
{
    free(element->label);
    free(element->constructor_arg);
    element->label = element->constructor_arg = NULL;
    free(element);
}

pl_factories new_factories(void)
{
    pl_factories collection = (pl_factories) calloc(1, sizeof(struct pl_factories_s));
    if (!collection)
        out_of_memory();
    return collection;
}
    
static int realloc_factories(pl_factories collection)
{
    enum { sizeof_el = sizeof(pl_factory), max_cap = UINT_MAX/sizeof_el, };
    
    unsigned old_cap = collection->capacity, new_cap = (old_cap < 10 ? 10 : old_cap < max_cap/2 ? old_cap * 2 : max_cap);
    pl_factory * data = (pl_factory *) realloc(collection->data, new_cap*sizeof_el);
    if (!data) {
        out_of_memory();
        return -1;
    }
    collection->data = data;
    collection->capacity = new_cap;
    return 0;
}

void append_new_factory(pl_factories collection, const char * label, const char * constructor_arg, pl_driver_constructor constructor)
{
    if (collection->size < collection->capacity || realloc_factories(collection) == 0) {
        pl_factory element = new_factory();
        if (element) {
            int error = 0;
            element->label = strdup_or_error(label, &error);
            element->constructor_arg = strdup_or_error(constructor_arg, &error);
            element->constructor = constructor;
            if (error)
                del_factory(element);
            else
                collection->data[collection->size++] = element; /* pointer copy */
        }
    }
}

void clear_factories(pl_factories collection)
{
    unsigned i, n = collection->size;
    collection->size = 0;
    for (i = 0; i < n; i++) {
        del_factory(collection->data[i]);
        collection->data[i] = NULL;
    }
}

void del_factories(pl_factories collection)
{
    clear_factories(collection);
    collection->capacity = 0;
    free(collection->data);
    collection->data = NULL;
    free(collection);
}

