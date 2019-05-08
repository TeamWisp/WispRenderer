function(add_test EXAMPLE_DIR EXAMPLE_NAME)
	message(STATUS "Configuring example ${EXAMPLE_NAME} in ${EXAMPLE_DIR}")
 
	# source
	file(GLOB SOURCES "${EXAMPLE_DIR}/*.cpp")
	file(GLOB HEADERS "${EXAMPLE_DIR}/*.hpp")
 
	add_executable(${EXAMPLE_NAME} WIN32 ${HEADERS} ${SOURCES})
	target_include_directories(${EXAMPLE_NAME} PUBLIC ../src/)
	target_link_libraries(${EXAMPLE_NAME} WispRenderer)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CXX_STANDARD 17)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
endfunction(add_test)

add_test(graphics_benchmark GraphicsBenchmark)
add_test(demo Demo)
