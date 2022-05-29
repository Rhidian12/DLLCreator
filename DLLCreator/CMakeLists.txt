cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/BUILD)

project(DLLCreatorProject)

add_subdirectory(DLLCreator)
add_subdirectory(Utils)

add_executable(DLLCreatorProject main.cpp)

set(Includes DLLCreatorInclude UtilsInclude)
set(Libraries DLLCreator Utils)

foreach(Include ${Includes})
	target_include_directories(DLLCreatorProject PUBLIC ${Include})
endforeach()

foreach(Library ${Libraries})
	target_link_libraries(DLLCreatorProject PUBLIC ${Library})
endforeach()

file(GLOB TextFiles "Resources/*.txt")

add_custom_command(TARGET DLLCreatorProject PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Resources/)

foreach(File ${TextFiles})
	add_custom_command(TARGET DLLCreatorProject POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	${File} 
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Resources/)
endforeach()