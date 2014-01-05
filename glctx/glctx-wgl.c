#include "glctx.h"

#include <stdlib.h>
#include <string.h>

extern int (*glctx__log)(const char *format, ...);
extern int glctx__log_ignore(const char *format, ...);
extern int *glctx__make_attrs_buffer(const int *attrs,
        const int *native_attrs, int native_attr_term);

static int glctx__profile_table[] = {
    0x0004,
    0x0001,
    0x0001,
    0x0002
};

struct GlctxData_ {
    HDC dpy;
    HWND window;
    GlctxProfile profile;
    int maj_version, min_version;
	HGLRC ctx;
};

GlctxError glctx_init(GlctxDisplay display, GlctxWindow window,
        GlctxProfile profile, int maj_version, int min_version,
        GlctxHandle *pctx)
{
    GlctxHandle ctx = (GlctxHandle) malloc(sizeof(struct GlctxData_));

    *pctx = NULL;
    if (!ctx)
        return GLCTX_ERROR_MEMORY;
    ctx->dpy = display;
    ctx->window = window;
    ctx->profile = profile;
    ctx->maj_version = maj_version;
    ctx->min_version = min_version;
    *pctx = ctx;
    return GLCTX_ERROR_NONE;
}

GlctxError glctx_get_config(GlctxHandle ctx, GlctxConfig *cfg_out,
        const int *attrs, int native_attrs)
{
	PIXELFORMATDESCRIPTOR pfd;
	int color_bits = 0;

	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;

	if (attrs)
	{
		int n;

		for (n = 0; attrs[n]; n += 2)
		{
			switch (attrs[n])
			{
			case GLCTX_CFG_RED_SIZE:
			case GLCTX_CFG_GREEN_SIZE:
			case GLCTX_CFG_BLUE_SIZE:
			case GLCTX_CFG_ALPHA_SIZE:
				color_bits += attrs[n + 1];
				break;
			case GLCTX_CFG_DEPTH_SIZE:
				pfd.cDepthBits = attrs[n + 1];
				break;
			case GLCTX_CFG_STENCIL_SIZE:
				pfd.cStencilBits = attrs[n + 1];
				break;
			default:
				break;
			}
		}
	}

	if (color_bits >= 15 && color_bits < 24)
		pfd.cColorBits = 16;
	else
		pfd.cColorBits = 32;

	*cfg_out = ChoosePixelFormat(ctx->dpy, &pfd);
	if (!*cfg_out)
		return GLCTX_ERROR_CONFIG;

    return GLCTX_ERROR_NONE;
}

int glctx_query_config(GlctxHandle ctx, GlctxConfig config, GlctxAttr attr)
{
    PIXELFORMATDESCRIPTOR pfd;

	if (!DescribePixelFormat(ctx->dpy, config, sizeof(PIXELFORMATDESCRIPTOR),
	        &pfd))
	{
		glctx__log("glctx: DescribePixelFormat failed (%ld)\n", GetLastError());
		return -1;
	}

	switch (attr)
	{
	case GLCTX_CFG_RED_SIZE:
	case GLCTX_CFG_GREEN_SIZE:
	case GLCTX_CFG_BLUE_SIZE:
	case GLCTX_CFG_ALPHA_SIZE:
		/* AFAIK the format always includes alpha */
		if (pfd.cColorBits == 16)
			return 4;
		else
			return 8;
	case GLCTX_CFG_DEPTH_SIZE:
		return pfd.cDepthBits;
	case GLCTX_CFG_STENCIL_SIZE:
		return pfd.cStencilBits;
	default:
		glctx__log("glctx: Bad attribute code %d for glctx_query_config\n",
		        attr);
	}
    return -1;
}

typedef HGLRC (__stdcall *wglCreateContextAttribsARBProc)
        (HDC, HGLRC, const int *);

GlctxError glctx_activate(GlctxHandle ctx, GlctxConfig config,
        GlctxWindow window, const int *attrs)
{

    PIXELFORMATDESCRIPTOR pfd;
	HGLRC fake_ctx;
	GlctxError result;
	wglCreateContextAttribsARBProc wglCreateContextAttribsARB;

	if (!DescribePixelFormat(ctx->dpy, config, sizeof(PIXELFORMATDESCRIPTOR),
	        &pfd))
	{
		glctx__log("glctx: DescribePixelFormat failed: %ld\n", GetLastError());
		return GLCTX_ERROR_CONFIG;
	}
	if (!SetPixelFormat(ctx->dpy, config, &pfd))
	{
		glctx__log("glctx: SetPixelFormat failed: %ld\n", GetLastError());
		return GLCTX_ERROR_CONFIG;
	}

	/* Get initial context */
	fake_ctx = wglCreateContext(ctx->dpy);
	if (!fake_ctx)
	{
		glctx__log("glctx: wglCreateContext failed: %ld\n", GetLastError());
		return GLCTX_ERROR_CONTEXT;
	}
	ctx->ctx = fake_ctx;
	result = glctx_bind(ctx);
	if (result)
		return result;

	/* Use extension if possible */
	wglCreateContextAttribsARB = (wglCreateContextAttribsARBProc)
				wglGetProcAddress("wglCreateContextAttribsARB");
	if (wglCreateContextAttribsARB)
	{
		int default_attrs[] = {
			0x9126, glctx__profile_table[ctx->profile],
			0x2091, ctx->maj_version,
			0x2092, ctx->min_version,
			0
		};

		if (!attrs)
			attrs = default_attrs;
		ctx->ctx = wglCreateContextAttribsARB(ctx->dpy, NULL, attrs);
		if (ctx->ctx)
		{
			 wglMakeCurrent(ctx->dpy, NULL);
			 wglDeleteContext(fake_ctx);
			 result = glctx_bind(ctx);
			 if (result)
				 return result;
		}
		else
		{
			glctx__log("glctx: wglCreateContextAttribsARB failed (%ld), "
					"falling back to old style context\n");
			ctx->ctx = fake_ctx;
		}
	}

	return GLCTX_ERROR_NONE;
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
    SwapBuffers(ctx->dpy);
}

GlctxError glctx_unbind(GlctxHandle ctx)
{
    wglMakeCurrent(ctx->dpy, NULL);
    return GLCTX_ERROR_NONE;
}

GlctxError glctx_bind(GlctxHandle ctx)
{
    if (!wglMakeCurrent(ctx->dpy, ctx->ctx))
	{
		glctx__log("glctx: wglMakeCurrent failed (%ld)\n", GetLastError());
		return GLCTX_ERROR_BIND;
	}
    return GLCTX_ERROR_NONE;
}

void glctx_terminate(GlctxHandle ctx)
{
    if (ctx->dpy)
    {
        glctx_unbind(ctx);
        if (ctx->ctx)
        {
            wglDeleteContext(ctx->ctx);
            ctx->ctx = NULL;
        }
    }
    free(ctx);
}
