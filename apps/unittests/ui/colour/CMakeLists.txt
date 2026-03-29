add_executable( tst_oklchcolour
    tst_oklchcolour.cpp
)

target_link_libraries( tst_oklchcolour PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    vcstream_warnings
    vcstream_app_defence
    vcstream_ui_colour
)

add_test( NAME tst_oklchcolour COMMAND tst_oklchcolour )

vcstream_apply_default_test_environment( tst_oklchcolour )
