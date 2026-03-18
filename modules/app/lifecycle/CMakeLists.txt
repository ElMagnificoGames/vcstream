add_library( vcstream_app_lifecycle
    appsupervisor.h
    appsupervisor.cpp
)

target_link_libraries( vcstream_app_lifecycle PRIVATE
    Qt6::Core
)

target_include_directories( vcstream_app_lifecycle PUBLIC
    ${PROJECT_SOURCE_DIR}
)
