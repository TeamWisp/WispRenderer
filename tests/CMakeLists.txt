function(add_test TEST_DIR TEST_NAME)
	message(STATUS "Configuring example ${TEST_NAME} in ${TEST_DIR}")
 
	# source
	file(GLOB SOURCES "${TEST_DIR}/*.cpp")
	file(GLOB HEADERS "${TEST_DIR}/*.hpp")
 
	add_executable(${TEST_NAME} ${HEADERS} ${SOURCES})
	target_include_directories(${TEST_NAME} PUBLIC ../src/)
	target_link_libraries(${TEST_NAME} WispRenderer)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD 20)
	set_target_properties(${TEST_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	set_target_properties(${TEST_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
	set_target_properties(${TEST_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
endfunction(add_test)

function(add_example EXAMPLE_DIR EXAMPLE_NAME)
	message(STATUS "Configuring example ${EXAMPLE_NAME} in ${EXAMPLE_DIR}")
 
	# source
	file(GLOB SOURCES "${EXAMPLE_DIR}/*.cpp")
	file(GLOB HEADERS "${EXAMPLE_DIR}/*.hpp")
 
	add_executable(${EXAMPLE_NAME} WIN32 ${HEADERS} ${SOURCES})
	target_include_directories(${EXAMPLE_NAME} PUBLIC ../src/)
	target_link_libraries(${EXAMPLE_NAME} WispRenderer)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CXX_STANDARD 20)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
endfunction(add_example)

add_example(demo Demo)
add_test(graphics_benchmark GraphicsBenchmark)
