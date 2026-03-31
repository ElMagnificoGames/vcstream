add_library( vcstream_ui_video
    vcvideopaintitem.cpp
    vcvideopaintitem.h
    vcvideopaintitem-dd.txt
)

target_link_libraries( vcstream_ui_video PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Multimedia
    vcstream_warnings
)

target_include_directories( vcstream_ui_video PUBLIC
    ${PROJECT_SOURCE_DIR}
)
