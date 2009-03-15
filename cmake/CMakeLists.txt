compiz_configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/CompizDefaults.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CompizDefaults.cmake
)

set (_files
    CompizCommon.cmake
    CompizPlugin.cmake
    CompizPackage.cmake
    CompizBcop.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CompizDefaults.cmake
)

if (USE_GCONF)
    list (APPEND _files CompizGconf.cmake)
endif ()

install (
    FILES ${_files}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/compiz/cmake
)