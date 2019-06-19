# Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
file(GLOB COMMON_SOURCES "common/*.cpp")
file(GLOB COMMON_HEADERS "common/*.hpp")

function(add_test TEST_DIR TEST_NAME)
	message(STATUS "Configuring example ${TEST_NAME} in ${TEST_DIR}")
 
	# source
	file(GLOB SOURCES "${TEST_DIR}/*.cpp")
	file(GLOB HEADERS "${TEST_DIR}/*.hpp")
 
	add_executable(${TEST_NAME} ${HEADERS} ${SOURCES} ${COMMON_HEADERS} ${COMMON_SOURCES})
	target_include_directories(${TEST_NAME} PUBLIC ../src/)
	target_link_libraries(${TEST_NAME} WispRenderer BulletCollision BulletDynamics LinearMath)
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
 
	add_executable(${EXAMPLE_NAME} WIN32 ${HEADERS} ${SOURCES} ${COMMON_HEADERS} ${COMMON_SOURCES})
	target_include_directories(${EXAMPLE_NAME} PUBLIC ../src/)
	target_link_libraries(${EXAMPLE_NAME} WispRenderer BulletCollision BulletDynamics LinearMath)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CXX_STANDARD 20)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
	set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
endfunction(add_example)

add_test(demo Demo)
add_test(graphics_benchmark GraphicsBenchmark)
