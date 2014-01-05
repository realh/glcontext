#ifndef GLCTX_H
#define GLCTX_H

#include "glctx-config.h"
#include "glctx_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GlctxDisplay
 * Platform's native display type
 */

/*
 * GlctxWindow
 * Platform's native window type
 */

/*
 * GlctxConfig
 * Platform's native config type
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

#if GLCTX_ENABLE_EGL
#include <EGL/egl.h>
typedef EGLConfig GlctxConfig;
typedef EGLContext GlctxNativeContext;
#elif GLCTX_ENABLE_GLX
#include <GL/glx.h>
typedef GLXFBConfig GlctxConfig;
typedef GLXContext GlctxNativeContext;
#elif GLCTX_ENABLE_WGL
typedef int GlctxConfig;
typedef HGLRC GlctxNativeContext;
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
    GLCTX_ERROR_PROFILE     /* Unable to bind profile rendering type */
} GlctxError;


/*
 * GlctxAttr
 * Attribute codes of a config.
 */
typedef enum {
    GLCTX_CFG_NONE,
    GLCTX_CFG_RED_SIZE,
    GLCTX_CFG_GREEN_SIZE,
    GLCTX_CFG_BLUE_SIZE,
    GLCTX_CFG_ALPHA_SIZE,
    GLCTX_CFG_DEPTH_SIZE,
    GLCTX_CFG_STENCIL_SIZE
} GlctxAttr;

/*
 * GlctxProfile
 * OpenGL/ES profile/compatibility type
 */
typedef enum {
    GLCTX_PROFILE_NONE,         /* Not really useful */
    GLCTX_PROFILE_OPENGLES,     /* OpenGL ES */
    GLCTX_PROFILE_OPENGL,       /* Don't care about core/compat */
    GLCTX_PROFILE_CORE,         /* OpenGL */
    GLCTX_PROFILE_COMPAT        /* OpenGL */
} GlctxProfile;

/*
 * GlctxLogFunction
 * Prototype of a function to log messages from glcontext.
 * Logging functions take printf-style arguments and the messages
 * will include trailing newlines. The return value is ignored but
 * inlcuded in the prototype so you can use printf without a wrapper.
 */
typedef int (*GlctxLogFunction)(const char *format, ...);

/*
 * glctx_set_log_function
 * Provide a logging function. If NULL, logging is disabled (the default).
 * This may be called at any time, but is usually called before any other
 * function.
 */
void GLCTX_EXPORT glctx_set_log_function(GlctxLogFunction logger);

/*
 * GlctxHandle
 * Pointer to an opaque structure to maintain context for glcontext.
 */
typedef struct GlctxData_ *GlctxHandle;

/*
 * glctx_get_error_name
 * Returns a string representation of the enum value
 */
const char GLCTX_EXPORT *glctx_get_error_name(GlctxError err);

/*
 * glctx_init
 * Initialise glcontext. If you are adding OpenGL to an existing window you
 * can pass in its handle here, which may help GLX get the right screen,
 * otherwise 0/NULL.
 *
 * display:     Native display
 * window:      Native window or 0/NULL
 * profile:     One of the GLCTX_CTX_PROFILE_ constants
 * maj_version: Desired OpenGL major version
 * min_version: Desired OpenGL minor version
 * pctx:        glcontext handle (out)
 */
GlctxError GLCTX_EXPORT glctx_init(GlctxDisplay display, GlctxWindow window,
        GlctxProfile profile, int maj_version, int min_version,
        GlctxHandle *pctx);

/*
 * glctx_get_config
 * Get best matching GL config
 *
 * ctx:             The context to shut down
 * cfg_out:         A config is returned here
 * attrs:           List of pairs of (GlctxAttr, value), terminated by
 *                  GLCTX_CFG_NONE.
 * native_attrs:    If non-zero this indicates that the attributes use codes
 *                  defined by the underlying implementation (EGL or GLX). Has
 *                  no effect with WGL. For experts only. Bear in mind the
 *                  terminator may not be 0 (for instance EGL_NONE seems to be
 *                  NZ on Raspberry Pi).
 */
GlctxError GLCTX_EXPORT glctx_get_config(GlctxHandle ctx, GlctxConfig *cfg_out,
        const int *attrs, int native_attrs);

/*
 * glctx_query_config
 * Get a config attribute
 */
int GLCTX_EXPORT glctx_query_config(GlctxHandle ctx, GlctxConfig config,
        GlctxAttr attr);

/*
 * glctx_activate
 * Activate a context with a given config in a given window. On Windows and
 * Linux (excluding RPi) it's your responsibility to make sure the window
 * matches the config.
 *
 * attrs:       List of pairs of (native_context_attr, value), terminated by
 *              implementation's NONE; non-NULL suppresses default, usually
 *              used for profile. (Experts only).
 */
GlctxError GLCTX_EXPORT glctx_activate(GlctxHandle ctx, GlctxConfig config,
        GlctxWindow window, const int *attrs);

/*
 * glctx_get_native_context
 * Gets the underlying EGL, GLX or WGL context
 */
GlctxNativeContext GLCTX_EXPORT glctx_get_native_context(GlctxHandle ctx);

#if GLCTX_ENABLE_EGL
EGLDisplay GLCTX_EXPORT glctx_get_egl_display(GlctxHandle ctx);

EGLSurface GLCTX_EXPORT glctx_get_egl_surface(GlctxHandle ctx);
#endif

/*
 * glctx_flip
 * Flips buffers to display a rendered scene
 */
void GLCTX_EXPORT glctx_flip(GlctxHandle ctx);

/*
 * glctx_unbind
 * Unbinds context from current thread
 */
GlctxError GLCTX_EXPORT glctx_unbind(GlctxHandle ctx);

/*
 * glctx_bind
 * Binds context to current thread (only to be called after glctx_unbind)
 */
GlctxError GLCTX_EXPORT glctx_bind(GlctxHandle ctx);

/*
 * glctx_terminate
 * Shut down a GL context
 * ctx:         The context to shut down, will be invalid afterwards
 */
void GLCTX_EXPORT glctx_terminate(GlctxHandle ctx);

#ifdef __cplusplus
}
#endif

#endif // GLCTX_H
