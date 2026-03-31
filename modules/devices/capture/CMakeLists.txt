add_library( vcstream_devices_capture
    camerabackend.h
    camerastreambackend.h
    dshowcamerabackend.h
    nullcamerabackend.h
    nullcamerabackend.cpp
    qtcamerabackend.h
    qtcamerabackend.cpp
    routingcamerabackend.h
    routingcamerabackend.cpp
    mediacapture.h
    mediacapture.cpp
    camerapreviewhandle.h
    camerapreviewhandle.cpp
)

if ( WIN32 )
    target_sources( vcstream_devices_capture PRIVATE
        dshowcamerabackend-win32.cpp
    )

    target_link_libraries( vcstream_devices_capture PRIVATE
        ole32
        oleaut32
        strmiids
    )
else()
    target_sources( vcstream_devices_capture PRIVATE
        dshowcamerabackend-other.cpp
    )
endif()

target_link_libraries( vcstream_devices_capture PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    vcstream_warnings
    vcstream_platform_shim_dshowcameraenum
)

target_include_directories( vcstream_devices_capture PUBLIC
    ${PROJECT_SOURCE_DIR}
)
