add_executable( tst_crashguard
    tst_crashguard.cpp
)

target_link_libraries( tst_crashguard PRIVATE
    Qt6::Core
    Qt6::Test
    vcstream_warnings
    vcstream_app_defence
)

add_test( NAME tst_crashguard COMMAND tst_crashguard )
