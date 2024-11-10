#ifndef RENDER_GRAPH_H_
#define RENDER_GRAPH_H_

#include <functional>
#include <cstdint>
#include <vector>
#include <array>

#include "Platform.h"
#include "GraphicsContext.h"
#include "RenderScope.h"

namespace zorya
{
    class Render_Graph_Registry;
    class Render_Graph_Builder;

    using Pass_Execute_Callback  = std::function<void(Render_Command_List&, Render_Graph_Registry&)>;
    using Pass_Setup_Callback  = std::function<Pass_Execute_Callback(Render_Graph_Builder&)>;

    enum class Format
    {
        FORMAT_R8G8B8A8_UNORM,
        FORMAT_R8G8B8A8_UNORM_SRGB,
        FORMAT_D32_FLOAT,
        FORMAT_D24_UNORM_S8_UINT,
        FORMAT_R16G16_UNORM,
        FORMAT_R11G11B10_FLOAT
    };

    enum Bind_Flag : uint8_t
    {
        SHADER_RESOURCE = 1,
        UNORDERED_ACCESS = 2,
        RENDER_TARGET = 4,
        DEPTH_STENCIL = 8,
        MASK = 15
    };

    enum Other_Flags : uint8_t
    {
        DEPTH_READ_ONLY = 16
    };

    using View_Flags = uint8_t;

    using Bind_Flags = uint8_t;

    struct Render_Graph_Resource_Desc
    {
        uint32_t width;
        uint32_t height;
        Format format;
        bool is_cubemap{ false };
        bool has_mips{ false };
        u32 arr_size{1};
        //TODO: temporary here; in the render graph you shouldn't explicitly define the binding options
        //Bind_Flags bind_flags = 0;
    };

    struct Render_Graph_Resource_Metadata
    {
        Render_Graph_Resource_Desc desc;
        Bind_Flags bind_flags;
    };

    struct Render_Graph_Resource_Handle
    {
        uint32_t index;
    };
      
    struct Render_Graph_Resource
    {
        uint32_t desc_hnd;
        uint32_t resource_hnd;
        uint32_t gpu_res_hnd;
        int32_t ref_count;
    };

    struct Render_Graph_View_Desc
    {
        Bind_Flag bind_flag;
        u32 slice_start_index{ 0 };
        u32 slice_size{ 0 };
        u32 mip_index{ 0 };
        bool is_read_only;
    };

    struct Render_Pass_Resource
    {
        uint32_t resource_hnd;
        uint32_t gpu_res_hnd;
        Render_Graph_View_Desc view_desc;
    };

    struct Render_Pass
    {
        std::vector<Render_Pass_Resource> pass_read_resources;
        std::vector<Render_Pass_Resource> pass_write_resources;
        std::vector<uint32_t> input_resources;
        uint32_t ref_count;
        const wchar_t* name;
        Pass_Execute_Callback pass_callback;
    };

    class Render_Graph_Builder
    {
    public:

        Render_Graph_Builder(std::vector<Render_Graph_Resource_Metadata>& _graph_resource_desc, std::vector<Render_Graph_Resource>& _graph_resources, std::vector<std::vector<uint32_t>>& _graph_resources_producers, std::vector<Render_Pass>& _render_passes, std::vector<Render_Resource_Handle>& _gpu_resources)
            : graph_resource_meta(_graph_resource_desc), graph_resources(_graph_resources), graph_resources_producers(_graph_resources_producers), render_passes(_render_passes), num_gpu_resources(0)/*, gpu_resources(_gpu_resources)*/ {}

        Render_Graph_Resource read(Render_Graph_Resource rg_resource, Bind_Flag bind_flag, u32 slice_index_start = 0, u32 slice_size = 0);
        Render_Graph_Resource write(Render_Graph_Resource rg_resource, Bind_Flag bind_flag, u32 slice_index_start = 0, u32 slice_size = 0, u32 mip_index = 0);
        Render_Graph_Resource create(const Render_Graph_Resource_Desc& resource_desc/*, Bind_Flag bind_flag*/);

        void build(std::vector<Render_Resource_Handle>& gpu_resources);
        void build_pass();
        void clear();

    private:
        std::vector<Render_Graph_Resource_Metadata>& graph_resource_meta;
        std::vector<Render_Graph_Resource>& graph_resources;
        std::vector<std::vector<uint32_t>>& graph_resources_producers;
        std::vector<Render_Pass>& render_passes;
        //std::vector<Render_Resource_Handle>& gpu_resources;
        uint32_t num_gpu_resources;

    };

    class Render_Graph_Registry
    {
    public:

        Render_Graph_Registry(std::vector<Render_Resource_Handle>& _gpu_resources) : gpu_resources(_gpu_resources) {}

        template <typename T>
        T get(Render_Graph_Resource hnd_rg_resource)
        {
            //zassert(gpu_resources[hnd_rg_resource.gpu_res_hnd].index != 0);
            return T{ gpu_resources[hnd_rg_resource.gpu_res_hnd].index };
        }

    private:
        std::vector<Render_Resource_Handle>& gpu_resources;
    };


	class Render_Graph
	{
	public:

        Render_Graph() : builder(graph_resource_meta, graph_resources, graph_resource_producers, render_passes, gpu_resources), registry(gpu_resources) {}

		template <typename Lambda>
        void add_pass_callback(const wchar_t* pass_name, Lambda&& setup_function)
        {
            Render_Pass& render_pass = render_passes.emplace_back();
            render_pass.name = pass_name;
            render_pass.pass_callback = setup_function(builder);
            builder.build_pass();
        }

        void compile();
        void execute(Render_Command_List& cmd_list);

        template <typename T, typename F>
        void import_persistent_resource(Render_Scope& render_scope, const F& persistent_resource)
        {
            render_scope.set(T{ persistent_resource });
        }

    private:
        void clear_setup();

        std::vector<Render_Graph_Resource_Metadata> graph_resource_meta;

        std::vector<Render_Graph_Resource> graph_resources;
        std::vector<std::vector<uint32_t>> graph_resource_producers;
        std::vector<Render_Pass> render_passes;
        std::vector<uint8_t> skip_pass;

        std::vector<Render_Resource_Handle> gpu_resources;

        bool is_recompile_required = true;

        Render_Graph_Registry registry;
        Render_Graph_Builder builder;

	};

}
#endif