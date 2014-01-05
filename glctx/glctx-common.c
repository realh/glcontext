#include "glctx.h"

#include <stdlib.h>

int glctx__log_ignore(const char *format, ...)
{
    (void) format;
    return 0;
}

int (*glctx__log)(const char *format, ...) = glctx__log_ignore;

void glctx_set_log_function(GlctxLogFunction logger)
{
    if (!logger)
        logger = glctx__log_ignore;
    glctx__log = logger;
}

const char *glctx_get_error_name(GlctxError err)
{
    switch (err)
    {
        case GLCTX_ERROR_NONE:
            return "GLCTX_ERROR_NONE";
        case GLCTX_ERROR_MEMORY:
            return "GLCTX_ERROR_MEMORY";
        case GLCTX_ERROR_DISPLAY:
            return "GLCTX_ERROR_DISPLAY";
        case GLCTX_ERROR_CONFIG:
            return "GLCTX_ERROR_CONFIG";
        case GLCTX_ERROR_WINDOW:
            return "GLCTX_ERROR_WINDOW";
        case GLCTX_ERROR_SURFACE:
            return "GLCTX_ERROR_SURFACE";
        case GLCTX_ERROR_CONTEXT:
            return "GLCTX_ERROR_CONTEXT";
        case GLCTX_ERROR_BIND:
            return "GLCTX_ERROR_BIND";
        case GLCTX_ERROR_PROFILE:
            return "GLCTX_ERROR_PROFILE";
        default:
            break;
    }
    return "GLCTX_ERROR_UNKNOWN";
}

int *glctx__make_attrs_buffer(const int *attrs,
        const int *native_attrs, int native_attr_term)
{
    int n_attrs = 0;
    int n;

    if (attrs)
    {
        for (n = 0; attrs[n]; n += 2)
            ++n_attrs;
    }
    if (native_attrs)
    {
        for (n = 0; native_attrs[n] != native_attr_term; n += 2)
            ++n_attrs;
    }
    n_attrs = n_attrs * 2 + 1;
    return malloc(sizeof(int) * n_attrs);
}

