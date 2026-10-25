add_test(NAME cli_version COMMAND $<TARGET_FILE:linscreen> --version)
set_tests_properties(cli_version PROPERTIES
  PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

add_test(NAME cli_help COMMAND $<TARGET_FILE:linscreen> --help)
set_tests_properties(cli_help PROPERTIES
  PASS_REGULAR_EXPRESSION "Usage:.*linscreen")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_executable(linscreen_tools_menu_test
  capturetoolsmenu_test.cpp
  ../src/widgets/capture/capturetoolsmenu.cpp
  ../src/widgets/capture/capturetoolsmenu.h)
set_target_properties(linscreen_tools_menu_test PROPERTIES AUTOMOC ON)
target_include_directories(linscreen_tools_menu_test PRIVATE ../src)
target_compile_features(linscreen_tools_menu_test PRIVATE cxx_std_20)
target_link_libraries(linscreen_tools_menu_test PRIVATE Qt6::Widgets)
add_test(NAME capture_tools_menu COMMAND linscreen_tools_menu_test)
set_tests_properties(capture_tools_menu PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 10)

add_executable(linscreen_buttonhandler_test
  buttonhandler_test.cpp
  ../src/widgets/capture/buttonhandler.cpp
  ../src/widgets/capture/buttonhandler.h
  ../src/widgets/capture/capturetoolbutton.h
  ../src/widgets/capture/capturebutton.cpp
  ../src/widgets/capture/capturebutton.h
  ../src/utils/colorutils.cpp
  ../src/utils/globalvalues.cpp)
set_target_properties(linscreen_buttonhandler_test PROPERTIES AUTOMOC ON)
target_include_directories(linscreen_buttonhandler_test PRIVATE ../src)
target_compile_features(linscreen_buttonhandler_test PRIVATE cxx_std_20)
target_compile_definitions(linscreen_buttonhandler_test PRIVATE
  APP_VERSION="v${PROJECT_VERSION}" LINSCREEN_GIT_HASH="test")
target_link_libraries(linscreen_buttonhandler_test PRIVATE Qt6::Widgets)
add_test(NAME capture_toolbar_responsive COMMAND linscreen_buttonhandler_test)
set_tests_properties(capture_toolbar_responsive PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)

add_executable(linscreen_ocr_test
  localocr_test.cpp ../src/utils/localocr.cpp ../src/utils/localocr.h
  ../src/widgets/ocrdialog.cpp ../src/widgets/ocrdialog.h)
set_target_properties(linscreen_ocr_test PROPERTIES AUTOMOC ON)
target_include_directories(linscreen_ocr_test PRIVATE ../src)
target_compile_features(linscreen_ocr_test PRIVATE cxx_std_20)
target_link_libraries(linscreen_ocr_test PRIVATE Qt6::Widgets)
add_test(NAME local_ocr_safety COMMAND linscreen_ocr_test)
set_tests_properties(local_ocr_safety PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
add_test(NAME local_ocr_dialog COMMAND linscreen_ocr_test --dialog)
set_tests_properties(local_ocr_dialog PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)
find_program(TESSERACT_EXECUTABLE tesseract)
if(TESSERACT_EXECUTABLE)
  add_test(NAME local_ocr_real COMMAND linscreen_ocr_test --real)
  set_tests_properties(local_ocr_real PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 10)
endif()

add_executable(linscreen_redaction_test
  redaction_test.cpp
  request_fixture.cpp
  ../src/tools/redact/redacttool.cpp
  ../src/tools/redact/redacttool.h
  ../src/tools/abstracttwopointtool.cpp
  ../src/tools/abstracttwopointtool.h
  ../src/tools/capturetool.h
  ../src/utils/colorutils.cpp
  ../src/utils/pathinfo.cpp)
set_target_properties(linscreen_redaction_test PROPERTIES AUTOMOC ON)
target_include_directories(linscreen_redaction_test PRIVATE ../src)
target_compile_features(linscreen_redaction_test PRIVATE cxx_std_20)
target_compile_definitions(linscreen_redaction_test PRIVATE APP_PREFIX="/usr")
target_link_libraries(linscreen_redaction_test PRIVATE Qt6::Widgets)
add_test(NAME opaque_redaction COMMAND linscreen_redaction_test)
set_tests_properties(opaque_redaction PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(linscreen_diagnostics_test
  diagnostics_test.cpp
  ../src/utils/diagnostics.cpp
  ../src/utils/desktopinfo.cpp
  ../src/utils/globalvalues.cpp)
target_include_directories(linscreen_diagnostics_test PRIVATE ../src)
target_compile_features(linscreen_diagnostics_test PRIVATE cxx_std_20)
target_compile_definitions(linscreen_diagnostics_test PRIVATE
  APP_VERSION="v${PROJECT_VERSION}" LINSCREEN_GIT_HASH="test")
target_link_libraries(linscreen_diagnostics_test PRIVATE Qt6::Widgets)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(Qt6 REQUIRED COMPONENTS DBus)
  target_link_libraries(linscreen_diagnostics_test PRIVATE Qt6::DBus)
endif()
add_test(NAME diagnostics_privacy COMMAND linscreen_diagnostics_test)
set_tests_properties(diagnostics_privacy PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(linscreen_infowindow_test
  infowindow_test.cpp
  ../src/widgets/infowindow.cpp ../src/widgets/infowindow.h
  ../src/widgets/infowindow.ui ../src/core/qguiappcurrentscreen.cpp
  ../data/graphics.qrc
  ../src/utils/diagnostics.cpp ../src/utils/desktopinfo.cpp
  ../src/utils/globalvalues.cpp)
set_target_properties(linscreen_infowindow_test PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_include_directories(linscreen_infowindow_test PRIVATE fixtures ../src)
target_compile_features(linscreen_infowindow_test PRIVATE cxx_std_20)
target_compile_definitions(linscreen_infowindow_test PRIVATE
  APP_VERSION="v${PROJECT_VERSION}" LINSCREEN_GIT_HASH="test")
target_link_libraries(linscreen_infowindow_test PRIVATE Qt6::Widgets)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(linscreen_infowindow_test PRIVATE Qt6::DBus)
endif()
add_test(NAME about_responsive COMMAND linscreen_infowindow_test)
set_tests_properties(about_responsive PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=2" TIMEOUT 15)

add_executable(linscreen_launcher_test
  launcher_test.cpp
  request_fixture.cpp
  fixtures/core/linscreen.h
  ../src/widgets/capturelauncher.cpp
  ../src/widgets/capturelauncher.h
  ../src/widgets/capturelauncher.ui
  ../src/core/qguiappcurrentscreen.cpp
  ../src/utils/globalvalues.cpp)
set_target_properties(linscreen_launcher_test PROPERTIES AUTOMOC ON AUTOUIC ON)
target_include_directories(linscreen_launcher_test PRIVATE fixtures ../src)
target_compile_features(linscreen_launcher_test PRIVATE cxx_std_20)
target_compile_definitions(linscreen_launcher_test PRIVATE
  APP_VERSION="v${PROJECT_VERSION}" LINSCREEN_GIT_HASH="test")
target_link_libraries(linscreen_launcher_test PRIVATE Qt6::Widgets)
add_test(NAME launcher_cancellation COMMAND linscreen_launcher_test)
set_tests_properties(launcher_cancellation PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 15)

add_test(
  NAME build_artifacts
  COMMAND ${CMAKE_COMMAND}
    -DLINSCREEN_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DLINSCREEN_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify-artifacts.cmake)
