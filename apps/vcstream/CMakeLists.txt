add_executable( vcstream
    main.cpp
    qml/qml.qrc
)

target_link_libraries( vcstream PRIVATE
    Qt6::Quick
)

if ( MSVC )
    target_compile_options( vcstream PRIVATE /W4 /WX )
else()
    target_compile_options( vcstream PRIVATE -Wall -Wextra -Wpedantic -Werror )
endif()
