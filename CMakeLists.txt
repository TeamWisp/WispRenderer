cmake_minimum_required(VERSION 3.13)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(EnginePrototype)

if (MSVC)
	add_compile_options(/W4)
endif()

##### OUTPUT DIRECTORIES #####
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

##### DEPENDENCIES #####
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" [CONFIGURE_DEPENDS])
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" [CONFIGURE_DEPENDS])
file(GLOB FG_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/fg/*.hpp" [CONFIGURE_DEPENDS])
file(GLOB FG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/fg/*.cpp" [CONFIGURE_DEPENDS])
source_group("High Level API" FILES ${SOURCES} ${HEADERS})
source_group("Framegraph" FILES ${FG_SOURCES} ${FG_HEADERS})

add_executable(EnginePrototype ${HEADERS} ${SOURCES} ${FG_HEADERS} ${FG_SOURCES})
target_link_libraries(EnginePrototype)
set_target_properties(EnginePrototype PROPERTIES CXX_STANDARD 17)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT EnginePrototype)
set_target_properties(EnginePrototype PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
