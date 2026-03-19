add_library( vcstream_ui_theme
    accentimageprovider.h
    accentimageprovider.cpp
)

target_link_libraries( vcstream_ui_theme PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    vcstream_ui_colour
)

target_include_directories( vcstream_ui_theme PUBLIC
    ${PROJECT_SOURCE_DIR}
)
