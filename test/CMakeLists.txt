file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "*.hpp")


set_target_properties (gtest PROPERTIES FOLDER ThirdParty)
set_target_properties (gtest_main PROPERTIES FOLDER ThirdParty)

if (MSVC)
    target_compile_options(gtest PRIVATE /W0)
endif()

add_executable(WispTest WIN32 ${HEADERS} ${SOURCES})
target_include_directories(WispTest PUBLIC ../deps/googletest/googletest/include)
target_link_libraries(WispTest gtest)
target_include_directories(WispTest PUBLIC ../src/)
target_link_libraries(WispTest WispRenderer)
set_target_properties(WispTest PROPERTIES CXX_STANDARD 17)
set_target_properties(WispTest PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(WispTest PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(WispTest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set (CMAKE_EXE_LINKER_FLAGS "$(CMAKE_EXE_LINKER_FLAGS) /SUBSYSTEM:CONSOLE")
