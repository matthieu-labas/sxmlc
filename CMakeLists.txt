cmake_minimum_required(VERSION 3.13)

option(BUILD_SHARED_LIBS "Build sxmlc as shared library instead of static" ON)

add_library(sxmlc src/sxmlc.c src/sxmlsearch.c)
target_include_directories(sxmlc PUBLIC src/)

if (BUILD_SHARED_LIBS)
    set_target_properties (sxmlc PROPERTIES VERSION 4.5.1 SOVERSION 4)
    include(GNUInstallDirs)
    install(TARGETS sxmlc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    # manually install headder files
    install(FILES src/sxmlc.h src/sxmlsearch.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
