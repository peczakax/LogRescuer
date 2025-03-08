# Find Brotli library and include files
#
# BROTLI_INCLUDE_DIRS - where to find brotli/decode.h, etc.
# BROTLI_LIBRARIES - List of libraries to link against when using Brotli
# BROTLI_FOUND - True if Brotli was found

find_path(BROTLI_INCLUDE_DIR
    NAMES brotli/decode.h brotli/encode.h
    PATHS
        ${BROTLI_ROOT}/include
        /usr/local/include
        /usr/include
)

find_library(BROTLICOMMON_LIBRARY
    NAMES brotlicommon
    PATHS
        ${BROTLI_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /usr/lib/x86_64-linux-gnu
)

find_library(BROTLIDEC_LIBRARY
    NAMES brotlidec
    PATHS
        ${BROTLI_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /usr/lib/x86_64-linux-gnu
)

find_library(BROTLIENC_LIBRARY
    NAMES brotlienc
    PATHS
        ${BROTLI_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /usr/lib/x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Brotli
    REQUIRED_VARS 
        BROTLI_INCLUDE_DIR 
        BROTLICOMMON_LIBRARY
        BROTLIDEC_LIBRARY
        BROTLIENC_LIBRARY
)

if(BROTLI_FOUND)
    set(BROTLI_INCLUDE_DIRS ${BROTLI_INCLUDE_DIR})
    set(BROTLI_LIBRARIES 
        ${BROTLIENC_LIBRARY}
        ${BROTLIDEC_LIBRARY}
        ${BROTLICOMMON_LIBRARY}
    )

    if(NOT TARGET brotli::brotlicommon)
        add_library(brotli::brotlicommon UNKNOWN IMPORTED)
        set_target_properties(brotli::brotlicommon PROPERTIES
            IMPORTED_LOCATION "${BROTLICOMMON_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIRS}"
        )
    endif()

    if(NOT TARGET brotli::brotlidec)
        add_library(brotli::brotlidec UNKNOWN IMPORTED)
        set_target_properties(brotli::brotlidec PROPERTIES
            IMPORTED_LOCATION "${BROTLIDEC_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIRS}"
        )
    endif()

    if(NOT TARGET brotli::brotlienc)
        add_library(brotli::brotlienc UNKNOWN IMPORTED)
        set_target_properties(brotli::brotlienc PROPERTIES
            IMPORTED_LOCATION "${BROTLIENC_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${BROTLI_INCLUDE_DIRS}"
        )
    endif()
endif()

mark_as_advanced(
    BROTLI_INCLUDE_DIR
    BROTLICOMMON_LIBRARY
    BROTLIDEC_LIBRARY
    BROTLIENC_LIBRARY
)
