add_library( vcstream_devices_catalogue
    localdevicecatalogue.h
    localdevicecatalogue.cpp
)

target_link_libraries( vcstream_devices_catalogue PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    vcstream_warnings
)

target_include_directories( vcstream_devices_catalogue PUBLIC
    ${PROJECT_SOURCE_DIR}
)
