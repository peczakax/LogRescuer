# Find ZStandard library
find_path(ZSTD_INCLUDE_DIR
    NAMES zstd.h
    PATHS
        ${ZSTD_ROOT}/include
        /usr/local/include
        /usr/include
)

find_library(ZSTD_LIBRARY
    NAMES zstd
    PATHS
        ${ZSTD_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /usr/lib/x86_64-linux-gnu
)

# Set required ZSTD version (1.4.8)
set(ZSTD_REQUIRED_VERSION_MAJOR 1)
set(ZSTD_REQUIRED_VERSION_MINOR 4)
set(ZSTD_REQUIRED_VERSION_PATCH 8)
math(EXPR ZSTD_REQUIRED_VERSION_NUMBER 
     "${ZSTD_REQUIRED_VERSION_MAJOR} * 100 * 100 + ${ZSTD_REQUIRED_VERSION_MINOR} * 100 + ${ZSTD_REQUIRED_VERSION_PATCH}")

# Extract version from header
if(ZSTD_INCLUDE_DIR)
    file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" ZSTD_VERSION_LINE REGEX "^#define ZSTD_VERSION_NUMBER[ \t]+([0-9]+)$")
    
    if(ZSTD_VERSION_LINE)
        string(REGEX REPLACE ".*#define ZSTD_VERSION_NUMBER[ \t]+([0-9]+).*" "\\1" ZSTD_VERSION_NUMBER "${ZSTD_VERSION_LINE}")
    else()
        file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" ZSTD_VERSION_MAJOR_LINE REGEX "^#define ZSTD_VERSION_MAJOR[ \t]+([0-9]+)$")
        file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" ZSTD_VERSION_MINOR_LINE REGEX "^#define ZSTD_VERSION_MINOR[ \t]+([0-9]+)$")
        file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" ZSTD_VERSION_RELEASE_LINE REGEX "^#define ZSTD_VERSION_RELEASE[ \t]+([0-9]+)$")
        
        if(ZSTD_VERSION_MAJOR_LINE AND ZSTD_VERSION_MINOR_LINE AND ZSTD_VERSION_RELEASE_LINE)
            string(REGEX REPLACE ".*#define ZSTD_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" ZSTD_VERSION_MAJOR "${ZSTD_VERSION_MAJOR_LINE}")
            string(REGEX REPLACE ".*#define ZSTD_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" ZSTD_VERSION_MINOR "${ZSTD_VERSION_MINOR_LINE}")
            string(REGEX REPLACE ".*#define ZSTD_VERSION_RELEASE[ \t]+([0-9]+).*" "\\1" ZSTD_VERSION_PATCH "${ZSTD_VERSION_RELEASE_LINE}")
            
            math(EXPR ZSTD_VERSION_NUMBER 
                 "${ZSTD_VERSION_MAJOR} * 100 * 100 + ${ZSTD_VERSION_MINOR} * 100 + ${ZSTD_VERSION_PATCH}")
        endif()
    endif()
    
    if(DEFINED ZSTD_VERSION_NUMBER)
        math(EXPR ZSTD_VERSION_MAJOR "${ZSTD_VERSION_NUMBER} / (100 * 100)")
        math(EXPR ZSTD_VERSION_MINOR "((${ZSTD_VERSION_NUMBER} - ${ZSTD_VERSION_MAJOR} * 100 * 100) / 100)")
        math(EXPR ZSTD_VERSION_PATCH "${ZSTD_VERSION_NUMBER} % 100")
        set(ZSTD_VERSION "${ZSTD_VERSION_MAJOR}.${ZSTD_VERSION_MINOR}.${ZSTD_VERSION_PATCH}")
    endif()
else()
    message(STATUS "ZSTD include directory not found")
endif()

# Check version
set(ZSTD_VERSION_OK TRUE)
if(DEFINED ZSTD_VERSION_NUMBER)
    if(ZSTD_VERSION_NUMBER LESS ZSTD_REQUIRED_VERSION_NUMBER)
        set(ZSTD_VERSION_OK FALSE)
        message(STATUS "ZSTD version ${ZSTD_VERSION} is too old. Required is at least ${ZSTD_REQUIRED_VERSION_MAJOR}.${ZSTD_REQUIRED_VERSION_MINOR}.${ZSTD_REQUIRED_VERSION_PATCH}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZSTD
    REQUIRED_VARS 
        ZSTD_INCLUDE_DIR 
        ZSTD_LIBRARY
        ZSTD_VERSION_OK
    VERSION_VAR ZSTD_VERSION
    HANDLE_VERSION_RANGE
)

if(ZSTD_FOUND)
    set(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
    set(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
    # Create the zstd::libzstd_static target for compatibility with Conan
    if(NOT TARGET zstd::libzstd_static)
        add_library(zstd::libzstd_static UNKNOWN IMPORTED)
        set_target_properties(zstd::libzstd_static PROPERTIES
            IMPORTED_LOCATION "${ZSTD_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY)
