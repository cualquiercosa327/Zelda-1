################################################################################
# Custom cmake module for zelda to find CEGUI
################################################################################
include(FindPackageHandleStandardArgs)

find_path(CEGUI_H_PATH NAMES CEGUI/CEGUI.h PATH_SUFFIXES cegui-0)
find_library(CEGUI_LIB NAMES CEGUIBase-0)
find_library(CEGUI_OGRE_RENDERER_LIB NAMES CEGUIOgreRenderer-0)
find_library(CEGUI_LIB_DBG NAMES CEGUIBase-0_d libCEGUIBase-0_d)

mark_as_advanced(CEGUI_H_PATH CEGUI_LIB CEGUI_OGRE_RENDERER_LIB CEGUI_LIB_DBG)

find_package_handle_standard_args(CEGUI DEFAULT_MSG CEGUI_LIB CEGUI_H_PATH)
find_package_handle_standard_args(CEGUI DEFAULT_MSG CEGUI_OGRE_RENDERER_LIB CEGUI_H_PATH)


# set up output vars
if (CEGUI_FOUND)
    set (CEGUI_INCLUDE_DIR ${CEGUI_H_PATH})
    set (CEGUI_LIBRARIES ${CEGUI_LIB} ${CEGUI_OGRE_RENDERER_LIB})
    if (CEGUI_LIB_DBG)
        set (CEGUI_LIBRARIES_DBG ${CEGUI_LIB_DBG})
    endif()
else()
    set (CEGUI_INCLUDE_DIR)
    set (CEGUI_LIBRARIES)
    set (CEGUI_LIBRARIES_DBG)
endif()

