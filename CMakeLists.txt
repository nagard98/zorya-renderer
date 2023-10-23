cmake_minimum_required(VERSION 3.12)
project(derma-renderer)

ADD_CUSTOM_TARGET(
        copy_shader_files
        ${CMAKE_COMMAND}
        -D SOURCE_DIR=${CMAKE_SOURCE_DIR}/src/shaders
        -D DESTINATION_DIR=${CMAKE_BINARY_DIR}
        -P CopyFile.cmake
        COMMENT "Copying Files for target: derma" VERBATIM 
        )

add_subdirectory("external/imgui")

find_package(directxtk CONFIG REQUIRED)
add_executable(derma WIN32 src/derma.cpp "include/Camera.h" "src/Camera.cpp" "include/Lights.h" "src/Shaders.cpp" "include/Shaders.h" "include/Mesh.h" "src/Mesh.cpp")

target_link_libraries(derma PRIVATE Microsoft::DirectXTK imgui)
ADD_DEPENDENCIES(derma copy_shader_files)