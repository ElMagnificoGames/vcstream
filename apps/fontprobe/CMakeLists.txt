add_executable( vcstream_fontprobe
    main.cpp
)

target_link_libraries( vcstream_fontprobe PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    vcstream_app_defence
)

target_include_directories( vcstream_fontprobe PRIVATE
    ${PROJECT_SOURCE_DIR}
)
