add_library( vcstream_ui_placement
    windowplacement.h
    windowplacement.cpp
)

target_link_libraries( vcstream_ui_placement PRIVATE
    Qt6::Core
    Qt6::Gui
    vcstream_warnings
    vcstream_ui_geometry
)

target_include_directories( vcstream_ui_placement PUBLIC
    ${PROJECT_SOURCE_DIR}
)
