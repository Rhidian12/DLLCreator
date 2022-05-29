cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")

set(CPACK_NSIS_MODIFY_PATH ON)
include(CPack)