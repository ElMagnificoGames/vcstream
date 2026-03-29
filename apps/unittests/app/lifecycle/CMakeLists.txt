add_executable( tst_appsupervisor
    tst_appsupervisor.cpp
)

target_link_libraries( tst_appsupervisor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Test
    vcstream_warnings
    vcstream_app_defence
    vcstream_app_lifecycle
)

add_test( NAME tst_appsupervisor COMMAND tst_appsupervisor )

vcstream_apply_default_test_environment( tst_appsupervisor )
