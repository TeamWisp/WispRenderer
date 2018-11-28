file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "*.hpp")

set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)

set_target_properties (gtest PROPERTIES FOLDER ThirdParty)
set_target_properties (gtest_main PROPERTIES FOLDER ThirdParty)
set_target_properties (gmock PROPERTIES FOLDER ThirdParty)
set_target_properties (gmock_main PROPERTIES FOLDER ThirdParty)

if (MSVC)
    target_compile_options(gtest PRIVATE /W0)
endif()

add_executable(UnitTest WIN32 ${HEADERS} ${SOURCES})
target_include_directories(UnitTest PUBLIC deps/googletest/googletest/include)
target_link_libraries(UnitTest gtest)
target_include_directories(UnitTest PUBLIC ../src/)
target_link_libraries(UnitTest WispRenderer)
set_target_properties(UnitTest PROPERTIES CXX_STANDARD 17)
set_target_properties(UnitTest PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(UnitTest PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
