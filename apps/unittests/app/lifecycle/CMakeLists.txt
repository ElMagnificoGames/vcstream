add_executable( tst_appsupervisor
    tst_appsupervisor.cpp
)

target_link_libraries( tst_appsupervisor PRIVATE
    Qt6::Core
    Qt6::Test
    vcstream_app_defence
    vcstream_app_lifecycle
)

add_test( NAME tst_appsupervisor COMMAND tst_appsupervisor )
