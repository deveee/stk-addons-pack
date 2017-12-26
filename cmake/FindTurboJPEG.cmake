# - Find TurboJPEG
# Find the TurboJPEG includes and libraries
#
# Following variables are provided:
# TURBOJPEG_FOUND
#     True if TurboJPEG has been found
# TURBOJPEG_INCLUDE_DIR
#     The include directory of TurboJPEG
# TURBOJPEG_LIBRARY
#     TurboJPEG library list

find_path(TURBOJPEG_INCLUDE_DIR turbojpeg.h)
find_library(TURBOJPEG_LIBRARY NAMES turbojpeg)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TurboJPEG DEFAULT_MSG TURBOJPEG_LIBRARY 
                                                        TURBOJPEG_INCLUDE_DIR)

mark_as_advanced(TURBOJPEG_LIBRARY TURBOJPEG_INCLUDE_DIR)
