set(VLF_CLIENT_SOURCES main.cc
    application.cc mainwindow.cc logwindow.cc stationlistview.cc monitor.cc scheduleview.cc
    newscheduledeventdialog.cc locationeditdialog.cc osmwidget.cc aboutwidget.cc)
set(VLF_CLIENT_MOC_HEADERS
    application.hh mainwindow.hh logwindow.hh stationlistview.hh monitor.hh scheduleview.hh
    newscheduledeventdialog.hh locationeditdialog.hh osmwidget.hh aboutwidget.hh)
set(VLF_CLIENT_HEADERS ${VLF_CLIENT_MOC_HEADERS}
    )

qt5_wrap_cpp(VLF_CLIENT_MOC_SOURCES ${VLF_CLIENT_MOC_HEADERS})
qt5_add_resources(VLF_CLIENT_RCC_SOURCES ../shared/resources.qrc)

add_executable(vlfclient ${VLF_CLIENT_SOURCES} ${VLF_CLIENT_MOC_SOURCES} ${VLF_CLIENT_RCC_SOURCES})
target_link_libraries(vlfclient vlfnet ${LIBS})

# Install binary
INSTALL(TARGETS vlfclient DESTINATION bin)

# Install logo and .desktop file under Linux:
#IF(UNIX AND NOT APPLE)
# INSTALL(FILES ../shared/ovlclient.png
#         DESTINATION share/icons)
# INSTALL(FILES ../shared/ovlclient.desktop
#         DESTINATION share/applications)
#ENDIF(UNIX AND NOT APPLE)

# Create "Application Bundle" under MacOS X
#if(UNIX AND APPLE)
# install(TARGETS ovlclient DESTINATION /Applications/OvlClient.app/Contents/MacOS)
# install(FILES ../shared/macos/ovlclient.icns DESTINATION /Applications/OvlClient.app/Contents)
# install(FILES ../shared/macos/Info.plist DESTINATION /Applications/OvlClient.app/Contents)
#endif(UNIX AND APPLE)
