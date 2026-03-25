add_library( vcstream_ui_fonts
    bundledfonts.qrc
    bundledfonts.h
    bundledfonts.cpp
    fontpreviewsafetychecker.h
    fontpreviewsafetychecker.cpp
    fontpreviewsafetycheckerdefault.h
    fontpreviewsafetycheckerdefault.cpp
    fontpreviewsafetycache.h
    fontpreviewsafetycache.cpp
)

target_link_libraries( vcstream_ui_fonts PRIVATE
    Qt6::Core
    Qt6::Gui
)

target_include_directories( vcstream_ui_fonts PUBLIC
    ${PROJECT_SOURCE_DIR}
)
