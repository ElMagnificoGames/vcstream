add_library( vcstream_ui_colour
    oklchcolour.h
    oklchcolour.cpp
    oklchutil.h
    oklchutil.cpp
)

target_link_libraries( vcstream_ui_colour PRIVATE
    Qt6::Core
    Qt6::Gui
)

target_include_directories( vcstream_ui_colour PUBLIC
    ${PROJECT_SOURCE_DIR}
)
