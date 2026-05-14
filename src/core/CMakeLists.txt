target_sources(linscreen PRIVATE
    linscreen.h
    linscreendaemon.h
    qguiappcurrentscreen.h
)

target_sources(linscreen PRIVATE
    capturerequest.cpp
    linscreen.cpp
    linscreendaemon.cpp
    qguiappcurrentscreen.cpp
)

if (UNIX AND NOT APPLE)
target_sources(linscreen PRIVATE
    linscreendbusadapter.h
    linscreendbusadapter.cpp
)
endif ()

if (USE_KDSINGLEAPPLICATION)
if (NOT WIN32)
target_sources(linscreen PRIVATE
    signaldaemon.h
    signaldaemon.cpp
)
endif ()
endif ()

if (WIN32)
    target_sources(linscreen PRIVATE globalshortcutfilter.h globalshortcutfilter.cpp)
endif ()
