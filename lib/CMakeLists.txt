set(VLF_LIB_SOURCES location.cc bootstraplist.cc socksservice.cc
    station.cc stationlist.cc query.cc audio.cc schedule.cc receiver.cc datasetfile.cc)
set(VLF_LIB_MOC_HEADERS
    station.hh stationlist.hh query.hh audio.hh schedule.hh receiver.hh datasetfile.hh)
set(VLF_LIB_HEADERS ${VLF_CLIENT_MOC_HEADERS}
    location.hh bootstraplist.hh socksservice.hh)

qt5_wrap_cpp(VLF_LIB_MOC_SOURCES ${VLF_LIB_MOC_HEADERS})

add_library(vlfnet STATIC ${VLF_LIB_SOURCES} ${VLF_LIB_MOC_SOURCES})
set_target_properties(vlfnet PROPERTIES MACOSX_RPATH "${CMAKE_INSTALL_RPATH}")
set_target_properties(vlfnet PROPERTIES INSTALL_NAME_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
target_link_libraries(vlfnet ${LIBS})

# Install binary
INSTALL(TARGETS vlfnet DESTINATION lib)
