add_library( vcstream_platform_shim_qtstartupshim
    qtstartupshim.h
)

if ( WIN32 )
    target_sources( vcstream_platform_shim_qtstartupshim PRIVATE
        qtstartupshim-win32.cpp
    )
else()
    target_sources( vcstream_platform_shim_qtstartupshim PRIVATE
        qtstartupshim-other.cpp
    )
endif()

target_link_libraries( vcstream_platform_shim_qtstartupshim PRIVATE
    Qt6::Core
    vcstream_warnings
)

target_include_directories( vcstream_platform_shim_qtstartupshim PUBLIC
    ${PROJECT_SOURCE_DIR}
)


add_library( vcstream_platform_shim_qtmultimediarescan
    qtmultimediarescan.h
)

if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    target_sources( vcstream_platform_shim_qtmultimediarescan PRIVATE
        qtmultimediarescan-linux.cpp
    )

    find_package( Qt6MultimediaPrivate QUIET )

    if ( Qt6MultimediaPrivate_FOUND AND TARGET Qt6::MultimediaPrivate )
        target_link_libraries( vcstream_platform_shim_qtmultimediarescan PRIVATE
            Qt6::MultimediaPrivate
        )
    endif()

    # Some distro packages ship the headers but do not export the versioned
    # private-include paths via CMake. The implementation contains a fallback
    # absolute-include probe for Debian-style installs.
else()
    target_sources( vcstream_platform_shim_qtmultimediarescan PRIVATE
        qtmultimediarescan-other.cpp
    )
endif()

target_link_libraries( vcstream_platform_shim_qtmultimediarescan PRIVATE
    Qt6::Core
    Qt6::Multimedia
    vcstream_warnings
)

target_include_directories( vcstream_platform_shim_qtmultimediarescan PUBLIC
    ${PROJECT_SOURCE_DIR}
)
