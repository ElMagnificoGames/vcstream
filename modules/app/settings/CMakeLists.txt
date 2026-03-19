add_library( vcstream_app_settings
    apppreferences.h
    apppreferences.cpp
)

target_link_libraries( vcstream_app_settings PRIVATE
    Qt6::Core
)

target_include_directories( vcstream_app_settings PUBLIC
    ${PROJECT_SOURCE_DIR}
)
