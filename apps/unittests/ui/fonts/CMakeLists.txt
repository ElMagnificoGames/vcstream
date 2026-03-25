add_executable( tst_fontpreviewsafetycache
    tst_fontpreviewsafetycache.cpp
)

target_link_libraries( tst_fontpreviewsafetycache PRIVATE
    Qt6::Core
    Qt6::Test
    vcstream_app_defence
    vcstream_ui_fonts
)

add_test( NAME tst_fontpreviewsafetycache COMMAND tst_fontpreviewsafetycache )
