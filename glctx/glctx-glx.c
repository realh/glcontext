#include "glctx.h"

#include "GL/glx.h"

#include <stdlib.h>
#include <string.h>

extern int (*glctx__log)(const char *format, ...);
extern int glctx__log_ignore(const char *format, ...);
extern int *glctx__make_attrs_buffer(const int *attrs, const int *native_attrs);

struct GlctxData_ {
    Display *dpy;
    Window window;
    int screen;
    int width, height;
    GLXContext ctx;
};

static void glctx_bind_xwindow(GlctxHandle ctx, Window window)
{
    if (window)
    {
        XWindowAttributes attribs;
        if (!XGetWindowAttributes(ctx->dpy, window, &attribs))
        {
            glctx__log("glctx: Unable to get window attributes\n");
            window = 0;
        }
        ctx->screen = XScreenNumberOfScreen(attribs.screen);
        ctx->width = attribs.width;
        ctx->height = attribs.height;
    }
    ctx->window = window;
    if (!window)
    {
        ctx->screen = XScreenNumberOfScreen(XDefaultScreenOfDisplay(ctx->dpy));
        ctx->width = ctx->height = 0;
    }
}

GlctxError glctx_init(GlctxDisplay display, GlctxWindow window,
                      GlctxHandle *pctx)
{
    GlctxHandle ctx = malloc(sizeof(struct GlctxData_));

    *pctx = NULL;
    if (!ctx)
        return GLCTX_ERROR_MEMORY;
    ctx->dpy = display;
    glctx_bind_xwindow(ctx, window);
    *pctx = ctx;
    return GLCTX_ERROR_NONE;
}

GlctxError glctx_get_config(GlctxHandle ctx, GlctxConfig *cfg_out,
        const int *attrs, int suppress_defaults)
{
    int fbc_count;
    GLXFBConfig* fbc;
    int i, n;
    int best_nsamples = -1;
    int *all_attrs;
    static const int default_native_attrs[] = {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        /*
        GLX_STENCIL_SIZE    , 8,
        GLX_SAMPLE_BUFFERS  , 1,
        GLX_SAMPLES         , 4,
        */
        None
    };
    const int *native_attrs = suppress_defaults ? NULL : default_native_attrs;

    all_attrs = glctx__make_attrs_buffer(attrs, native_attrs);
    if (attrs)
    {
        for (n = 0; attrs[n]; n += 2)
        {
            all_attrs[i++] = attrs[n];
            all_attrs[i++] = attrs[n + 1];
        }
    }
    if (native_attrs)
    {
        for (n = 0; native_attrs[n]; n += 2)
        {
            all_attrs[i++] = native_attrs[n];
            all_attrs[i++] = native_attrs[n + 1];
        }
    }
    all_attrs[i] = 0;

    fbc = glXChooseFBConfig(ctx->dpy, ctx->screen, all_attrs, &fbc_count);
    free(all_attrs);
    if (!fbc)
    {
        glctx__log("glctx: Unable to get any matching GLX configs\n");
        return GLCTX_ERROR_CONFIG;
    }

    for (i = 0; i < fbc_count; ++i)
    {
        XVisualInfo *vi = glXGetVisualFromFBConfig(ctx->dpy, fbc[i]);
        if (vi)
        {
            int samp_buf, nsamples;

            glXGetFBConfigAttrib(ctx->dpy, fbc[i],
                    GLX_SAMPLE_BUFFERS, &samp_buf);
            glXGetFBConfigAttrib(ctx->dpy, fbc[i],
                    GLX_SAMPLES, &nsamples);

            glctx__log("glctx: Matching fbconfig %d, visual ID 0x%2x: "
                    "SAMPLE_BUFFERS = %d, SAMPLES = %d\n",
                    i, vi -> visualid, samp_buf, nsamples );

            if (best_nsamples < 0 || (samp_buf && nsamples > best_nsamples))
            {
                *cfg_out = fbc[i];
                best_nsamples = nsamples;
            }
        }
        XFree(vi);
    }

    /* GLXFBConfig is a pointer type, so it's safe to free the list of configs
     * now although we still need one of the configs from the list
     */
    XFree(fbc);

    return GLCTX_ERROR_NONE;
}

int glctx_query_config(GlctxHandle ctx, GlctxConfig config, int attr)
{
    int val;

    glXGetFBConfigAttrib(ctx->dpy, config, attr, &val);
    return val;
}

static int glctx_glx_supports_extension(const char *extensions, const char *ext)
{
    const char *start;
    const char *where, *term;

    for (start = extensions;;)
    {
        where = strstr(start, ext);
        if (!where)
            break;
        term = where + strlen(ext);
        if ((where == start || *(where - 1) == ' ') &&
                (*term == ' ' || !*term))
        {
            return 1;
        }
        start = term;
    }
    return 0;
}

typedef GLXContext (*glXCreateContextAttribsARBProc)
        (Display*, GLXFBConfig, GLXContext, Bool, const int *);
static glXCreateContextAttribsARBProc glXCreateContextAttribsARB = NULL;
static const char *glctx_glx_extensions = NULL;

GlctxError glctx_activate(GlctxHandle ctx, GlctxConfig config,
        GlctxWindow window, const int *attrs)
{
    glctx_bind_xwindow(ctx, window);
    if (!glctx_glx_extensions)
    {
        glctx_glx_extensions = glXQueryExtensionsString(ctx->dpy,
            DefaultScreen(ctx->dpy));
    }
    if (!glXCreateContextAttribsARB)
    {
        glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
                glXGetProcAddressARB((const GLubyte *)
                        "glXCreateContextAttribsARB");
    }
    if (!glctx_glx_supports_extension(glctx_glx_extensions,
            "GLX_ARB_create_context") ||
            !glXCreateContextAttribsARB)
    {
        glctx__log("glctx: GLX_ARB_create_context not supported\n");
        return GLCTX_ERROR_CONTEXT;
    }

    glctx__log("glctx: Creating context\n");
    ctx->ctx = glXCreateContextAttribsARB(ctx->dpy, config,
            0, True, attrs);
    if (!ctx->ctx)
    {
        glctx__log("glctx: glXCreateContextAttribsARB failed\n");
        return GLCTX_ERROR_CONTEXT;
    }

    if (!glXIsDirect(ctx->dpy, ctx->ctx))
    {
        glctx__log("glctx: Warning: rendering is not direct\n");
    }

    return glctx_bind(ctx);
}

GlctxNativeContext glctx_get_native_context(GlctxHandle ctx)
{
    return ctx->ctx;
}

#if 0
int glctx_get_width(GlctxHandle ctx)
{
    return ctx->width;
}

int glctx_get_height(GlctxHandle ctx)
{
    return ctx->height;
}
#endif

void glctx_flip(GlctxHandle ctx)
{
    glXSwapBuffers(ctx->dpy, ctx->window);
}

GlctxError glctx_unbind(GlctxHandle ctx)
{
    glXMakeCurrent(ctx->dpy, None, NULL);
    return GLCTX_ERROR_NONE;
}

GlctxError glctx_bind(GlctxHandle ctx)
{
    glXMakeCurrent(ctx->dpy, ctx->window, ctx->ctx);
    return GLCTX_ERROR_NONE;
}

void glctx_terminate(GlctxHandle ctx)
{
    if (ctx->dpy)
    {
        glctx_unbind(ctx);
        if (ctx->ctx)
        {
            glXDestroyContext(ctx->dpy, ctx->ctx);
            ctx->ctx = NULL;
        }
    }
    free(ctx);
}
