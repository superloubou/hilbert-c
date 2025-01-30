cmake_minimum_required(VERSION 3.14)

project(hilbert)

set(CMAKE_C_STANDARD 11)

add_subdirectory(lib)

add_custom_target(copy_data ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders) 

add_executable(hilbert ./src/main.c)

add_dependencies(hilbert copy_data)

target_link_libraries(hilbert PRIVATE glad glfw)