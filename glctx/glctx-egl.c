#include "glctx.h"

#include "EGL/egl.h"

#include <stdlib.h>

#ifdef __ANDROID__
#include <android/native_window.h>
#elif GLCTX_ENABLE_RPI
#include "bcm_host.h"
extern int glGetError(void);
#endif

extern int (*glctx__log)(const char *format, ...);
extern int glctx__log_ignore(const char *format, ...);
extern int *glctx__make_attrs_buffer(const int *attrs,
        const int *native_attrs, int native_attr_term);

static int glctx__attr_table[] = {
    EGL_NONE,
    EGL_RED_SIZE,
    EGL_GREEN_SIZE,
    EGL_BLUE_SIZE,
    EGL_ALPHA_SIZE,
    EGL_DEPTH_SIZE,
    EGL_STENCIL_SIZE
};

struct GlctxData_ {
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    GlctxWindow window;
    GlctxProfile profile;
    int version;
#if GLCTX_ENABLE_RPI
    EGL_DISPMANX_WINDOW_T nativewindow;
#endif
};

GlctxError glctx_init(GlctxDisplay display, GlctxWindow window,
                      GlctxProfile profile, int maj_version, int min_version,
                      GlctxHandle *pctx)
{
    GlctxHandle ctx = malloc(sizeof(struct GlctxData_));
    EGLint emaj, emin;

    *pctx = NULL;
    if (!ctx)
        return GLCTX_ERROR_MEMORY;
#if GLCTX_ENABLE_RPI
    (void) display;
    ctx->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#else
    ctx->display = eglGetDisplay(display);
#endif
    ctx->surface = EGL_NO_SURFACE;
    ctx->context = EGL_NO_CONTEXT;
    if (ctx->display == EGL_NO_DISPLAY ||
            !eglInitialize(ctx->display, &emaj, &emin))
    {
        free(ctx);
        return GLCTX_ERROR_DISPLAY;
    }
    *pctx = ctx;
    ctx->window = window;
    ctx->profile = profile;
    ctx->version = maj_version;
    (void) min_version;
    glctx__log("glctx: Initialised display with EGL %d.%d\n", emaj, emin);
    return GLCTX_ERROR_NONE;
}

GlctxError glctx_get_config(GlctxHandle ctx, GlctxConfig *cfg_out,
        const int *attrs, int native_attrs)
{
    int eprofile = (ctx->profile == GLCTX_PROFILE_OPENGLES) ?
                ((ctx->version > 1) ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT) :
                EGL_OPENGL_BIT;
    EGLint default_attrs[] = {
        EGL_RENDERABLE_TYPE, eprofile,
        EGL_CONFORMANT, eprofile,
        EGL_NONE
    };
    EGLint n_configs = 0;
    int *all_attrs;
    int i = 0;
    int n;

    if (native_attrs)
    {
        all_attrs = (int *) default_attrs;
    }
    else
    {
        all_attrs = glctx__make_attrs_buffer(attrs, default_attrs, EGL_NONE);
        i = 0;
        if (attrs)
        {
            for (n = 0; attrs[n]; n += 2)
            {
                all_attrs[i++] = glctx__attr_table[attrs[n]];
                all_attrs[i++] = attrs[n + 1];
            }
        }
        for (n = 0; default_attrs[n] != EGL_NONE; n += 2)
        {
            all_attrs[i++] = default_attrs[n];
            all_attrs[i++] = default_attrs[n + 1];
        }
        all_attrs[i] = EGL_NONE;
    }

    if (glctx__log != glctx__log_ignore)
    {
        int n;
        int chosen = 0;
        int result = eglChooseConfig(ctx->display, all_attrs, 0, 0, &n_configs);

        if (!result || n_configs < 1)
        {
            free(all_attrs);
            glctx__log("glctx: No EGL configs available:\n");
            return GLCTX_ERROR_CONFIG;
        }
        glctx__log("glctx: %d EGL configs available:\n", n_configs);
        EGLConfig *configs = malloc(sizeof(EGLConfig) * n_configs);
        eglChooseConfig(ctx->display, all_attrs,
                configs, n_configs, &n_configs);
        if (!native_attrs)
            free(all_attrs);

        for (n = 0; n < n_configs; ++n)
        {
            EGLint r, g, b, a, d;

            if (!configs[n])
            {
                glctx__log("  Config %d is NULL!\n", n);
                if (n == chosen)
                    ++chosen;
                continue;
            }
            eglGetConfigAttrib(ctx->display, configs[n], EGL_RED_SIZE, &r);
            eglGetConfigAttrib(ctx->display, configs[n], EGL_GREEN_SIZE, &g);
            eglGetConfigAttrib(ctx->display, configs[n], EGL_BLUE_SIZE, &b);
            eglGetConfigAttrib(ctx->display, configs[n], EGL_ALPHA_SIZE, &a);
            eglGetConfigAttrib(ctx->display, configs[n], EGL_DEPTH_SIZE, &d);
            glctx__log("  %d, RGBA(%d%d%d%d), depth %d\n",
                    n, r, g, b, a, d);
        }
        if (chosen >= n_configs)
        {
            glctx__log("glctx: No EGL configs available:\n");
            free(configs);
            return GLCTX_ERROR_CONFIG;
        }
        *cfg_out = configs[chosen];
        free(configs);
    }
    else
    {
        int result = eglChooseConfig(ctx->display, all_attrs,
                cfg_out, 1, &n_configs);
        free(all_attrs);
        if (!result || n_configs < 1)
        {
            return GLCTX_ERROR_CONFIG;
        }
    }
    return GLCTX_ERROR_NONE;
}

