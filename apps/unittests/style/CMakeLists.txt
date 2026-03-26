add_executable( tst_style
    tst_style.cpp
)

target_link_libraries( tst_style PRIVATE
    Qt6::Core
    Qt6::Test
    vcstream_warnings
    vcstream_app_defence
)

target_compile_definitions( tst_style PRIVATE
    VCSTREAM_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

add_test( NAME tst_style COMMAND tst_style )
