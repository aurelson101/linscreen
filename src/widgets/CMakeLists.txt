add_subdirectory(panel)
add_subdirectory(capture)

# Required to generate MOC
target_sources(
        linscreen
        PRIVATE
        infowindow.ui
        capturelauncher.ui

        capturelauncher.h
        draggablewidgetmaker.h
        imagelabel.h
        trayicon.h
        infowindow.h
        loadspinner.h
        notificationwidget.h
        orientablepushbutton.h

        colorpickerwidget.h
        capture/capturetoolobjects.h
)

if (ENABLE_IMGUR)
target_sources(
    linscreen
    PRIVATE
        uploadhistory.ui
        uploadlineitem.ui
        uploadhistory.h
        uploadlineitem.h
        imguploaddialog.h
)
endif()

target_sources(
        linscreen
        PRIVATE
        capturelauncher.cpp
        draggablewidgetmaker.cpp
        imagelabel.cpp
        trayicon.cpp
        infowindow.cpp
        loadspinner.cpp
        notificationwidget.cpp
        orientablepushbutton.cpp
        colorpickerwidget.cpp
        capture/capturetoolobjects.cpp
)

if (ENABLE_IMGUR)
target_sources(
    linscreen
    PRIVATE
        uploadhistory.cpp
        uploadlineitem.cpp
        imguploaddialog.cpp
)
endif()

if (NOT DISABLE_UPDATE_CHECKER)
    target_sources(
            linscreen
            PRIVATE
            updatenotificationwidget.h
            updatenotificationwidget.cpp
    )
endif()