int glctx_query_config(GlctxHandle ctx, GlctxConfig config, GlctxAttr attr)
{
    EGLint val;

    eglGetConfigAttrib(ctx->display, config, glctx__attr_table[attr], &val);
    return val;
}

#if defined(__ANDROID__)
static GlctxError glctx_configure_platform(GlctxHandle ctx, GlctxConfig config)
{
    EGLint format;

    if (!eglGetConfigAttrib(ctx->display, config,
            EGL_NATIVE_VISUAL_ID, &format))
    {
        glctx__log("glctx: Unable to configure Android window\n");
        return GLCTX_ERROR_WINDOW;
    }
    ANativeWindow_setBuffersGeometry(ctx->window, 0, 0, format);
    return GLCTX_ERROR_NONE;
}
#elif GLCTX_ENABLE_RPI
static GlctxError glctx_configure_platform(GlctxHandle ctx, GlctxConfig config)
{
    DISPMANX_ELEMENT_HANDLE_T dispman_element;
    DISPMANX_DISPLAY_HANDLE_T dispman_display;
    DISPMANX_UPDATE_HANDLE_T dispman_update;
    VC_RECT_T dst_rect;
    VC_RECT_T src_rect;
    uint32_t disp_w, disp_h;

    (void) config;

    if (graphics_get_display_size(0 /* LCD */, &disp_w, &disp_h) < 0)
    {
        glctx__log("glctx: Unable to get RPi screen size\n");
        return GLCTX_ERROR_WINDOW;
    }
    dst_rect.x = dst_rect.y = 0;
    dst_rect.width = disp_w;
    dst_rect.height = disp_h;
    src_rect.x = src_rect.y = 0;
    src_rect.width = dst_rect.width << 16;
    src_rect.height = dst_rect.height << 16;
    dispman_display = vc_dispmanx_display_open(0 /* LCD */);
    dispman_update = vc_dispmanx_update_start(0);
    dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display,
            0 /* layer */, &dst_rect,
            0 /* src */, &src_rect,
            DISPMANX_PROTECTION_NONE,
            0 /* alpha */, 0 /* clamp */, (DISPMANX_TRANSFORM_T) 0);
    ctx->nativewindow.element = dispman_element;
    ctx->nativewindow.width = disp_w;
    ctx->nativewindow.height = disp_h;
    vc_dispmanx_update_submit_sync(dispman_update);
    if (glGetError())
    {
        glctx__log("glctx: Unable to set up vc display manager\n");
    }
    return GLCTX_ERROR_NONE;
}
#else
static GlctxError glctx_configure_platform(GlctxHandle ctx, GlctxConfig config)
{
    (void) ctx;
    (void) config;
    return GLCTX_ERROR_NONE;
}
#endif

