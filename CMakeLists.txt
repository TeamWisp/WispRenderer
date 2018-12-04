cmake_minimum_required(VERSION 3.13)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_compile_definitions(_ENABLE_EXTENDED_ALIGNED_STORAGE)

project(Wisp)

option(ENABLE_UNIT_TEST "enables unit tests." OFF)

option(BUILD_SHARED_LIBS OFF)

##### OUTPUT DIRECTORIES #####
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

##### SOURCES #####
file(GLOB HEADERS CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCES CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB FG_HEADERS CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/frame_graph/*.hpp")
file(GLOB FG_SOURCES CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/frame_graph/*.cpp")
file(GLOB SG_HEADERS CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/scene_graph/*.hpp")
file(GLOB SG_SOURCES CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/scene_graph/*.cpp")
file(GLOB RT_HEADERS CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/render_tasks/*.hpp")
file(GLOB RT_SOURCES CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/render_tasks/*.cpp")
file(GLOB D3D12_HEADERS CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/d3d12/*.hpp")
file(GLOB D3D12_SOURCES CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/d3d12/*.cpp")
file(GLOB UTIL_HEADERS CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.hpp")
file(GLOB UTIL_SOURCES CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp")
file(GLOB IMGUI_HEADERS CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/*.hpp")
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS] "${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/*.cpp")

source_group("High Level API" FILES ${SOURCES} ${HEADERS})
source_group("Frame Graph" FILES ${FG_SOURCES} ${FG_HEADERS})
source_group("Scene Graph" FILES ${SG_SOURCES} ${SG_HEADERS})
source_group("Render Tasks" FILES ${RT_SOURCES} ${RT_HEADERS})
source_group("D3D12" FILES ${D3D12_SOURCES} ${D3D12_HEADERS})
source_group("Utility" FILES ${UTIL_SOURCES} ${UTIL_HEADERS})
source_group("ImGui" FILES ${IMGUI_SOURCES} ${IMGUI_HEADERS})

## dependencies ##
add_subdirectory(deps/fmt)
add_subdirectory(deps/fallback)

set(ASSIMP_BUILD_SHARED_LIBS OFF)

add_subdirectory(deps/assimp)

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

set_target_properties(assimp PROPERTIES FOLDER ThirdParty)
# if (MSVC)
#	target_compile_options(assimp PRIVATE /W0)
# endif()

set_target_properties(assimp_cmd PROPERTIES FOLDER ThirdParty)
# if (MSVC)
#	target_compile_options(assimp_cmd PRIVATE /W0)
# endif()

set_target_properties(IrrXML PROPERTIES FOLDER ThirdParty)
# if (MSVC)
#	target_compile_options(IrrXML PRIVATE /W0)
# endif()

set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ThirdParty)

set_target_properties(zlib PROPERTIES FOLDER ThirdParty)
# if (MSVC)
#	target_compile_options(zlib PRIVATE /W0)
# endif()

set_target_properties(zlibstatic PROPERTIES FOLDER ThirdParty)
# if (MSVC)
#	target_compile_options(zlibstatic PRIVATE /W0)
# endif()

add_library(WispRenderer STATIC ${HEADERS} ${SOURCES} ${IMGUI_HEADERS} ${IMGUI_SOURCES} ${UTIL_HEADERS} ${UTIL_SOURCES} ${RT_HEADERS} ${RT_SOURCES} ${FG_HEADERS} ${FG_SOURCES} ${SG_HEADERS} ${SG_SOURCES} ${D3D12_SOURCES} ${D3D12_HEADERS})
target_link_libraries(WispRenderer DXRFallback dxguid.lib d3d12.lib dxgi.lib d3dcompiler.lib)
set_target_properties(WispRenderer PROPERTIES CXX_STANDARD 17)
set_target_properties(WispRenderer PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(WispRenderer PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
	target_compile_options(WispRenderer PRIVATE /W4 /permissive- /MP /Gm-)
endif()

link_directories(deps/fallback/Bin)
target_include_directories(WispRenderer PUBLIC deps/fallback/Include deps/fmt/include deps/assimp/include)
target_link_libraries(WispRenderer fmt assimp)

# target_include_directories(WispRenderer PUBLIC deps/assimp/include)
# target_link_libraries(WispRenderer assimp)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WispDemo)
set_target_properties(WispRenderer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")

add_subdirectory(demo)

## Unit Test ##
if(ENABLE_UNIT_TEST)
  ## add_executable(UnitTest WIN32 EXCLUDE_FROM_ALL ${HEADERS} 
  ## ${SOURCES} 
  ## ${IMGUI_HEADERS} 
  ## ${IMGUI_SOURCES} 
  ## ${UTIL_HEADERS} 
  ## ${UTIL_SOURCES} 
  ## ${RT_HEADERS} 
  ## ${RT_SOURCES} 
  ## ${FG_HEADERS} 
  ## ${FG_SOURCES} 
  ## ${SG_HEADERS} 
  ## ${SG_SOURCES} 
  ## ${D3D12_SOURCES} 
  ## ${D3D12_HEADERS} )

  ## target_link_libraries(UnitTest dxguid.lib d3d12.lib dxgi.lib d3dcompiler.lib)
  ## set_target_properties(UnitTest PROPERTIES CXX_STANDARD 17)
  ## set_target_properties(UnitTest PROPERTIES CXX_EXTENSIONS OFF)
  ## set_target_properties(UnitTest PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
  ## if (MSVC)
  ## target_compile_options(UnitTest PRIVATE /W4 /permissive- /MP /Gm-)
  ## endif()

  ## target_include_directories(UnitTest PUBLIC deps/fmt/include)
  ## target_link_libraries(UnitTest fmt)
  ## set_target_properties(UnitTest PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
endif(ENABLE_UNIT_TEST)

## Generate Include Directory
macro(configure_files srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

	file(GLOB_RECURSE templateFiles RELATIVE ${srcDir} ${srcDir}/*)
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            message(STATUS "Configuring file ${templateFile}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    @ONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
endmacro(configure_files)

#configure_files("${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/include")
