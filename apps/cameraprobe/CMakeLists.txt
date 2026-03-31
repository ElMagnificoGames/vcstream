add_executable( vcstream_cameraprobe
    main.cpp
)

target_link_libraries( vcstream_cameraprobe PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    vcstream_warnings
    vcstream_devices_capture
    vcstream_platform_shim_dshowcameraenum
    vcstream_platform_shim_qtstartupshim
)

target_include_directories( vcstream_cameraprobe PRIVATE
    ${PROJECT_SOURCE_DIR}
)
