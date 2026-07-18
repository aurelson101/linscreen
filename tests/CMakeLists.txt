add_test(NAME cli_version COMMAND $<TARGET_FILE:linscreen> --version)
set_tests_properties(cli_version PROPERTIES
  PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

add_test(NAME cli_help COMMAND $<TARGET_FILE:linscreen> --help)
set_tests_properties(cli_help PROPERTIES
  PASS_REGULAR_EXPRESSION "Usage:.*linscreen")

add_test(
  NAME build_artifacts
  COMMAND ${CMAKE_COMMAND}
    -DLINSCREEN_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DLINSCREEN_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify-artifacts.cmake)
