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
add_subdirectory("external/assimp")
add_subdirectory("external/DirectXTK")
add_executable(derma WIN32 src/derma.cpp "include/Camera.h" "src/Camera.cpp" "include/Lights.h" "src/Shaders.cpp" "include/Shaders.h" "include/Model.h"  "include/Material.h" "include/RendererBackend.h"  "include/BufferCache.h" "include/BufferObject.h" "src/BufferObject.cpp" "src/BufferCache.cpp" "include/RenderHardwareInterface.h" "src/RenderHardwareInterface.cpp" "include/RHIState.h" "src/RHIState.cpp" "include/ModelManager.h" "include/RendererFrontend.h" "src/RendererFrontend.cpp" "src/RendererBackend.cpp" "include/ResourceCache.h" "src/ResourceCache.cpp" "src/Model.cpp" "include/Editor/SceneHierarchy.h" "src/Editor/SceneHierarchy.cpp" "src/Lights.cpp" "include/Editor/Editor.h" "src/Editor/Editor.cpp" "include/Editor/EntityOutline.h" "src/Editor/EntityOutline.cpp" "include/SceneGraph.h" "src/SceneGraph.cpp" "include/Editor/Logger.h" "src/Editor/Logger.cpp" "include/Transform.h" "src/Transform.cpp")
target_include_directories(derma PRIVATE "include")

set_property (TARGET derma
  PROPERTY
    # Enable C++17 standard compliance
    CXX_STANDARD 17
)


target_link_libraries(derma PRIVATE DirectXTK imgui assimp::assimp)
ADD_DEPENDENCIES(derma copy_shader_files)