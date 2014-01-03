#ifdef ENABLE_OPENGL
#include "GL/glew.h"
#define MY_GL_VERSION_MINOR 1
#else
#include "GLES2/gl2.h"
#define MY_GL_VERSION_MINOR 0
#endif

#include <glctx/glctx.h>

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Surface *init_sdl_window()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Surface *surf = SDL_SetVideoMode(640, 480, 0, 0);
    if (!surf)
    {
        printf("Failed to create SDL window\n");
        exit(1);
    }
    return surf;
}

#if !ENABLE_OPENGLES
static void init_glew(void)
{
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        printf("glewInit failed: %s\n", glewGetErrorString(err));
        exit(1);
    }
    printf("GLEW %s successful\n", glewGetString(GLEW_VERSION));
}
#endif

static GlctxHandle init_gl(void)
{
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    GlctxConfig config;
    int cfg_attrs[] = {
            GLCTX_CFG_RED_SIZE, 8,
            GLCTX_CFG_GREEN_SIZE, 8,
            GLCTX_CFG_BLUE_SIZE, 8,
            GLCTX_CFG_END
    };
    GlctxDisplay dpy;
    GlctxWindow win;

    if (!SDL_GetWMInfo(&wminfo))
    {
        printf("SDL_GetWMInfo not implemented\n");
        exit(1);
    }

    glctx_set_log_function(printf);
    GlctxHandle ctx;
    GlctxError err;
#if defined (_WIN32)
    dpy = EGL_DEFAULT_DISPLAY
    win = wminfo.window;;
#else
    dpy = wminfo.info.x11.display;
    win = wminfo.info.x11.window;
#endif
    err = glctx_init(dpy, win, MY_GLCTX_PROFILE, 2,
#if GLCTX_ENABLE_OPENGL
            1,
#else
            0,
#endif
            &ctx);
    if (err)
    {
        printf("glctx_init failed: %s\n", glctx_get_error_name(err));
        exit(1);
    }

    err = glctx_get_config(ctx, &config, cfg_attrs, 0);
    if (err)
    {
        printf("glctx_get_config failed: %s\n", glctx_get_error_name(err));
        exit(1);
    }

    err = glctx_activate(ctx, config, win, NULL);
    if (err)
    {
        printf("glctx_activate failed: %s\n", glctx_get_error_name(err));
        exit(1);
    }

#if !ENABLE_OPENGLES
    init_glew();
#endif

    printf("OpenGL '%s', GLSL '%s'\n", glGetString(GL_VERSION),
        glGetString(GL_SHADING_LANGUAGE_VERSION));

    return ctx;
}

static void check_shader_build(GLuint shader, GLenum status_pname,
        int link, const char *msg)
{
    GLint status = GL_TRUE;
    if (link)
        glGetProgramiv(shader, status_pname, &status);
    else
        glGetShaderiv(shader, status_pname, &status);
    if (status == GL_FALSE)
    {
        GLint len;
        if (link)
            glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &len);
        else
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        size_t l = strlen(msg);
        char *logstr = malloc(l + len + 1);
        strcpy(logstr, msg);
        if (link)
            glGetProgramInfoLog(shader, len, 0, (GLchar *) logstr + l);
        else
            glGetShaderInfoLog(shader, len, 0, (GLchar *) logstr + l);
        printf("%s\n", logstr);
        exit(1);
    }
}

static GLuint compile_shader(GLenum stype, const char *src)
{
    GLuint shader = glCreateShader(stype);
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);
    check_shader_build(shader, GL_COMPILE_STATUS, 0,
            (stype == GL_VERTEX_SHADER)
                    ? "Error compiling vertex shader: "
                    : "Error compiling fragment shader: ");
    return shader;
}

GLuint link_shaders(GLuint vert, GLuint frag)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    check_shader_build(prog, GL_LINK_STATUS, 1, "Error linking shaders: ");
    glDetachShader(prog, vert);
    glDeleteShader(vert);
    glDetachShader(prog, frag);
    glDeleteShader(frag);
    return prog;
}

