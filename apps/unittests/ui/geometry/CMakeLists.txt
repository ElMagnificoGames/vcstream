add_executable( tst_windowrectfitter
    tst_windowrectfitter.cpp
)

target_link_libraries( tst_windowrectfitter PRIVATE
    Qt6::Core
    Qt6::Test
    vcstream_app_defence
    vcstream_ui_geometry
)

add_test( NAME tst_windowrectfitter COMMAND tst_windowrectfitter )
