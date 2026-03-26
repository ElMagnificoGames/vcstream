add_library( vcstream_app_defence
    crashguard.h
    crashguard.cpp
)

target_link_libraries( vcstream_app_defence PRIVATE
    Qt6::Core
    vcstream_warnings
)

target_include_directories( vcstream_app_defence PUBLIC
    ${PROJECT_SOURCE_DIR}
)
