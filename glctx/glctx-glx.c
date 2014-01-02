#include "glctx.h"

#include "GL/glx.h"

#include <stdlib.h>
#include <string.h>

extern int (*glctx__log)(const char *format, ...);
extern int glctx__log_ignore(const char *format, ...);

struct GlctxData_ {
    Display *dpy;
    Window window;
    int screen;
    int width, height;
    GlctxApiType api;
    int version_maj;
    int version_min;
    GLXFBConfig fbc;
    GLXContext ctx;
};

GlctxError glctx_init(GlctxDisplay display, GlctxWindow window,
                      GlctxApiType api, int version_maj, int version_min,
                      GlctxHandle *pctx)
{
    XWindowAttributes attribs;
    GlctxHandle ctx = malloc(sizeof(struct GlctxData_));

    *pctx = NULL;
    if (!ctx)
        return GLCTX_ERROR_MEMORY;

    if (!XGetWindowAttributes(display, window, &attribs))
    {
        glctx__log("glctx: Unable to get window attributes\n");
        free(ctx);
        return GLCTX_ERROR_WINDOW;
    }

    ctx->dpy = display;
    ctx->window = window;
    ctx->screen = XScreenNumberOfScreen(attribs.screen);
    ctx->width = attribs.width;
    ctx->height = attribs.height;
    ctx->api = api;
    ctx->version_maj = version_maj;
    ctx->version_min = version_min;
    ctx->fbc = NULL;

    *pctx = ctx;

    return GLCTX_ERROR_NONE;
}

GlctxApiType glctx_query_api_type(GlctxHandle ctx)
{
    return ctx->api ? ctx->api : GLCTX_API_OPENGL;
}

GlctxError glctx_get_config(GlctxHandle ctx,
                            int r, int g, int b, int a, int depth)
{
    int fbc_count;
    GLXFBConfig* fbc;
    int i;
    int best_nsamples = -1;
    int attrs[] = {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_RED_SIZE        , r,
        GLX_GREEN_SIZE      , g,
        GLX_BLUE_SIZE       , b,
        GLX_ALPHA_SIZE      , a,
        GLX_DEPTH_SIZE      , depth,
        GLX_DOUBLEBUFFER    , True,
        /*
        GLX_STENCIL_SIZE    , 8,
        GLX_SAMPLE_BUFFERS  , 1,
        GLX_SAMPLES         , 4,
        */
        None
    };

    fbc = glXChooseFBConfig(ctx->dpy, ctx->screen, attrs, &fbc_count);
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
                ctx->fbc = fbc[i];
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

void glctx_describe_config(GlctxHandle ctx,
                           int *r, int *g, int *b, int *a, int *depth)
{
    if (r)
        glXGetFBConfigAttrib(ctx->dpy, ctx->fbc, GLX_RED_SIZE, r);
    if (g)
        glXGetFBConfigAttrib(ctx->dpy, ctx->fbc, GLX_GREEN_SIZE, g);
    if(b)
        glXGetFBConfigAttrib(ctx->dpy, ctx->fbc, GLX_BLUE_SIZE, b);
    if (a)
        glXGetFBConfigAttrib(ctx->dpy, ctx->fbc, GLX_ALPHA_SIZE, a);
    if (depth)
        glXGetFBConfigAttrib(ctx->dpy, ctx->fbc, GLX_DEPTH_SIZE, depth);
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

#ifndef GLX_CONTEXT_MAJOR_VERSION_ARB
#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#endif
#ifndef GLX_CONTEXT_MINOR_VERSION_ARB
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
#endif
#ifndef GLX_CONTEXT_PROFILE_MASK_ARB
#define GLX_CONTEXT_PROFILE_MASK_ARB        0x9126
#endif
#ifndef GLX_CONTEXT_ES_PROFILE_BIT_EXT
#define GLX_CONTEXT_ES_PROFILE_BIT_EXT      0x0004
#endif

GlctxError glctx_activate(GlctxHandle ctx)
{
    int attrs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, ctx->version_maj,
        GLX_CONTEXT_MINOR_VERSION_ARB, ctx->version_min,
        GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_ES_PROFILE_BIT_EXT,
        None
      };

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
    if (ctx->api == GLCTX_API_OPENGLES)
    {
        if (!glctx_glx_supports_extension(glctx_glx_extensions,
                    "GLX_ARB_create_context_profile"))
        {
             glctx__log("glctx: GLX_ARB_create_context_profile not supported "
                    "(required for OpenGL ES)\n");
            return GLCTX_ERROR_API;
        }
    }
    else
    {
        attrs[4] = None;
    }

    glctx__log("glctx: Creating context\n");
    ctx->ctx = glXCreateContextAttribsARB(ctx->dpy, ctx->fbc,
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

int glctx_get_width(GlctxHandle ctx)
{
    return ctx->width;
}

int glctx_get_height(GlctxHandle ctx)
{
    return ctx->height;
}

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
    glctx__log("glctx: Binding context\n");
    glXMakeCurrent(ctx->dpy, ctx->window, ctx->ctx);
    glctx__log("glctx: Bound context\n");
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
