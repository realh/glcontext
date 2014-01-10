project(glcontext)
cmake_minimum_required(VERSION 2.8)
set(GLCTX_VERSION_MAJOR 1)
set(GLCTX_VERSION_MINOR 0)

set(GLCTX_SOVERSION ${GLCTX_VERSION_MAJOR})
set(GLCTX_VERSION_STRING "${GLCTX_VERSION_MAJOR}.${GLCTX_VERSION_MINOR}")

include(CMakeDependentOption)
include(GenerateExportHeader)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ENABLE_TESTS "Build tests" ON)

if(NOT MSVC)
    # add_compile_options would be better, but it's too new
    add_definitions(-Wall -Wextra -Wunused -Wwrite-strings)
endif()

# Work out platform/back-end
if(UNIX AND NOT APPLE)
    # Raspberry Pi?
    if(EXISTS "/opt/vc/include/EGL/egl.h")
        set(HAVE_RPI_INC 1)
    else()
        set(HAVE_RPI_INC 0)
    endif()
    execute_process(COMMAND "uname" "-m" "-o" OUTPUT_VARIABLE MY_UNAME)
    if ("${MY_UNAME}" STREQUAL "armv6l GNU/Linux\n" AND HAVE_RPI_INC)
        option(ENABLE_RPI "Build for Raspberry Pi" ON)
    else()
        option(ENABLE_RPI "Build for Raspberry Pi" OFF)
    endif()
else()
    set(ENABLE_RPI OFF)
endif()

# Support multiarch on Debian etc
if(NOT CMAKE_ARCH_TRIPLET)
    execute_process(COMMAND "dpkg-architecture" "-qDEB_HOST_MULTIARCH"
            OUTPUT_VARIABLE CMAKE_ARCH_TRIPLET ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(CMAKE_ARCH_TRIPLET)
    set(GLCTX_LIBDIR "lib/${CMAKE_ARCH_TRIPLET}")
else()
    set(GLCTX_LIBDIR "lib")
endif()


CMAKE_DEPENDENT_OPTION(ENABLE_EGL "Use EGL" OFF "NOT ENABLE_RPI" ON)

if(ENABLE_RPI)
    set(GLCTX_ENABLE_RPI 1)
else()
    set(GLCTX_ENABLE_RPI 0)
endif()

if(ENABLE_EGL)
    set(GLCTX_ENABLE_EGL 1)
    set(GLCTX_ENABLE_GLX 0)
    set(GLCTX_ENABLE_WGL 0)
    set(GLCTX_BACKEND_NAME "EGL")
else()
    set(GLCTX_ENABLE_EGL 0)
    if(WIN32)
        set(GLCTX_ENABLE_GLX 0)
        set(GLCTX_ENABLE_WGL 1)
        set(GLCTX_BACKEND_NAME "WGL")
    else()
        set(GLCTX_ENABLE_GLX 1)
        set(GLCTX_ENABLE_WGL 0)
        set(GLCTX_BACKEND_NAME "GLX")
    endif()
endif()

if(WIN32)
    set(GLCTX_MSWIN 1)
    set(GLCTX_X11 0)
else()
    set(GLCTX_MSWIN 0)
    set(GLCTX_X11 1)
endif()

# Find libraries we need
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake/modules)
if(ENABLE_RPI)
    find_package(BcmHost REQUIRED)
    find_package(RPiEGL REQUIRED)
    set(GLCTX_INCLUDES ${EGL_INCLUDE_DIR} ${BCM_HOST_INCLUDE_DIR})
    set(GLCTX_LIBRARIES ${EGL_LIBRARY} ${BCM_HOST_LIBRARY})
    set(GLCTX_PKG_LIBS "-L/opt/vc/lib -lEGL -lbcm_host")
elseif(ENABLE_EGL)
    find_package(EGL REQUIRED)
    set(GLCTX_INCLUDES ${EGL_INCLUDE_DIR})
    set(GLCTX_LIBRARIES ${EGL_LIBRARY})
    set(GLCTX_PKG_DEPS "${GLCTX_PKG_DEPS} egl")
elseif(GLCTX_ENABLE_GLX)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    set(GLCTX_INCLUDES ${OPENGL_INCLUDE_DIR} ${X11_INCLUDE_DIR})
    set(GLCTX_LIBRARIES ${OPENGL_gl_LIBRARY} ${X11_LIBRARY})
    set(GLCTX_PKG_DEPS "${GLCTX_PKG_DEPS} gl")
elseif(GLCTX_ENABLE_WGL)
    find_package(OpenGL REQUIRED)
    set(GLCTX_INCLUDES ${OPENGL_INCLUDE_DIR})
    set(GLCTX_LIBRARIES ${OPENGL_gl_LIBRARY})
    set(GLCTX_PKG_DEPS "${GLCTX_PKG_DEPS} gl")
endif()

list(APPEND GLCTX_PKG_INCLUDES ${CMAKE_INSTALL_PREFIX}/include)
foreach(I ${GLCTX_INCLUDES})
    list(APPEND GLCTX_PKG_INCLUDES ${I})
