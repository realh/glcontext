# GLContext on Raspberry Pi #

Raspberry Pi differs from conventional Linux by only supporting OpenGL ES and EGL, with specialised libraries in /opt/vc. You also need to interact with the Broadcom chip via its own API when setting up an EGL context. The accelerated rendering is independent of X and it only really makes sense to use full-screen mode, but an X window (eg provided by SDL) hidden behind the GL framebuffer is a good way to handle input for games etc.

GLContext takes care of nearly all of these details, but you will have to call `bcm_host_init` yourself, before `glctx_init`. You can use the `GLCTX_ENABLE_RPI` macro from `<glctx/glctx-config.h>` to decide whether this is necessary. There is a corresponding `bcm_host_deinit` you can use when finished. The `glctx_` functions ignore the `display` and `window` parameters on Raspberry Pi, so you can either use 0 for Pi-specific programs, or the same values as you would use in other versions of Linux.

The [test program](https://code.google.com/p/glcontext/source/browse/tests/glctx-test.c) shows all of this in action, including how to query the size of the EGL surface. On a normal PC one would usually read the size from the window or SDL surface instead.