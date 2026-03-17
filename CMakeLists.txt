cmake_minimum_required( VERSION 3.24 )

project( vcstream VERSION 0.0.2 LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( CMAKE_AUTOMOC ON )
set( CMAKE_AUTORCC ON )
set( CMAKE_AUTOUIC OFF )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

find_package( Qt6 REQUIRED COMPONENTS Quick )

add_subdirectory( apps/vcstream )
