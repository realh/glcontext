
# Find bcm_host libs for Raspberry Pi
#
# BCM_HOST_INCLUDE_DIR
# BCM_HOST_LIBRARY
# BCM_HOST_FOUND

find_path(BCM_HOST_INCLUDE_DIR bcm_host.h PATHS /opt/vc/include NO_DEFAULT_PATH)
if (NOT ${BCM_HOST_INCLUDE_DIR} MATCHES "NOTFOUND")
    set(BCM_HOST_INCLUDE_DIR ${BCM_HOST_INCLUDE_DIR}
            ${BCM_HOST_INCLUDE_DIR}/interface/vcos
            ${BCM_HOST_INCLUDE_DIR}/interface/vcos/pthreads
            ${BCM_HOST_INCLUDE_DIR}/interface/vmcs_host/linux)
endif()

find_library(BCM_HOST_LIBRARY bcm_host PATHS /opt/vc/lib NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BCM_HOST DEFAULT_MSG
        BCM_HOST_LIBRARY BCM_HOST_INCLUDE_DIR)

mark_as_advanced(BCM_HOST_INCLUDE_DIR BCM_HOST_LIBRARY)
