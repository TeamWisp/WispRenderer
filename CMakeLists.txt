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
configure_file(${CMAKE_SOURCE_DIR}/deps/fallback/Bin/dxrfallbackcompiler.dll ${CMAKE_SOURCE_DIR} COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/deps/fallback/Bin/dxil.dll ${CMAKE_SOURCE_DIR} COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/deps/fallback/Bin/dxcompiler.dll ${CMAKE_SOURCE_DIR} COPYONLY)

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

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
## dependencies ##
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/fmt ${CMAKE_BINARY_DIR}/fmt)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/DirectXTex ${CMAKE_BINARY_DIR}/DirectXTex)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/fallback ${CMAKE_BINARY_DIR}/fallback)

set(ASSIMP_BUILD_SHARED_LIBS OFF)

set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)

#IMPORTERS
set( ASSIMP_BUILD_AMF_IMPORTER      OFF )
set( ASSIMP_BUILD_3DS_IMPORTER      OFF )
set( ASSIMP_BUILD_AC_IMPORTER       OFF )
set( ASSIMP_BUILD_ASE_IMPORTER      OFF )
set( ASSIMP_BUILD_ASSBIN_IMPORTER   OFF )
set( ASSIMP_BUILD_ASSXML_IMPORTER   OFF )
set( ASSIMP_BUILD_B3D_IMPORTER      OFF )
set( ASSIMP_BUILD_BVH_IMPORTER      OFF )
set( ASSIMP_BUILD_COLLADA_IMPORTER  OFF )
set( ASSIMP_BUILD_DXF_IMPORTER      OFF )
set( ASSIMP_BUILD_CSM_IMPORTER      OFF )
set( ASSIMP_BUILD_HMP_IMPORTER      OFF )
set( ASSIMP_BUILD_IRRMESH_IMPORTER  OFF )
set( ASSIMP_BUILD_IRR_IMPORTER      OFF )
set( ASSIMP_BUILD_LWO_IMPORTER      OFF )
set( ASSIMP_BUILD_LWS_IMPORTER      OFF )
set( ASSIMP_BUILD_MD2_IMPORTER      OFF )
set( ASSIMP_BUILD_MD3_IMPORTER      OFF )
set( ASSIMP_BUILD_MD5_IMPORTER      OFF )
set( ASSIMP_BUILD_MDC_IMPORTER      OFF )
set( ASSIMP_BUILD_MDL_IMPORTER      OFF )
set( ASSIMP_BUILD_NFF_IMPORTER      OFF )
set( ASSIMP_BUILD_NDO_IMPORTER      OFF )
set( ASSIMP_BUILD_OFF_IMPORTER      OFF )
set( ASSIMMP_BUILD_OBJ_IMPORTER     ON  ) #ON
set( ASSIMP_BUILD_OGRE_IMPORTER     OFF )
set( ASSIMP_BUILD_OPENGEX_IMPORTER  OFF )
set( ASSIMP_BUILD_PLY_IMPORTER      OFF )
set( ASSIMP_BUILD_MS3D_IMPORTER     OFF )
set( ASSIMP_BUILD_COB_IMPORTER      OFF )
set( ASSIMP_BUILD_BLEND_IMPORTER    OFF )
set( ASSIMP_BUILD_IFC_IMPORTER      OFF )
set( ASSIMP_BUILD_XGL_IMPORTER      OFF )
set( ASSIMMP_BUILD_FBX_IMPORTER     ON  ) #ON
set( ASSIMP_BUILD_Q3D_IMPORTER      OFF )
set( ASSIMP_BUILD_Q3BSP_IMPORTER    OFF )
set( ASSIMP_BUILD_RAW_IMPORTER      ON  ) #ON
set( ASSIMP_BUILD_SIB_IMPORTER      ON  ) #ON
set( ASSIMP_BUILD_SMD_IMPORTER      ON  ) #ON
set( ASSIMP_BUILD_STL_IMPORTER      OFF )
set( ASSIMP_BUILD_TERRAGEN_IMPORTER OFF )
set( ASSIMP_BUILD_3D_IMPORTER       OFF )
set( ASSIMP_BUILD_X_IMPORTER        OFF )
set( ASSIMP_BUILD_X3D_IMPORTER      OFF )
set( ASSIMP_BUILD_GLTF_IMPORTER     ON  ) #ON
set( ASSIMP_BUILD_3MF_IMPORTER      OFF )
set( ASSIMP_BUILD_MMD_IMPORTER      OFF )
# END IMPORTERS

#set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT CACHE INTERNAL FALSE)
#set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/assimp ${CMAKE_BINARY_DIR}/assimp)

## dependencies options ##
set(FMT_TEST OFF)
set(FMT_DOC OFF)
set(FMT_PEDANTIC OFF)
set(FMT_WERROR OFF)
## dependencies sorting ##
set_target_properties (fmt PROPERTIES FOLDER ThirdParty)
set_target_properties (DirectXTex PROPERTIES FOLDER ThirdParty)

if (MSVC)
	target_compile_options(fmt PRIVATE /W0)
	target_compile_options(DirectXTex PRIVATE /W0)
endif()

set_target_properties(assimp PROPERTIES FOLDER ThirdParty)
set_target_properties(assimp_cmd PROPERTIES FOLDER ThirdParty)
set_target_properties(IrrXML PROPERTIES FOLDER ThirdParty)
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ThirdParty)
set_target_properties(zlib PROPERTIES FOLDER ThirdParty)
set_target_properties(zlibstatic PROPERTIES FOLDER ThirdParty)
# set_target_properties(unit PROPERTIES FOLDER ThirdParty)
set_target_properties(uninstall PROPERTIES FOLDER ThirdParty)

add_library(WispRenderer STATIC ${HEADERS} ${SOURCES} ${IMGUI_HEADERS} ${IMGUI_SOURCES} ${UTIL_HEADERS} ${UTIL_SOURCES} ${RT_HEADERS} ${RT_SOURCES} ${FG_HEADERS} ${FG_SOURCES} ${SG_HEADERS} ${SG_SOURCES} ${D3D12_SOURCES} ${D3D12_HEADERS})
set_target_properties(WispRenderer PROPERTIES CXX_STANDARD 17)
set_target_properties(WispRenderer PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(WispRenderer PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
target_compile_options(WispRenderer PRIVATE /W4 /permissive- /MP /Gm-)
endif()

target_include_directories(WispRenderer PUBLIC ${CMAKE_SOURCE_DIR}/deps/DirectXTex/DirectXTex/)
target_include_directories(WispRenderer PUBLIC ${CMAKE_SOURCE_DIR}/deps/fmt/include)
target_include_directories(WispRenderer PUBLIC ${CMAKE_SOURCE_DIR}/deps/assimp/include)
target_include_directories(WispRenderer PUBLIC ${CMAKE_SOURCE_DIR}/deps/fallback/Include)

target_link_libraries(WispRenderer DXRFallback dxguid.lib d3d12.lib dxgi.lib d3dcompiler.lib dxcompiler DirectXTex fmt assimp)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WispDemo)
set_target_properties(WispRenderer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")

add_subdirectory(demo ${CMAKE_BINARY_DIR}/WispDemo)

## Unit Test ##
if(ENABLE_UNIT_TEST)
    set(BUILD_GMOCK 0)
    set(INSTALL_GTEST 0)
    add_subdirectory(${CMAKE_SOURCE_DIR}/deps/googletest/googletest ${CMAKE_BINARY_DIR}/googletest)
    add_subdirectory(test ${CMAKE_BINARY_DIR}/Wisptest)
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
