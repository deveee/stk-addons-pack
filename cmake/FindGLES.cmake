# - Find OpenGL ES
# Find the OpenGL ES includes and libraries
#
# Following variables are provided:
# GLES_FOUND
#     True if OpenGL ES has been found
# GLES_INCLUDE_DIRS
#     The include directories of OpenGL ES
# GLES_LIBRARY
#     GLES library list

find_path(GLES2_INCLUDE_DIR GLES2/gl2.h)
find_path(GLES3_INCLUDE_DIR GLES3/gl3.h)
find_library(GLES_LIBRARY NAMES GLESv3 GLESv2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLES DEFAULT_MSG GLES_LIBRARY 
												   GLES2_INCLUDE_DIR 
												   GLES3_INCLUDE_DIR)

set(GLES_INCLUDE_DIRS ${GLES2_INCLUDE_DIR} ${GLES3_INCLUDE_DIR})

list(REMOVE_DUPLICATES GLES_INCLUDE_DIRS)

mark_as_advanced(GLES_LIBRARY GLES2_INCLUDE_DIR GLES3_INCLUDE_DIR)
