add_library( vcstream_app_lifecycle
    appsupervisor.h
    appsupervisor.cpp
)

target_link_libraries( vcstream_app_lifecycle PRIVATE
    Qt6::Core
    Qt6::Gui
    vcstream_warnings
    vcstream_app_settings
    vcstream_devices_catalogue
    vcstream_devices_capture
)

target_link_libraries( vcstream_app_lifecycle PUBLIC
    vcstream_ui_fonts
)

target_include_directories( vcstream_app_lifecycle PUBLIC
    ${PROJECT_SOURCE_DIR}
)
