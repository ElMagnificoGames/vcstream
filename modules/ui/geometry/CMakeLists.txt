add_library( vcstream_ui_geometry
    windowrectfitter.h
    windowrectfitter.cpp
)

target_link_libraries( vcstream_ui_geometry PRIVATE
    Qt6::Core
    Qt6::Gui
)

target_include_directories( vcstream_ui_geometry PUBLIC
    ${PROJECT_SOURCE_DIR}
)
