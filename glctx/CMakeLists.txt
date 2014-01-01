configure_file(glctx-config.h.in glctx/glctx-config.h)

# Choose source and build library
if(ENABLE_EGL)
    set(GLCTX_SRC glctx-egl.c)
elseif(ENABLE_GLX)
    set(GLCTX_SRC glctx-glx.c)
elseif(ENABLE_WGL)
    set(GLCTX_SRC glctx-wgl.c)
endif()
