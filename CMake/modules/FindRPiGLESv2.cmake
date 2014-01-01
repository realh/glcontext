# Find GLESv2 for Raspberry Pi
#
# GLESv2_INCLUDE_DIR
# GLESv2_LIBRARY
# GLESv2_FOUND

find_path(GLESv2_INCLUDE_DIR GLES2/gl2.h PATHS /opt/vc/include NO_DEFAULT_PATH)
find_library(GLESv2_LIBRARY GLESv2 PATHS /opt/vc/lib NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLESv2 DEFAULT_MSG
        GLESv2_LIBRARY GLESv2_INCLUDE_DIR)

mark_as_advanced(GLESv2_INCLUDE_DIR GLESv2_LIBRARY)
