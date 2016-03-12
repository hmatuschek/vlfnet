set(VLF_DAEMON_SOURCES main.cc
    application.cc)
set(VLF_DAEMON_MOC_HEADERS
    application.hh)
set(VLF_DAEMON_HEADERS ${VLF_CLIENT_MOC_HEADERS}
    )

qt5_wrap_cpp(VLF_DAEMON_MOC_SOURCES ${VLF_DAEMON_MOC_HEADERS})
qt5_add_resources(VLF_DAEMON_RCC_SOURCES ../shared/resources.qrc)

add_executable(vlfdaemon ${VLF_DAEMON_SOURCES} ${VLF_DAEMON_MOC_SOURCES} ${VLF_DAEMON_RCC_SOURCES})
target_link_libraries(vlfdaemon vlfnet ${LIBS})

IF (WITH_SYSTEMD)
 message(STATUS "Install systemd service file.")
 configure_file("../shared/init/vlfdaemon.service.in" "${CMAKE_BINARY_DIR}/vlfdaemon.service")
 install(FILES "${CMAKE_BINARY_DIR}/vlfdaemon.service" DESTINATION "/etc/systemd/system")
ENDIF (WITH_SYSTEMD)

if (WITH_LAUNCHD)
 message(STATUS "Install launchd service file.")
 configure_file("../shared/init/vlfdaemon.plist.in" "${CMAKE_BINARY_DIR}/io.github.hmatuschek.vlfdaemon.plist")
 install(FILES "${CMAKE_BINARY_DIR}/io.github.hmatuschek.vlfdaemon.plist" DESTINATION "/Library/LaunchDaemons")
endif (WITH_LAUNCHD)

# Install binary
INSTALL(TARGETS vlfdaemon DESTINATION bin)
