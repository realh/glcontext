# Find EGL for Raspberry Pi
#
# EGL_INCLUDE_DIR
# EGL_LIBRARY
# EGL_FOUND

find_path(EGL_INCLUDE_DIR EGL/egl.h PATHS /opt/vc/include NO_DEFAULT_PATH)
find_library(EGL_LIBRARY EGL PATHS /opt/vc/lib NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL DEFAULT_MSG EGL_LIBRARY EGL_INCLUDE_DIR)

mark_as_advanced(EGL_INCLUDE_DIR EGL_LIBRARY)
