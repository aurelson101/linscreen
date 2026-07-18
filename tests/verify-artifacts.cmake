set(_required_files
  "${LINSCREEN_BUILD_DIR}/src/translations/Internationalization_fr.qm"
  "${LINSCREEN_BUILD_DIR}/src/share/applications/org.linscreen.LinScreen.desktop"
  "${LINSCREEN_BUILD_DIR}/src/share/metainfo/org.linscreen.LinScreen.metainfo.xml"
  "${LINSCREEN_BUILD_DIR}/src/share/dbus-1/services/org.linscreen.LinScreen.service")

foreach(_file IN LISTS _required_files)
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "Required build artifact is missing: ${_file}")
  endif()
  file(SIZE "${_file}" _size)
  if(_size EQUAL 0)
    message(FATAL_ERROR "Required build artifact is empty: ${_file}")
  endif()
endforeach()

file(READ
  "${LINSCREEN_BUILD_DIR}/src/share/applications/org.linscreen.LinScreen.desktop"
  _desktop)
if(NOT _desktop MATCHES "Icon=org\\.linscreen\\.LinScreen")
  message(FATAL_ERROR "Desktop icon ID is inconsistent")
endif()

file(READ
  "${LINSCREEN_BUILD_DIR}/src/share/dbus-1/services/org.linscreen.LinScreen.service"
  _dbus_service)
if(NOT _dbus_service MATCHES "Name=org\\.linscreen\\.LinScreen")
  message(FATAL_ERROR "D-Bus service ID is inconsistent")
endif()