#if ENABLE_OPENGLES
#define GLSL_VERSION "#version 100\n"
#else
#define GLSL_VERSION "#version 120\n"
#endif

/* Returns shader prog */
static GLuint load_shaders(GLint *coord2d_tag, GLint *angle_tag)
{
    GLuint vert = compile_shader(GL_VERTEX_SHADER,
        GLSL_VERSION
        "attribute vec2 coord2d;\n"
        "attribute float angle;\n"
        "void main() {\n"
        "  mat2 rotation = mat2(\n"
        "    vec2( cos(angle), sin(angle)),\n"
        "    vec2(-sin(angle), cos(angle)));\n"
        "  gl_Position = vec4(rotation * coord2d, 0.0, 1.0);\n"
        "}\n");
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER,
        GLSL_VERSION
        "void main(void) {\n"
        "  gl_FragColor[0] = 0.0;\n"
        "  gl_FragColor[1] = 0.0;\n"
        "  gl_FragColor[2] = 1.0;\n"
        "}\n");
    GLuint prog = link_shaders(vert, frag);
    *coord2d_tag = glGetAttribLocation(prog, "coord2d");
    if (*coord2d_tag == -1)
    {
        printf("Failed to find coord2d attribute in shader prog\n");
        exit(1);
    }
    *angle_tag = glGetAttribLocation(prog, "angle");
    if (*angle_tag == -1)
    {
        printf("Failed to find angle attribute in shader prog\n");
        exit(1);
    }
    return prog;
}

#define COS30 0.8660254f
#define SIN30 0.5f

static void render_loop(GlctxHandle ctx, int w, int h)
{
    int running = 1;
    static const GLfloat verts[] = {
             0.0f,          0.9f,
            -0.9f * COS30, -0.9f * SIN30,
             0.9f * COS30, -0.9f * SIN30,
    };
    GLint coord2d, angle_tag;
    GLuint vbo;
    GLuint prog;
    float angle = 0;

    if (w <= 0 || h <= 0)
    {
        printf("Unable to get screen size\n");
    }
    else
    {
        if (w >= h)
            glViewport((w - h) / 2, 0, h, h);
        else
            glViewport(0, (h - w) / 2, w, w);
    }
    printf("Setting up shaders and buffers\n");
    prog = load_shaders(&coord2d, &angle_tag);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glClearColor(1, 1, 1, 1);
    glctx_flip(ctx);

    while (running)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glVertexAttrib1f(angle_tag, angle);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(coord2d);
        glVertexAttribPointer(coord2d, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(coord2d);
        glctx_flip(ctx);
        SDL_Event ev;
        if (SDL_PollEvent(&ev) && (ev.type == SDL_QUIT
                || (ev.type == SDL_KEYDOWN
                        && ev.key.keysym.sym == SDLK_ESCAPE)))
        {
            running = 0;
        }
        angle += 0.0314159f;
    }
}

int main(int argc, char **argv)
{
    GlctxHandle ctx;
    SDL_Surface *surf;
    int w, h;
#if GLCTX_ENABLE_RPI
    EGLDisplay edpy;
    EGLSurface esurf;

    bcm_host_init();
#endif

    (void) argc;
    (void) argv;

    surf = init_sdl_window();
    ctx = init_gl();
#if GLCTX_ENABLE_RPI
    edpy = glctx_get_egl_display(ctx);
    esurf = glctx_get_egl_surface(ctx);
    if (!eglQuerySurface(edpy, esurf, EGL_WIDTH, &w) ||
        !eglQuerySurface(edpy, esurf, EGL_HEIGHT, &h))
    {
        printf("Unable to get size of EGL surface\n");
        w = h = 0;
    }
#else
    w = surf->w;
    h = surf->h;
#endif
    render_loop(ctx, w, h);

    return 0;
}
