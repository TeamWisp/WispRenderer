
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "*.hpp")

add_executable(WispDemo WIN32 ${HEADERS} ${SOURCES})
target_include_directories(WispDemo PUBLIC ../src/)
target_link_libraries(WispDemo WispRenderer)
set_target_properties(WispDemo PROPERTIES CXX_STANDARD 17)
set_target_properties(WispDemo PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(WispDemo PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
