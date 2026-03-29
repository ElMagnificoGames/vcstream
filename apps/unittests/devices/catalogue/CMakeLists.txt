add_executable( tst_devicememorycache
    tst_devicememorycache.cpp
)

target_link_libraries( tst_devicememorycache PRIVATE
    Qt6::Core
    Qt6::Test
    vcstream_warnings
    vcstream_devices_catalogue
)

add_test( NAME tst_devicememorycache COMMAND tst_devicememorycache )

vcstream_apply_default_test_environment( tst_devicememorycache )
