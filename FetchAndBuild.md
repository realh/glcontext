# Fetch and build glcontext #

## Fetch with git ##

As a new project glcontext is only available via git. The [Source section](https://code.google.com/p/glcontext/source/checkout) contains further instructions.

## Choosing back-end ##

You should first choose which variant (EGL, WGL or GLX) of glcontext to build. For Windows I recommend WGL, using OpenGL in client programs. For Linux on conventional PCs use GLX and OpenGL. For Raspberry Pi use EGL and OpenGL ES. If run with no options cmake should auto-detect the platform and choose the appropriate back-end as above.

## Prerequesites ##

When using OpenGL (not ES) you will still need some way to [load OpenGL functions](http://www.opengl.org/wiki/Load_OpenGL_Functions). glcontext's test program uses [GLEW](http://glew.sourceforge.net/). Note GLEW is not compatible with EGL. The test program also uses [SDL](http://libsdl.org/) 1.2. For Linux you can simply install packages such as libsdl1.2-dev and libglew-dev, but for Windows you need to install them somewhere where CMake will be able to find them.

InstallingWindowsLibs

## Running CMake ##

glcontext uses [cmake](http://www.cmake.org/) to set up a build system. Thus you can use the standard make utility on Unix or generate a MSVC project file on Windows (MinGW is also an option).

The easiest way to configure the project is with cmake-gui, but on Raspberry Pi it's probably better to use the curses version (ccmake) for performance. Or you can run cmake on the command line and set options by adding `-DENABLE_SOME_OPTION` or `-DSOME_VARIABLE=value`. Generally it's better to use a separate directory for building to avoid cluttering up the source tree. cmake-gui has options to configure both. With ccmake or cmake run it from the build folder and specify the source folder (containing CMakeLists.txt) as a command argument. If all goes well the default settings should be appropriate for your platform and you can go ahead and click Generate. You may also want to configure the build type eg Debug or Release.

Now you can run make or load the generated project into MSVC for building. If the test is enabled you should see a spinning blue triangle on a white background when you run it. Close the window or press Esc to terminate it.