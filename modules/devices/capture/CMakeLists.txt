add_library( vcstream_devices_capture
    camerabackend.h
    camerastreambackend.h
    nullcamerabackend.h
    nullcamerabackend.cpp
    qtcamerabackend.h
    qtcamerabackend.cpp
    mediacapture.h
    mediacapture.cpp
    camerapreviewhandle.h
    camerapreviewhandle.cpp
)

target_link_libraries( vcstream_devices_capture PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    vcstream_warnings
)

target_include_directories( vcstream_devices_capture PUBLIC
    ${PROJECT_SOURCE_DIR}
)
