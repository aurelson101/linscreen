# Required to generate MOC
target_sources(
  linscreen
  PRIVATE abstractlogger.h
          filenamehandler.h
          screengrabber.h
          systemnotification.h
          valuehandler.h
          strfparse.h
)

target_sources(
  linscreen
  PRIVATE abstractlogger.cpp
          filenamehandler.cpp
          screengrabber.cpp
          monitorpreview.cpp
          confighandler.cpp
          systemnotification.cpp
          valuehandler.cpp
          screenshotsaver.cpp
          globalvalues.cpp
          desktopfileparse.cpp
          desktopinfo.cpp
          waylandutils.cpp
          pathinfo.cpp
          colorutils.cpp
          history.cpp
          strfparse.cpp
)

IF (UNIX AND NOT APPLE)
target_sources(
  linscreen
  PRIVATE request.h
          request.cpp
)
ENDIF()

IF (WIN32)
  target_sources(
    linscreen
    PRIVATE winlnkfileparse.cpp
  )
ENDIF()
