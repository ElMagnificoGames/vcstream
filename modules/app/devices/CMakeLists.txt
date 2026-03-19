add_library( vcstream_app_devices
    localdevicecatalogue.h
    localdevicecatalogue.cpp
)

target_link_libraries( vcstream_app_devices PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
)

target_include_directories( vcstream_app_devices PUBLIC
    ${PROJECT_SOURCE_DIR}
)
