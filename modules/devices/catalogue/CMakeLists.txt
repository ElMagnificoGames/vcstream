add_library( vcstream_devices_catalogue
    devicememorycache.h
    devicememorycache.cpp
    fakedevicenamegenerator.h
    fakedevicenamegenerator.cpp
    localdevicecatalogue.h
    localdevicecatalogue.cpp
    windowcapturewatcher.h
    windowcapturewatcher.cpp
)

target_link_libraries( vcstream_devices_catalogue PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    vcstream_warnings
    vcstream_platform_shim_qtmultimediarescan
    vcstream_platform_shim_dshowcameraenum
)

target_include_directories( vcstream_devices_catalogue PUBLIC
    ${PROJECT_SOURCE_DIR}
)
