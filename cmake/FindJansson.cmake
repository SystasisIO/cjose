# FindJansson.cmake
# ----------------
# Locate the Jansson JSON library (https://github.com/akheron/jansson).
#
# This first tries the package's own CMake config (janssonConfig.cmake) and,
# failing that, falls back to pkg-config and a manual search. A requested
# version (find_package(Jansson <ver>)) is enforced for either discovery path.
#
# Result variables:
#   Jansson_FOUND        - True if Jansson was found
#   Jansson_INCLUDE_DIRS - Include directories for Jansson
#   Jansson_LIBRARIES    - Libraries to link against Jansson
#   Jansson_VERSION      - Version of Jansson, if determinable
#
# Imported target:
#   Jansson::Jansson

include(FindPackageHandleStandardArgs)

set(Jansson_VERSION "")
set(_jansson_config_target FALSE)
set(_jansson_include_dirs "")

# 1) Prefer the upstream CMake package config, if present.
find_package(jansson CONFIG QUIET)
if(jansson_FOUND AND TARGET jansson::jansson)
    if(NOT TARGET Jansson::Jansson)
        add_library(Jansson::Jansson INTERFACE IMPORTED)
        set_target_properties(Jansson::Jansson PROPERTIES
            INTERFACE_LINK_LIBRARIES jansson::jansson)
    endif()
    set(_jansson_config_target TRUE)
    get_target_property(_jansson_include_dirs jansson::jansson INTERFACE_INCLUDE_DIRECTORIES)
else()
    # 2) Fall back to pkg-config + manual discovery.
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PC_JANSSON QUIET jansson)
    endif()

    find_path(Jansson_INCLUDE_DIR
        NAMES jansson.h
        HINTS ${PC_JANSSON_INCLUDEDIR} ${PC_JANSSON_INCLUDE_DIRS})

    find_library(Jansson_LIBRARY
        NAMES jansson libjansson
        HINTS ${PC_JANSSON_LIBDIR} ${PC_JANSSON_LIBRARY_DIRS})

    set(_jansson_include_dirs "${Jansson_INCLUDE_DIR}")

    if(Jansson_LIBRARY AND Jansson_INCLUDE_DIR)
        set(Jansson_LIBRARIES ${Jansson_LIBRARY})
        set(Jansson_INCLUDE_DIRS ${Jansson_INCLUDE_DIR})
        if(NOT TARGET Jansson::Jansson)
            add_library(Jansson::Jansson UNKNOWN IMPORTED)
            set_target_properties(Jansson::Jansson PROPERTIES
                IMPORTED_LOCATION "${Jansson_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${Jansson_INCLUDE_DIR}")
        endif()
    endif()

    mark_as_advanced(Jansson_INCLUDE_DIR Jansson_LIBRARY)
endif()

# Some upstream configs report the library ABI version, not the release version.
# Read only headers belonging to the selected target/discovery path; do not
# search another installation to assign a version to a config-package target.
foreach(_jansson_include_dir IN LISTS _jansson_include_dirs)
    if(IS_ABSOLUTE "${_jansson_include_dir}" AND EXISTS "${_jansson_include_dir}/jansson.h")
        file(STRINGS "${_jansson_include_dir}/jansson.h" _jansson_version_line
            LIMIT_COUNT 1 REGEX "^[ \t]*#[ \t]*define[ \t]+JANSSON_VERSION[ \t]+\"[^\"]+\"")
        if(_jansson_version_line)
            string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" Jansson_VERSION "${_jansson_version_line}")
            break()
        endif()
    endif()
endforeach()
if(NOT Jansson_VERSION AND NOT _jansson_config_target AND PC_JANSSON_VERSION)
    set(Jansson_VERSION "${PC_JANSSON_VERSION}")
endif()
unset(_jansson_config_target)
unset(_jansson_include_dirs)
unset(_jansson_include_dir)
unset(_jansson_version_line)

# Single validation point so the requested version is checked for both paths.
if(TARGET Jansson::Jansson)
    set(_Jansson_TARGET Jansson::Jansson)
endif()
find_package_handle_standard_args(Jansson
    REQUIRED_VARS _Jansson_TARGET
    VERSION_VAR Jansson_VERSION)
unset(_Jansson_TARGET)
