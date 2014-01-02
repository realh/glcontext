#ifndef GLCTX_H
#define GLCTX_H

#if defined __ANDROID__
#include "glctx-config-android.h"
#else
#include <glctx/glctx-config.h>
#endif

/*
 * GlctxDisplay
 * Platform's native display type
 */

/*
 * GlctxWindow
 * Platform's native window type
 */

#if defined __ANDROID__
typedef void *GlctxDisplay;
typedef struct ANativeWindow *GlctxWindow;
#elif GLCTX_X11
#include <X11/Xlib.h>
typedef Display *GlctxDisplay;
typedef Window GlctxWindow;
#elif GLCTX_MSWIN
#include <windows.h>
typedef HDC GlctxDisplay;
typedef HWND GlctxWindow;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GlctxError
 * glcontext error codes
 */
typedef enum {
    GLCTX_ERROR_NONE,
    GLCTX_ERROR_MEMORY,     /* Unable to allocate memory */
    GLCTX_ERROR_DISPLAY,    /* Error initialising/configuring display */
    GLCTX_ERROR_CONFIG,     /* Unable to get a suitable GL config */
    GLCTX_ERROR_WINDOW,     /* Unable to configure window */
    GLCTX_ERROR_SURFACE,    /* Unable to set up GL surface */
    GLCTX_ERROR_CONTEXT,    /* Unable to create OpenGL context */
    GLCTX_ERROR_BIND,       /* Unable to bind context to current thread */
    GLCTX_ERROR_API         /* Unable to bind API type */
} GlctxError;

/*
 * GlctxApiType
 * Target API, OpenGL or OpenGL ES. DEFAULT is not recommended because you
 * have to decide between them at compile time to choose which headers to
 * include (eg GLES2/gl2.h vs using GLEW or similar).
 */
typedef enum {
    GLCTX_API_DEFAULT,
    GLCTX_API_OPENGL,
    GLCTX_API_OPENGLES
} GlctxApiType;

/*
 * GlctxLogFunction
 * Prototype of a function to log messages from glcontext.
 * Logging functions take printf-style arguments and the messages
 * will include trailing newlines. The return value is ignored but
 * inlcuded in the prototype so you can use printf without a wrapper.
 */
typedef int (*GlctxLogFunction)(const char *format, ...);

/*
 * glctx_version_major
 */
extern const int glctx_version_major;

/*
 * glctx_version_minor
 */
extern const int glctx_version_minor;

/*
 * glctx_set_log_function
 * Provide a logging function. If NULL, logging is disabled (the default).
 * This may be called at any time, but is usually called before any other
 * function.
 */
void glctx_set_log_function(GlctxLogFunction logger);

/*
 * GlctxHandle
 * Pointer to an opaque structure to maintain context for glcontext.
 */
typedef struct GlctxData_ *GlctxHandle;

/*
 * glctx_get_error_name
 * Returns a string representation of the enum value
 */
const char *glctx_get_error_name(GlctxError err);

/*
 * glctx_init
 * Initialise glcontext.
 *
 * display:     Native display
 * window:      Native window
 * api:         API type, see comment for typedef
 * version_maj: OpenGL(ES) major version
 * version_min: OpenGL(ES) minor version
 * pctx:        glcontext handle (out)
 */
GlctxError glctx_init(GlctxDisplay display, GlctxWindow window,
                      GlctxApiType api, int version_maj, int version_min,
                      GlctxHandle *pctx);

/*
 * glctx_query_api_type
 * Gets the current API type. If it returns GLCTX_API_DEFAULT
 * this is indicative of an error.
 */
GlctxApiType glctx_query_api_type(GlctxHandle ctx);

/*
 * glctx_get_config
 * Get best matching GL config
 *
 * ctx:         The context to shut down
 * r, g, b, a:  Size in bits of each component of a pixel
 * depth:       Size in bits of each word in depth buffer
 */
GlctxError glctx_get_config(GlctxHandle ctx,
                            int r, int g, int b, int a, int depth);

/*
 * glctx_describe_config
 * Describe last chosen config. Params other than ctx may be NULL to ignore
 *
 * ctx:         The context to shut down
 * r, g, b, a:  Size in bits of each component of a pixel (out)
 * depth:       Size in bits of each word in depth buffer (out)
 */
void glctx_describe_config(GlctxHandle ctx,
                           int *r, int *g, int *b, int *a, int *depth);

/*
 * glctx_activate
 * Activate a context after a config has been chosen successfully
 */
GlctxError glctx_activate(GlctxHandle ctx);

/*
 * glctx_get_width
 * Gets the width of the surface in pixels
 */
int glctx_get_width(GlctxHandle ctx);

/*
 * glctx_get_height
 * Gets the height of the surface in pixels
 */
int glctx_get_height(GlctxHandle ctx);

/*
 * glctx_flip
 * Flips buffers to display a rendered scene
 */
void glctx_flip(GlctxHandle ctx);

/*
 * glctx_unbind
 * Unbinds context from current thread
 */
GlctxError glctx_unbind(GlctxHandle ctx);

/*
 * glctx_bind
 * Binds context to current thread (only to be called after glctx_unbind)
 */
GlctxError glctx_bind(GlctxHandle ctx);

/*
 * glctx_terminate
 * Shut down a GL context
 * ctx:         The context to shut down, will be invalid afterwards
 */
void glctx_terminate(GlctxHandle ctx);

#ifdef __cplusplus
}
#endif

#endif // GLCTX_H
