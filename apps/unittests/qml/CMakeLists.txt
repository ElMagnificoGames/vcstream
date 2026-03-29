add_executable( tst_qml_ui
    tst_qml_ui.cpp
    ${PROJECT_SOURCE_DIR}/apps/vcstream/qml/qml.qrc
)

target_link_libraries( tst_qml_ui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Test
    vcstream_warnings
    vcstream_app_defence
    vcstream_app_lifecycle
    vcstream_platform_shim_qtstartupshim
    vcstream_ui_placement
    vcstream_ui_colour
    vcstream_ui_theme
)

add_test( NAME tst_qml_ui COMMAND tst_qml_ui )

vcstream_apply_default_test_environment( tst_qml_ui )

set_tests_properties( tst_qml_ui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
