add_test(NAME cli_version COMMAND $<TARGET_FILE:linscreen> --version)
set_tests_properties(cli_version PROPERTIES
  PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

add_test(NAME cli_help COMMAND $<TARGET_FILE:linscreen> --help)
set_tests_properties(cli_help PROPERTIES
  PASS_REGULAR_EXPRESSION "Usage:.*linscreen")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_executable(linscreen_redaction_test
  redaction_test.cpp
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

add_test(
  NAME build_artifacts
  COMMAND ${CMAKE_COMMAND}
    -DLINSCREEN_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DLINSCREEN_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify-artifacts.cmake)
