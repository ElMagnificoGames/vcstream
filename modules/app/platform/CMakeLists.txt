add_library( vcstream_app_platform
    qtshims.h
    qtshims.cpp
)

target_link_libraries( vcstream_app_platform PRIVATE
    Qt6::Core
    vcstream_warnings
)

target_include_directories( vcstream_app_platform PUBLIC
    ${PROJECT_SOURCE_DIR}
)
