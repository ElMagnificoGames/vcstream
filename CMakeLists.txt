cmake_minimum_required( VERSION 3.24 )

project( vcstream VERSION 0.6.0 LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC OFF )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

find_package( Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Multimedia Test )

enable_testing()

add_subdirectory( modules )
add_subdirectory( apps/vcstream )
add_subdirectory( apps/unittests )

option( VCSTREAM_BUILD_INTERNAL_TOOLS "Build internal developer tools" OFF )

if ( VCSTREAM_BUILD_INTERNAL_TOOLS )
    add_subdirectory( apps/fontprobe )
endif()