endforeach()
list(REMOVE_DUPLICATES GLCTX_PKG_INCLUDES)
foreach(I IN LISTS GLCTX_PKG_INCLUDES)
    set(GLCTX_PKG_CFLAGS "${GLCTX_PKG_CFLAGS} -I${I}")
endforeach()

if(NOT WIN32)
    set(GLCTX_PKG_DEPS "${GLCTX_PKG_DEPS} x11")
endif()

include_directories(${GLCTX_INCLUDES}
        ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})

configure_file(glctx-config.h.in glctx-config.h)
configure_file(glcontext.pc.in glcontext.pc @ONLY)
configure_file(FindGLContext.cmake.in FindGLContext.cmake @ONLY)

# Choose source and build library
if(ENABLE_EGL)
    set(GLCTX_SRC glctx/glctx-egl.c)
elseif(GLCTX_ENABLE_GLX)
    set(GLCTX_SRC glctx/glctx-glx.c)
elseif(GLCTX_ENABLE_WGL)
    set(GLCTX_SRC glctx/glctx-wgl.c)
endif()
set(GLCTX_SRC ${GLCTX_SRC} glctx/glctx-common.c glctx/glctx.h)
add_library(glcontext ${GLCTX_SRC})
generate_export_header(glcontext BASE_NAME glctx)
if(BUILD_SHARED_LIBS)
    set_target_properties(glcontext PROPERTIES VERSION ${GLCTX_VERSION_STRING})
    if (UNIX)
        set_target_properties(glcontext PROPERTIES SOVERSION ${GLCTX_SOVERSION})
    endif()
endif()
target_link_libraries(glcontext ${GLCTX_LIBRARIES})

if(ENABLE_TESTS)
    find_package(SDL 1.2)
    if(NOT SDL_FOUND)
        message(WARNING "Unable to build tests: SDL 1.2 not found")
    else()
        if(ENABLE_RPI)
            find_package(RPiGLESv2)
        else()
            find_package(OpenGL)
            find_package(GLESv2)
            find_package(GLEW)
        endif()
        if(OPENGL_FOUND AND NOT GLEW_FOUND)
            message(WARNING "Unable to build OpenGL test: GLEW not found")
            set(OPENGL_FOUND 0)
        endif()
        if(NOT OPENGL_FOUND AND NOT GLESV2_FOUND)
            message(WARNING "Unable to build tests: no OpenGL or GLESv2")
        else()
            if(OPENGL_FOUND)
                message("Building OpenGL test")
            else()
                message("Not building OpenGL test")
            endif()
            if(GLESV2_FOUND)
                message("Building OpenGL ES test")
            else()
                message("Not building OpenGL ES test")
            endif()
        endif()
        if(OPENGL_FOUND)
            add_executable(glctx-testgl tests/glctx-test.c)
            target_compile_definitions(glctx-testgl PUBLIC
                    -DMY_GLCTX_PROFILE=GLCTX_PROFILE_OPENGL
                    -DGLCTX_ENABLE_OPENGL)
            target_include_directories(glctx-testgl PUBLIC
                    ${GLEW_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
            target_link_libraries(glctx-testgl glcontext ${GLCTX_LIBRARIES}
                    ${SDL_LIBRARY} ${GLEW_LIBRARIES} ${OPENGL_gl_LIBRARY})
        endif()
        if(GLESV2_FOUND)
            add_executable(glctx-testgles tests/glctx-test.c)
            target_compile_definitions(glctx-testgles PUBLIC
                    -DMY_GLCTX_PROFILE=GLCTX_PROFILE_OPENGLES
                    -DGLCTX_ENABLE_OPENGLES)
            target_include_directories(glctx-testgles PUBLIC
                    ${GLESv2_INCLUDE_DIR})
            target_link_libraries(glctx-testgles glcontext ${GLCTX_LIBRARIES}
                    ${SDL_LIBRARY} ${GLESv2_LIBRARY})
        endif()
    endif()
endif()


install(TARGETS glcontext EXPORT glcontext
        LIBRARY DESTINATION ${GLCTX_LIBDIR}
        ARCHIVE DESTINATION ${GLCTX_LIBDIR})
install(FILES
        ${PROJECT_SOURCE_DIR}/glctx/glctx.h
        ${PROJECT_BINARY_DIR}/glctx-config.h
        ${PROJECT_BINARY_DIR}/glctx_export.h
        DESTINATION include/glctx)
install(FILES ${PROJECT_BINARY_DIR}/glcontext.pc
        DESTINATION ${GLCTX_LIBDIR}/pkgconfig)
install(FILES ${PROJECT_BINARY_DIR}/FindGLContext.cmake
        DESTINATION ${GLCTX_LIBDIR}/cmake/GLContext)

# This isn't much use
#install(EXPORT glcontext
#        FILE GLContext.cmake
#        DESTINATION ${GLCTX_LIBDIR}/cmake/GLContext)
