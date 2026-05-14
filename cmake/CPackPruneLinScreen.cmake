set(_linscreen_prune_roots)

foreach(_candidate IN ITEMS
    "${CPACK_TEMPORARY_DIRECTORY}"
    "${CMAKE_INSTALL_PREFIX}")
  if(_candidate AND IS_DIRECTORY "${_candidate}")
    list(APPEND _linscreen_prune_roots "${_candidate}")
  endif()
endforeach()

if(CPACK_TEMPORARY_DIRECTORY AND IS_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}")
  file(GLOB _linscreen_package_roots
       LIST_DIRECTORIES true
       "${CPACK_TEMPORARY_DIRECTORY}/*")
  foreach(_package_root IN LISTS _linscreen_package_roots)
    if(IS_DIRECTORY "${_package_root}")
      list(APPEND _linscreen_prune_roots "${_package_root}")
    endif()
  endforeach()
endif()

list(REMOVE_DUPLICATES _linscreen_prune_roots)

foreach(_root IN LISTS _linscreen_prune_roots)
  if(_root STREQUAL "/" OR _root STREQUAL "/usr")
    continue()
  endif()

  file(REMOVE_RECURSE
       "${_root}/usr/include"
       "${_root}/usr/lib"
       "${_root}/usr/lib64"
       "${_root}/include"
       "${_root}/lib"
       "${_root}/lib64")
endforeach()
