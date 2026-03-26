add_executable( vcstream
    main.cpp
    qml/qml.qrc
)

target_link_libraries( vcstream PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    vcstream_warnings
    vcstream_app_defence
    vcstream_app_lifecycle
    vcstream_app_platform
    vcstream_ui_geometry
    vcstream_ui_placement
    vcstream_ui_colour
    vcstream_ui_theme
)

target_compile_definitions( vcstream PRIVATE
    VCSTREAM_VERSION_STRING="${PROJECT_VERSION}"
)
