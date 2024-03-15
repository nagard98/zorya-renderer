cmake_minimum_required(VERSION 3.12)
project(derma-renderer)

add_subdirectory("external/imgui")
add_subdirectory("external/assimp")
add_subdirectory("external/DirectXTK")
add_executable(derma WIN32 src/derma.cpp "include/Camera.h" "src/Camera.cpp" "include/Lights.h" "src/Shaders.cpp" "include/Shaders.h" "include/Model.h"  "include/Material.h" "include/RendererBackend.h"  "include/BufferCache.h" "include/BufferObject.h" "src/BufferObject.cpp" "src/BufferCache.cpp" "include/RenderHardwareInterface.h" "src/RenderHardwareInterface.cpp" "include/RHIState.h" "src/RHIState.cpp" "include/ModelManager.h" "include/RendererFrontend.h" "src/RendererFrontend.cpp" "src/RendererBackend.cpp" "include/ResourceCache.h" "src/ResourceCache.cpp" "src/Model.cpp" "include/Editor/SceneHierarchy.h" "src/Editor/SceneHierarchy.cpp" "src/Lights.cpp" "include/Editor/Editor.h" "src/Editor/Editor.cpp" "include/Editor/EntityOutline.h" "src/Editor/EntityOutline.cpp" "include/SceneGraph.h" "src/SceneGraph.cpp" "include/Editor/Logger.h" "src/Editor/Logger.cpp" "include/Transform.h" "src/Transform.cpp" "include/RenderDevice.h" "src/RenderDeviceDX11.cpp" "src/BuildTools/PreProcessor.cpp" "include/BuildTools/PreProcessor.h" "include/reflection/Reflection.h")

target_include_directories(derma PRIVATE "include")
target_link_libraries(derma PRIVATE DirectXTK imgui assimp::assimp)

set_property (TARGET derma
  PROPERTY
    # Enable C++17 standard compliance
    CXX_STANDARD 17
)

add_custom_target(
        copy_shader_files
        ${CMAKE_COMMAND}
        -D SOURCE_DIR=${CMAKE_SOURCE_DIR}/src/shaders
        -D DESTINATION_DIR=${CMAKE_BINARY_DIR}
        -P CopyFile.cmake
        COMMENT "Copying Files for target: derma" VERBATIM 
        )


# Pre-processor for reflection support
add_subdirectory("src/BuildTools")

add_custom_target(zorya_preprocessor
    COMMAND preprocessor
    DEPENDS preprocessor
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running Preprocessor"
)

set(HLSL_SHADER_FILES "src/shaders/DepthPS.hlsl" "src/shaders/DepthVS.hlsl" "src/shaders/GBufferPS.hlsl"
                "src/shaders/GBufferVS.hlsl" "src/shaders/Lighting.hlsl" "src/shaders/Present.hlsl" "src/shaders/FullscreenQuad.hlsl"
                "src/shaders/ShadowMapping.hlsl" "src/shaders/SkyboxPS.hlsl" "src/shaders/SkyboxVS.hlsl" "src/shaders/SSSSS.hlsl" )
                
set_source_files_properties("src/shaders/DepthPS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/DepthVS.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties("src/shaders/GBufferPS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/GBufferVS.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties("src/shaders/Lighting.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/Present.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/ShadowMapping.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/SkyboxPS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/SkyboxVS.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties("src/shaders/SSSSS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/FullscreenQuad.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "5_0")

foreach(FILE ${HLSL_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  get_source_file_property(shadermodel ${FILE} ShaderModel)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/shaders/${FILE_WE}.h
                     COMMAND fxc.exe /nologo /E${shadertype} /Vn g_${FILE_WE} /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fh ./include/shaders/${FILE_WE}.h /Fd ${CMAKE_BINARY_DIR}/shaders/${FILE_WE}.pdb ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "Compiling HLSL ${FILE}..."
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)

  add_custom_target(shaders_${FILE_WE} DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/shaders/${FILE_WE}.h)
  add_dependencies(derma shaders_${FILE_WE})
endforeach(FILE)


add_dependencies(derma copy_shader_files)
add_dependencies(derma zorya_preprocessor)