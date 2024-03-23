cmake_minimum_required(VERSION 3.12)
project(derma-renderer)

add_subdirectory("external/imgui")
add_subdirectory("external/assimp")
add_subdirectory("external/DirectXTK")
add_executable(derma WIN32 "src/derma.cpp"   "src/renderer/backend/rhi/BufferObject.cpp" "src/renderer/frontend/Camera.cpp"  "src/renderer/frontend/Shaders.cpp" "src/renderer/backend/BufferCache.cpp"  "src/renderer/backend/rhi/RenderHardwareInterface.cpp"  "src/renderer/backend/rhi/RHIState.cpp"   "src/renderer/frontend/RendererFrontend.cpp" "src/renderer/backend/RendererBackend.cpp"  "src/renderer/backend/ResourceCache.cpp" "src/renderer/frontend/Model.cpp"  "src/editor/SceneHierarchy.cpp" "src/renderer/frontend/Lights.cpp"  "src/editor/Editor.cpp"  "src/editor/EntityOutline.cpp"  "src/renderer/frontend/SceneGraph.cpp"  "src/editor/Logger.cpp"  "src/renderer/frontend/Transform.cpp"  "src/renderer/backend/rhi/RenderDeviceDX11.cpp"   "src/renderer/frontend/Material.cpp" "src/renderer/frontend/Shader.cpp")

target_include_directories(derma PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_include_directories(derma PRIVATE "${PROJECT_SOURCE_DIR}/external/imgui-filebrowser")
target_link_libraries(derma PRIVATE DirectXTK imgui assimp::assimp)

set_property (TARGET derma
  PROPERTY
    # Enable C++17 standard compliance
    CXX_STANDARD 17
)

add_custom_target(
        copy_shader_files
        ${CMAKE_COMMAND}
        -D SOURCE_DIR=${CMAKE_SOURCE_DIR}/src/shaders/assets
        -D DESTINATION_DIR=${CMAKE_BINARY_DIR}
        -P CopyFile.cmake
        COMMENT "Copying Files for target: derma" VERBATIM 
        )

add_dependencies(derma copy_shader_files)

# Pre-processor for reflection support
add_subdirectory("tools")

add_custom_target(zorya_preprocessor
    COMMAND preprocessor
    DEPENDS preprocessor
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running Preprocessor"
)

add_dependencies(derma zorya_preprocessor)

# Shader compilation
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

set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderOptions NOOPTIONS)

set_property(
  SOURCE "src/shaders/SSSSS.hlsl"
  APPEND
  PROPERTY ShaderOptions
  OP1 OP2
)


foreach(FILE ${HLSL_SHADER_FILES})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    get_source_file_property(shadertype ${FILE} ShaderType)
    get_source_file_property(shadermodel ${FILE} ShaderModel)
    get_source_file_property(shaderoptions ${FILE} ShaderOptions)

    foreach(shaderopt ${shaderoptions})
        add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/headers/${FILE_WE}_${shaderopt}.h"
                         COMMAND fxc.exe /nologo /E${shadertype} /Vn g_${FILE_WE} /I "${PROJECT_SOURCE_DIR}/src" /D ${shaderopt} /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fh "./src/shaders/headers/${FILE_WE}_${shaderopt}.h" /Fd "${CMAKE_BINARY_DIR}/shaders/${FILE_WE}_${shaderopt}.pdb" ${FILE}
                         MAIN_DEPENDENCY ${FILE}
                         COMMENT "Compiling HLSL ${FILE}..."
                         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                         VERBATIM)

        add_custom_target(shaders_${FILE_WE}_${shaderopt} DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/headers/${FILE_WE}_${shaderopt}.h")
        add_dependencies(derma shaders_${FILE_WE}_${shaderopt})
    endforeach(shaderopt)

endforeach(FILE)
