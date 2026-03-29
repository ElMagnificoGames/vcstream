add_executable( tst_mediacapture
    tst_mediacapture.cpp
)

target_link_libraries( tst_mediacapture PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Test
    vcstream_warnings
    vcstream_devices_capture
)

add_test( NAME tst_mediacapture COMMAND tst_mediacapture )

vcstream_apply_default_test_environment( tst_mediacapture )
