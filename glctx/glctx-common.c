#include "glctx.h"

const int glctx_version_major = GLCTX_VERSION_MAJOR;
const int glctx_version_minor = GLCTX_VERSION_MINOR;

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
        case GLCTX_ERROR_MEMORY :
            return "GLCTX_ERROR_MEMORY";
        case GLCTX_ERROR_DISPLAY:
            return "GLCTX_ERROR_DISPLAY";
        case GLCTX_ERROR_CONFIG :
            return "GLCTX_ERROR_CONFIG";
        case GLCTX_ERROR_WINDOW :
            return "GLCTX_ERROR_WINDOW";
        case GLCTX_ERROR_SURFACE:
            return "GLCTX_ERROR_SURFACE";
        case GLCTX_ERROR_CONTEXT:
            return "GLCTX_ERROR_CONTEXT";
        case GLCTX_ERROR_BIND   :
            return "GLCTX_ERROR_BIND";
        case GLCTX_ERROR_API     :
            return "GLCTX_ERROR_API";
        default:
            break;
    }
    return "GLCTX_ERROR_UNKNOWN";
}
