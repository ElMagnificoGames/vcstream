cmake_minimum_required( VERSION 3.24 )

project( vcstream VERSION 0.9.2 LANGUAGES CXX )

# Single-config generators (Ninja/Makefiles) default to an empty CMAKE_BUILD_TYPE.
# For VCStream we want Debug by default so local builds include symbols, and so
# debug-only tooling (such as sanitisers) is enabled without extra flags.
get_property( VCSTREAM_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG )
if ( NOT VCSTREAM_IS_MULTI_CONFIG )
    if ( NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "" )
        set( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE )
        set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel" )
    endif()
endif()

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC OFF )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

add_library( vcstream_warnings INTERFACE )

# Sanitiser support.
# Enabled by default for Debug builds, disabled for Release builds.
option( VCSTREAM_ENABLE_SANITIZERS "Enable sanitizers for Debug builds" ON )
add_library( vcstream_sanitizers INTERFACE )

if ( VCSTREAM_ENABLE_SANITIZERS )
    include( CheckCXXCompilerFlag )

    if ( MSVC )
        check_cxx_compiler_flag( "/fsanitize=address" VCSTREAM_HAS_MSVC_ASAN )
        if ( VCSTREAM_HAS_MSVC_ASAN )
            target_compile_options( vcstream_sanitizers INTERFACE "$<$<CONFIG:Debug>:/fsanitize=address>" )

            # AddressSanitizer is enabled via the compiler flag. Do not pass
            # `/fsanitize=address` to `link.exe` (it is not a linker option).
            # Also disable incremental linking in ASAN builds to avoid
            # linker warnings about incompatible metadata.
            target_link_options( vcstream_sanitizers INTERFACE "$<$<CONFIG:Debug>:/INCREMENTAL:NO>" )
        endif()
    else()
        check_cxx_compiler_flag( "-fsanitize=address" VCSTREAM_HAS_ASAN )
        check_cxx_compiler_flag( "-fsanitize=undefined" VCSTREAM_HAS_UBSAN )
        check_cxx_compiler_flag( "-fno-omit-frame-pointer" VCSTREAM_HAS_FNO_OMIT_FP )

        set( VCSTREAM_SAN_LIST "" )
        if ( VCSTREAM_HAS_ASAN )
            list( APPEND VCSTREAM_SAN_LIST "address" )
        endif()
        if ( VCSTREAM_HAS_UBSAN )
            list( APPEND VCSTREAM_SAN_LIST "undefined" )
        endif()

        if ( VCSTREAM_SAN_LIST )
            list( JOIN VCSTREAM_SAN_LIST "," VCSTREAM_SAN_FLAGS )
            target_compile_options( vcstream_sanitizers INTERFACE "$<$<CONFIG:Debug>:-fsanitize=${VCSTREAM_SAN_FLAGS}>" )
            target_link_options( vcstream_sanitizers INTERFACE "$<$<CONFIG:Debug>:-fsanitize=${VCSTREAM_SAN_FLAGS}>" )

            if ( VCSTREAM_HAS_FNO_OMIT_FP )
                target_compile_options( vcstream_sanitizers INTERFACE "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>" )
            endif()
        endif()
    endif()
endif()

target_link_libraries( vcstream_warnings INTERFACE vcstream_sanitizers )

if ( MSVC )
    target_compile_options( vcstream_warnings INTERFACE
        /W4
        /WX
    )
else()
    target_compile_options( vcstream_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Werror
    )
endif()

find_package( Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Multimedia Test )

enable_testing()

function( vcstream_apply_default_test_environment testName )
    # Ensure tests can run from a plain shell environment on Windows.
    # In particular, Debug builds may depend on Qt debug DLLs, the MSVC debug CRT,
    # and (when sanitisers are enabled) the ASAN runtime DLL.
    if ( WIN32 )
        # Add the Qt bin directory (Qt6Cored.dll, etc.) to PATH.
        if ( DEFINED Qt6_DIR )
            cmake_path( GET Qt6_DIR PARENT_PATH _vcstream_qt6_cmake_dir )
            cmake_path( GET _vcstream_qt6_cmake_dir PARENT_PATH _vcstream_qt6_lib_dir )
            cmake_path( GET _vcstream_qt6_lib_dir PARENT_PATH _vcstream_qt6_prefix_dir )
            set( _vcstream_qt6_bin_dir "${_vcstream_qt6_prefix_dir}/bin" )
            if ( EXISTS "${_vcstream_qt6_bin_dir}" )
                set_property( TEST ${testName} APPEND PROPERTY
                    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_vcstream_qt6_bin_dir}" )
            endif()
        endif()

        if ( MSVC )
            # Add the compiler bin directory; this is where MSVC places
            # clang_rt.asan_dynamic-x86_64.dll for /fsanitize=address.
            get_filename_component( _vcstream_msvc_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY )
            if ( EXISTS "${_vcstream_msvc_bin_dir}/clang_rt.asan_dynamic-x86_64.dll" )
                set_property( TEST ${testName} APPEND PROPERTY
                    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_vcstream_msvc_bin_dir}" )
            endif()

            # Add the MSVC debug CRT directory (MSVCP140D.dll, etc.) if present.
            cmake_path( GET _vcstream_msvc_bin_dir PARENT_PATH _vcstream_msvc_arch_dir )
            cmake_path( GET _vcstream_msvc_arch_dir PARENT_PATH _vcstream_msvc_host_dir )
            cmake_path( GET _vcstream_msvc_host_dir PARENT_PATH _vcstream_msvc_bin_root_dir )
            cmake_path( GET _vcstream_msvc_bin_root_dir PARENT_PATH _vcstream_msvc_version_dir )
            cmake_path( GET _vcstream_msvc_version_dir PARENT_PATH _vcstream_msvc_root_dir )
            cmake_path( GET _vcstream_msvc_root_dir PARENT_PATH _vcstream_vc_tools_dir )
            cmake_path( GET _vcstream_vc_tools_dir PARENT_PATH _vcstream_vc_dir )

            get_filename_component( _vcstream_msvc_version "${_vcstream_msvc_version_dir}" NAME )
            set( _vcstream_debug_crt_dir
                "${_vcstream_vc_dir}/Redist/MSVC/${_vcstream_msvc_version}/debug_nonredist/x64/Microsoft.VC143.DebugCRT" )
            if ( EXISTS "${_vcstream_debug_crt_dir}/MSVCP140D.dll" )
                set_property( TEST ${testName} APPEND PROPERTY
                    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_vcstream_debug_crt_dir}" )
            endif()
        endif()
    endif()

    if ( VCSTREAM_ENABLE_SANITIZERS )
        # Keep the options cross-platform and low-assumption.
        set_property( TEST ${testName} APPEND PROPERTY ENVIRONMENT "ASAN_OPTIONS=abort_on_error=1:detect_leaks=0" )
        set_property( TEST ${testName} APPEND PROPERTY ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1" )
    endif()
endfunction()

add_subdirectory( modules )
add_subdirectory( apps/vcstream )
add_subdirectory( apps/unittests )

option( VCSTREAM_BUILD_INTERNAL_TOOLS "Build internal developer tools" OFF )

if ( VCSTREAM_BUILD_INTERNAL_TOOLS )
    add_subdirectory( apps/fontprobe )
    add_subdirectory( apps/cameraprobe )
endif()
