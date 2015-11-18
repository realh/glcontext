# Introduction #

GLContext is a cross-platform library with a unified API for setting up an OpenGL or OpenGL ES context. It currently supports MS Windows, Linux (and other Unixes with GLX or EGL), including on Raspberry Pi, and Android (not fully tested yet). I hope to add Mac and iOS support one day.

All files are distributed under a BSD license, see [COPYING.txt](https://glcontext.googlecode.com/git/COPYING.txt) for details.

# Motivation #

There are enough similarities between OpenGL 2.1 core profile and OpenGL ES 2.0 that it's easy to write a portable game that uses essentially the same rendering code and shaders (with some caveats, see for example [this section of the OpenGL Wiki book](http://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Introduction#Vertex_shader) on several desktop and mobile platforms.

However, each platform has its own way of preparing a window or screen for OpenGL rendering aka obtaining an OpenGL context. Windows uses WGL, while Linux uses GLX. Where OpenGL ES is supported you can use EGL, but that won't work on PCs with NVidia graphics cards. glcontext provides a simple API that unifies this process for several popular platforms.