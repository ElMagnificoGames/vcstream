add_executable( vcstream
    main.cpp
    qml/qml.qrc
)

target_link_libraries( vcstream PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    vcstream_app_lifecycle
    vcstream_ui_geometry
    vcstream_ui_placement
)

target_compile_definitions( vcstream PRIVATE
    VCSTREAM_VERSION_STRING="${PROJECT_VERSION}"
)

if ( MSVC )
    target_compile_options( vcstream PRIVATE /W4 /WX )
else()
    target_compile_options( vcstream PRIVATE -Wall -Wextra -Wpedantic -Werror )
endif()
