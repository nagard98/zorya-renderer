cmake_minimum_required(VERSION 3.12)
project(zorya-renderer)

add_subdirectory("external/imgui")
add_subdirectory("external/assimp")
add_subdirectory("external/DirectXTK")
add_subdirectory("external/libyaml")
add_subdirectory("external/xxHash/cmake_unofficial")

add_executable(zorya WIN32 "src/zorya.cpp"   "src/renderer/backend/rhi/BufferObject.cpp" "src/renderer/frontend/Camera.cpp"  "src/renderer/frontend/Shaders.cpp" "src/renderer/frontend/DiffusionProfile.cpp" "src/renderer/backend/BufferCache.cpp"  "src/renderer/backend/rhi/RenderHardwareInterface.cpp"  "src/renderer/backend/rhi/RHIState.cpp"   "src/renderer/frontend/SceneManager.cpp" "src/renderer/backend/Renderer.cpp"  "src/renderer/backend/ResourceCache.cpp" "src/renderer/frontend/Model.cpp"  "src/editor/SceneHierarchy.cpp" "src/renderer/frontend/Lights.cpp"  "src/editor/Editor.cpp"  "src/editor/EntityOutline.cpp"  "src/renderer/frontend/SceneGraph.cpp"  "src/editor/Logger.cpp"  "src/renderer/frontend/Transform.cpp"  "src/renderer/backend/rhi/RenderDeviceDX11.cpp"   "src/renderer/frontend/Material.cpp" "src/renderer/frontend/Shader.cpp" "src/renderer/backend/GraphicsContext.cpp" "src/editor/ContentBrowser.cpp" "src/editor/ContentBrowser.cpp" "src/renderer/frontend/Asset.cpp" "src/editor/AssetRegistry.cpp" "src/data_structures/Tree.cpp" "src/renderer/backend/PipelineStateObject.cpp" "src/editor/AssetInspector.cpp" "src/renderer/frontend/Texture2D.cpp" "src/utils/Hash.cpp" "src/utils/Arena.h" "src/utils/Arena.cpp" "src/utils/Pool.h" "src/utils/Pool.cpp" "src/Platform.cpp" "src/data_structures/DynamicArray.cpp" "src/renderer/backend/JimenezSeparable.h" "src/renderer/passes/GBufferPass.cpp" "src/renderer/backend/RenderGraph.cpp" "src/renderer/passes/SkyboxPass.cpp" "src/renderer/passes/PresentPass.cpp" "src/renderer/passes/DebugViewPass.cpp" "src/renderer/passes/LightingPass.cpp" "src/renderer/passes/ShadowMappingPass.cpp" "src/renderer/passes/SSSPass.cpp" "src/renderer/frontend/PipelineStateManager.cpp" "src/renderer/passes/EquirectangularToCubemapPass.cpp")

target_include_directories(zorya PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_include_directories(zorya PRIVATE "${PROJECT_SOURCE_DIR}/external/imgui-filebrowser")
target_include_directories(zorya PRIVATE "${PROJECT_SOURCE_DIR}/external/libyaml/include")
target_include_directories(zorya PRIVATE "${PROJECT_SOURCE_DIR}/external/stb")

target_link_libraries(zorya PRIVATE DirectXTK imgui assimp::assimp yaml xxHash::xxhash)

set_property (TARGET zorya
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
        COMMENT "Copying Files for target: zorya" VERBATIM 
        )

add_dependencies(zorya copy_shader_files)

# Pre-processor for reflection support
add_subdirectory("tools")

add_custom_target(zorya_preprocessor
    COMMAND preprocessor
    DEPENDS preprocessor
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running Preprocessor"
)

#add_dependencies(derma zorya_preprocessor)

# Shader compilation
set(HLSL_SHADER_FILES "src/shaders/DepthPS.hlsl" "src/shaders/DepthVS.hlsl" "src/shaders/GBufferPS.hlsl" "src/shaders/ShadowMaskBuild.hlsl" "src/shaders/BuildPreFilteredMap.hlsl"
                "src/shaders/GBufferVS.hlsl" "src/shaders/Lighting.hlsl" "src/shaders/Present.hlsl" "src/shaders/FullscreenQuad.hlsl" "src/shaders/SimpleVS.hlsl" "src/shaders/Convolve_Cubemap.hlsl"
                "src/shaders/ShadowMapping.hlsl" "src/shaders/SkyboxPS.hlsl" "src/shaders/SkyboxVS.hlsl" "src/shaders/SSSSS.hlsl" "src/shaders/Composit.hlsl" "src/shaders/EquirectangularToCubemap.hlsl")
                
set_source_files_properties("src/shaders/DepthPS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/DepthVS.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties("src/shaders/SimpleVS.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties("src/shaders/GBufferPS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/GBufferVS.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties("src/shaders/Lighting.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/Present.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/Composit.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/ShadowMapping.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/ShadowMaskBuild.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/SkyboxPS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/Convolve_Cubemap.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/BuildPreFilteredMap.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/SkyboxVS.hlsl" PROPERTIES ShaderType "vs")
set_source_files_properties("src/shaders/SSSSS.hlsl" PROPERTIES ShaderType "ps")
set_source_files_properties("src/shaders/EquirectangularToCubemap.hlsl" PROPERTIES ShaderType "ps")
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
    #get_source_file_property(shaderoptions ${FILE} ShaderOptions)

    #foreach(shaderopt ${shaderoptions})
        add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/shaders/${FILE_WE}.cso"
                         COMMAND fxc.exe /nologo /E${shadertype} /Vn g_${FILE_WE} /I "${PROJECT_SOURCE_DIR}/src" /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo "${CMAKE_BINARY_DIR}/shaders/${FILE_WE}.cso" ${FILE}
                         MAIN_DEPENDENCY ${FILE}
                         COMMENT "Compiling HLSL ${FILE}..."
                         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                         VERBATIM)

        add_custom_target(shaders_${FILE_WE} DEPENDS "${CMAKE_BINARY_DIR}/shaders/${FILE_WE}.cso")
        add_dependencies(zorya shaders_${FILE_WE})
    #endforeach(shaderopt)

endforeach(FILE)