GlctxError glctx_activate(GlctxHandle ctx, GlctxConfig config,
        GlctxWindow window, const int *attrs)
{
    EGLenum eapi;
    EGLint default_attrs[] = {
            EGL_CONTEXT_CLIENT_VERSION, ctx->version,
            EGL_NONE
    };
    GlctxError result = GLCTX_ERROR_NONE;

    ctx->window = window;
    if (!attrs)
        attrs = default_attrs;
    if (ctx->profile == GLCTX_PROFILE_OPENGLES)
        eapi = EGL_OPENGL_ES_API;
    else
        eapi = EGL_OPENGL_API;
    if (!eglBindAPI(eapi))
        return GLCTX_ERROR_PROFILE;

    result = glctx_configure_platform(ctx, config);
    if (result)
        return result;

    ctx->surface = eglCreateWindowSurface(ctx->display, config,
#if GLCTX_ENABLE_RPI
            &ctx->nativewindow,
#else
            window,
#endif
            0);
    if (ctx->surface == EGL_NO_SURFACE)
    {
        glctx__log("glctx: Unable to create OpenGL(ES) surface with EGL\n");
        return GLCTX_ERROR_SURFACE;
    }

    ctx->context = eglCreateContext(ctx->display, config,
            EGL_NO_CONTEXT, attrs);
    if (ctx->context == EGL_NO_CONTEXT)
    {
        glctx__log("glctx: Unable to create OpenGL(ES) context with EGL\n");
        return GLCTX_ERROR_CONTEXT;
    }

    return glctx_bind(ctx);
}

#if 0
int glctx_get_width(GlctxHandle ctx)
{
    EGLint w;
    if (!eglQuerySurface(ctx->display, ctx->surface, EGL_WIDTH, &w))
    {
        glctx__log("glctx: Unable to get width of EGL surface\n");
        return -1;
    }
    return w;
}

int glctx_get_height(GlctxHandle ctx)
{
    EGLint h;
    if (!eglQuerySurface(ctx->display, ctx->surface, EGL_HEIGHT, &h))
    {
        glctx__log("glctx: Unable to get height of EGL surface\n");
        return -1;
    }
    return h;
}
#endif

GlctxNativeContext glctx_get_native_context(GlctxHandle ctx)
{
    return ctx->context;
}

EGLDisplay glctx_get_egl_display(GlctxHandle ctx)
{
    return ctx->display;
}

EGLSurface glctx_get_egl_surface(GlctxHandle ctx)
{
    return ctx->surface;
}

void glctx_flip(GlctxHandle ctx)
{
    eglSwapBuffers(ctx->display, ctx->surface);
}

GlctxError glctx_unbind(GlctxHandle ctx)
{
    if (!eglMakeCurrent(ctx->display,
            EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
    {
        glctx__log("glctx: Unable to unbind thread from OpenGL(ES)");
        return GLCTX_ERROR_BIND;
    }
    return GLCTX_ERROR_NONE;
}

GlctxError glctx_bind(GlctxHandle ctx)
{
    if (!eglMakeCurrent(ctx->display,
            ctx->surface, ctx->surface, ctx->context))
    {
        glctx__log("glctx: Unable to bind thread to OpenGL(ES)");
        return GLCTX_ERROR_BIND;
    }
    return GLCTX_ERROR_NONE;
}

void glctx_terminate(GlctxHandle ctx)
{
    if (ctx->display != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(ctx->display,
                EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (ctx->context != EGL_NO_CONTEXT)
            eglDestroyContext(ctx->display, ctx->context);
        if (ctx->surface != EGL_NO_SURFACE)
        {
            eglDestroySurface(ctx->display, ctx->surface);
            ctx->surface = EGL_NO_SURFACE;
        }
        eglTerminate(ctx->display);
    }
    free(ctx);
}
