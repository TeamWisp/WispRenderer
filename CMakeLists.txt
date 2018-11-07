cmake_minimum_required(VERSION 3.13)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(EnginePrototype)

##### OUTPUT DIRECTORIES #####
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

##### DEPENDENCIES #####
file(GLOB HEADERS [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCES [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB FG_HEADERS [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/frame_graph/*.hpp")
file(GLOB FG_SOURCES [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/frame_graph/*.cpp")
file(GLOB SG_HEADERS [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/scene_graph/*.hpp")
file(GLOB SG_SOURCES [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/scene_graph/*.cpp")
file(GLOB RT_HEADERS [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/render_tasks/*.hpp")
file(GLOB RT_SOURCES [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/render_tasks/*.cpp")
file(GLOB D3D12_HEADERS [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/d3d12/*.hpp")
file(GLOB D3D12_SOURCES [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/d3d12/*.cpp")
file(GLOB UTIL_HEADERS [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.hpp")
file(GLOB UTIL_SOURCES [CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp")

source_group("High Level API" FILES ${SOURCES} ${HEADERS})
source_group("Frame Graph" FILES ${FG_SOURCES} ${FG_HEADERS})
source_group("Scene Graph" FILES ${SG_SOURCES} ${SG_HEADERS})
source_group("Render Tasks" FILES ${RT_SOURCES} ${RT_HEADERS})
source_group("D3D12" FILES ${D3D12_SOURCES} ${D3D12_HEADERS})
source_group("Utility" FILES ${UTIL_SOURCES} ${UTIL_HEADERS})

## dependencies ##
add_subdirectory(deps/fmt)

## dependencies options ##
set(FMT_TEST OFF)
set(FMT_DOC OFF)
set(FMT_PEDANTIC OFF)
set(FMT_WERROR OFF)

## dependencies sorting ##
set_target_properties (fmt PROPERTIES FOLDER ThirdParty)
if (MSVC)
	target_compile_options(fmt PRIVATE /W0)
endif()

add_executable(EnginePrototype ${HEADERS} ${SOURCES} ${UTIL_HEADERS} ${UTIL_SOURCES}  ${RT_HEADERS} ${RT_SOURCES} ${FG_HEADERS} ${FG_SOURCES} ${SG_HEADERS} ${SG_SOURCES} ${D3D12_SOURCES} ${D3D12_HEADERS})
target_link_libraries(EnginePrototype dxguid.lib d3d12.lib dxgi.lib d3dcompiler.lib)
set_target_properties(EnginePrototype PROPERTIES CXX_STANDARD 17)
set_target_properties(EnginePrototype PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(EnginePrototype PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
	target_compile_options(EnginePrototype PRIVATE /W4 /permissive- /MP /Gm-)
endif()

target_include_directories(EnginePrototype PUBLIC deps/fmt/include)
target_link_libraries(EnginePrototype fmt)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT EnginePrototype)
set_target_properties(EnginePrototype PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
