#include "RenderGraph.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"
#include "utils/Pool.h"
#include "xxhash.h"

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace zorya
{
	using Render_Pass_Index = int32_t;

	struct Free_Render_Resource
	{
		Render_Resource_Handle gpu_hnd;
		Bind_Flags bind_flags;
		Free_Render_Resource* next_free;
	};

	struct Resource_Key
	{
		Render_Graph_Resource_Desc desc;
	};

	static uint64_t hash(const Resource_Key graph_resource_key)
	{
		return XXH64(&graph_resource_key, sizeof(graph_resource_key), 0);
	}
	
	DXGI_FORMAT convert_format(Format format)
	{
		switch (format)
		{
		case zorya::Format::FORMAT_R8G8B8A8_UNORM:
		case zorya::Format::FORMAT_R8G8B8A8_UNORM_SRGB:
		{
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
			break;
		}
		case zorya::Format::FORMAT_D32_FLOAT:
		{
			return DXGI_FORMAT_R32_TYPELESS;
			break;
		}
		case zorya::Format::FORMAT_D24_UNORM_S8_UINT:
		{
			return DXGI_FORMAT_R24G8_TYPELESS;
			break;
		}
		case zorya::Format::FORMAT_R11G11B10_FLOAT:
		{
			return DXGI_FORMAT_R11G11B10_FLOAT;
			break;
		}
		default:
			zassert(false);
			break;
		}
	}

	DXGI_FORMAT convert_format(Format format, Bind_Flag bind_flag)
	{
		switch (format)
		{
		case zorya::Format::FORMAT_R8G8B8A8_UNORM:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			}
			case zorya::DEPTH_STENCIL:
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_R8G8B8A8_UNORM_SRGB:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			}
			case zorya::DEPTH_STENCIL:
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_D32_FLOAT:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R32_FLOAT;
				break;
			}
			case zorya::DEPTH_STENCIL:
			{
				return DXGI_FORMAT_D32_FLOAT;
				break;
			}
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_D24_UNORM_S8_UINT:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			}
			case zorya::DEPTH_STENCIL:
			{
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			}
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		case zorya::Format::FORMAT_R11G11B10_FLOAT:
		{
			switch (bind_flag)
			{
			case zorya::SHADER_RESOURCE:
			case zorya::UNORDERED_ACCESS:
			case zorya::RENDER_TARGET:
			{
				return DXGI_FORMAT_R11G11B10_FLOAT;
				break;
			}
			case zorya::DEPTH_STENCIL:
			default:
			{
				zassert(false);
				break;
			}
			}
			break;
		}
		default:
		{
			zassert(false);
			break;
		}
		}
	}

	static std::vector<uint8_t> cull_render_passes(std::vector<Render_Graph_Resource>& graph_resources, std::vector<std::vector<u32>>& graph_resource_producers, std::vector<Render_Pass>& render_passes)
	{
		std::vector<u32> unreferenced_resources;
		std::vector<u32> culled_passes;

		//for (int i = 0; i < render_passes.size(); i++)
		//{
		//	auto& render_pass = render_passes[i];

		//	if (render_pass.ref_count == 0)
		//	{
		//		culled_passes.push_back(i);
		//		for (auto& input_res : render_pass.pass_read_resources)
		//		{
		//			graph_resources.at(input_res.res_hnd).ref_count -= 1;
		//		}
		//	}
		//}

		for (int i = 0; i < graph_resources.size(); i++)
		{
			if (graph_resources[i].ref_count == 0) unreferenced_resources.push_back(i);
		}

		while (!unreferenced_resources.empty())
		{
			auto popped_res_index = unreferenced_resources.back();
			unreferenced_resources.pop_back();

			auto& res_producers = graph_resource_producers.at(popped_res_index);
			for (auto& producer_index : res_producers)
			{
				auto& render_pass = render_passes.at(producer_index);
				render_pass.ref_count -= 1;
				if (render_pass.ref_count == 0)
				{
					culled_passes.push_back(producer_index);

					for (auto input_resource_hnd : render_pass.input_resources)
					{
						auto& read_reasource = graph_resources.at(input_resource_hnd);
						read_reasource.ref_count -= 1;
						if (read_reasource.ref_count == 0) unreferenced_resources.push_back(input_resource_hnd);
					}
				}
			}

		}

		std::vector<uint8_t> skip_pass;
		skip_pass.resize(render_passes.size());

		for (auto pass_index : culled_passes)
		{
			skip_pass.at(pass_index) = 1;
		}

		return skip_pass;
	}


	void Render_Graph::compile()
	{
		static Pool pool(256, sizeof(Free_Render_Resource), alignof(Free_Render_Resource));
		
		if(is_recompile_required){

			builder.build(gpu_resources);

			skip_pass = cull_render_passes(graph_resources, graph_resource_producers, render_passes);

			struct Resource_And_Views
			{
				Render_Resource_Handle hnd_gpu_resource;
				std::unordered_map<View_Flags, Render_Resource_Handle> views;
			};

			std::vector<Resource_And_Views> resource_view_hnds;
			resource_view_hnds.resize(graph_resource_meta.size());
			std::unordered_map<uint64_t, Free_Render_Resource*> free_transient_resources;

			//gpu_resources.resize(graph_resources.size());

			auto buff_size = graph_resource_meta.size() * sizeof(Render_Pass_Index);

			Render_Pass_Index* resource_first_use = (Render_Pass_Index*)_alloca(buff_size);
			Render_Pass_Index* resource_last_use = (Render_Pass_Index*)_alloca(buff_size);

			memset(resource_last_use, -1, buff_size);
			for (int i = 0; i < graph_resource_meta.size(); i++)
			{
				resource_first_use[i] = INT_MAX;
			}

			//Determines resource lifetimes
			for (int i = 0, num_render_passes = render_passes.size(); i < num_render_passes; i++)
			{
				if (skip_pass.at(i) == 0)
				{
					auto& render_pass = render_passes[i];

					for (auto input_res_hnd : render_pass.input_resources)
					{
						auto& graph_resource = graph_resources.at(input_res_hnd);

						if (graph_resource.ref_count > 0)
						{
							resource_first_use[graph_resource.desc_hnd] = std::min(i, resource_first_use[graph_resource.desc_hnd]);
							resource_last_use[graph_resource.desc_hnd] = (std::max)(i, resource_last_use[graph_resource.desc_hnd]);
						} else
						{
							continue;
						}
					}
				}
			}

			std::unordered_set<u32> referenced_graph_resources_desc;

			//Filter resource desc handles for only referenced resources
			for (auto& graph_resource : graph_resources)
			{
				if (graph_resource.ref_count > 0)
				{
					referenced_graph_resources_desc.insert(graph_resource.desc_hnd);
				}
			}

			for (int pass_id = 0, num_render_passes = render_passes.size(); pass_id < num_render_passes; pass_id++)
			{
				if (skip_pass.at(pass_id) == 0)
				{
					auto& render_pass = render_passes[pass_id];

					//Adds to free list the resources that have exceeded their lifetimes
					for (auto graph_resource_desc_id : referenced_graph_resources_desc)
					{
						Render_Pass_Index last_pass_index = resource_last_use[graph_resource_desc_id];
						if (last_pass_index == pass_id - 1)
						{
							auto& gpu_resource = resource_view_hnds.at(graph_resource_desc_id);
							auto& resource_meta = graph_resource_meta.at(graph_resource_desc_id);

							Resource_Key resource_key{ resource_meta.desc };
							auto key_hash = hash(resource_key);

							auto free_resource = free_transient_resources[key_hash];
							Free_Render_Resource* new_free_resource = static_cast<Free_Render_Resource*>(pool.alloc());
							new_free_resource->next_free = free_resource;
							new_free_resource->bind_flags = resource_meta.bind_flags;
							new_free_resource->gpu_hnd = gpu_resource.hnd_gpu_resource;

							free_transient_resources[key_hash] = new_free_resource;
						}
						
					}

					for (auto& read_resource : render_pass.pass_read_resources)
					{
						auto& graph_resource = graph_resources.at(read_resource.resource_hnd);

						if (graph_resource.ref_count > 0)
						{
							auto& gpu_resource = resource_view_hnds.at(graph_resource.desc_hnd);
							auto& resource_meta = graph_resource_meta.at(graph_resource.desc_hnd);

							if (gpu_resource.hnd_gpu_resource.index == 0)
							{
								Resource_Key resource_key{ resource_meta.desc };
								auto key_hash = hash(resource_key);
								auto free_resource = free_transient_resources[key_hash];

								bool is_new_resource_required = true;

								if (free_resource != nullptr)
								{
									Free_Render_Resource* current = free_resource;
									Free_Render_Resource* head = nullptr;

									while (current != nullptr)
									{
										if ((current->bind_flags & resource_meta.bind_flags) == resource_meta.bind_flags)
										{
											gpu_resource.hnd_gpu_resource = current->gpu_hnd;
											free_transient_resources[key_hash] = current->next_free;

											while (current->next_free != nullptr) current = current->next_free;

											current->next_free = head;
											is_new_resource_required = false;

										} else
										{
											head = current;
											current = current->next_free;
										}

									}
								}

								if (is_new_resource_required)
								{
									zassert(rhi.create_tex((Render_Texture_Handle*)&gpu_resource.hnd_gpu_resource, nullptr, resource_meta).value == S_OK);
								}

							}

							//auto& view_map = gpu_resource.views;
							//View_Flags required_flags = read_resource.view_desc.bind_flag | (read_resource.view_desc.is_read_only ? Other_Flags::DEPTH_READ_ONLY : 0);
							//auto view = view_map.find(required_flags);

							//if (view != view_map.end())
							//{
							//	gpu_resources.at(read_resource.gpu_res_hnd) = view->second;
							//} else
							//{
								Render_Resource_Handle hnd_view;

								switch (read_resource.view_desc.bind_flag)
								{
								case zorya::SHADER_RESOURCE:
								{					
									zassert(rhi.create_srv((Render_SRV_Handle*)&hnd_view, (Render_Texture_Handle*)&gpu_resource.hnd_gpu_resource, resource_meta, read_resource.view_desc).value == S_OK);
									//view_map[required_flags] = hnd_view;

									break;
								}
								case zorya::RENDER_TARGET:
								{
									zassert(rhi.create_rtv((Render_RTV_Handle*)&hnd_view, (Render_Texture_Handle*)&gpu_resource.hnd_gpu_resource, resource_meta, read_resource.view_desc).value == S_OK);
									//view_map[required_flags] = hnd_view;

									break;
								}
								case zorya::DEPTH_STENCIL:
								{
									zassert(rhi.create_dsv((Render_DSV_Handle*)&hnd_view, (Render_Texture_Handle*)&gpu_resource.hnd_gpu_resource, resource_meta, read_resource.view_desc).value == S_OK);
									//view_map[required_flags] = hnd_view;

									break;
								}
								case zorya::UNORDERED_ACCESS:
								default:
									zassert(false);
									break;
								}

								gpu_resources.at(read_resource.gpu_res_hnd) = hnd_view;
							//}

						} else
						{
							continue;
						}
					}

					for (auto& write_resource : render_pass.pass_write_resources)
					{
						auto& graph_resource = graph_resources.at(write_resource.resource_hnd);

						if (graph_resource.ref_count > 0)
						{
							auto& gpu_resource = resource_view_hnds.at(graph_resource.desc_hnd);
							auto& resource_meta = graph_resource_meta.at(graph_resource.desc_hnd);

							if (gpu_resource.hnd_gpu_resource.index == 0)
							{
								Resource_Key resource_key{ resource_meta.desc };
								auto key_hash = hash(resource_key);
								auto free_resource = free_transient_resources[key_hash];

								bool is_new_resource_required = true;

								if (free_resource != nullptr)
								{
									Free_Render_Resource* current = free_resource;
									Free_Render_Resource* head = nullptr;

									while (current != nullptr)
									{
										if ((current->bind_flags & resource_meta.bind_flags) == resource_meta.bind_flags)
										{
											gpu_resource.hnd_gpu_resource = current->gpu_hnd;
											free_transient_resources[key_hash] = current->next_free;

											while (current->next_free != nullptr) current = current->next_free;

											current->next_free = head;
											current = nullptr;
											is_new_resource_required = false;

										} else
										{
											head = current;
											current = current->next_free;
										}

									}
								}

								if (is_new_resource_required)
								{
									zassert(rhi.create_tex((Render_Texture_Handle*)&gpu_resource.hnd_gpu_resource, nullptr, resource_meta).value == S_OK);
								}
							}

							zassert(gpu_resource.hnd_gpu_resource.index != 0);

							//auto& view_map = gpu_resource.views;
							//auto view = view_map.find(write_resource.view_desc.bind_flag);

							//if (view != view_map.end())
							//{
							//	gpu_resources.at(write_resource.gpu_res_hnd) = view->second;
							//} else
							//{
								Render_Resource_Handle hnd_view;

								switch (write_resource.view_desc.bind_flag)
								{
								case zorya::RENDER_TARGET:
								{
									zassert(rhi.create_rtv((Render_RTV_Handle*)&hnd_view, (Render_Texture_Handle*)&gpu_resource.hnd_gpu_resource, resource_meta, write_resource.view_desc).value == S_OK);
									//view_map[write_resource.view_desc.bind_flag] = hnd_view;

									break;
								}
								case zorya::DEPTH_STENCIL:
								{
									zassert(rhi.create_dsv((Render_DSV_Handle*)&hnd_view, (Render_Texture_Handle*)&gpu_resource.hnd_gpu_resource, resource_meta, write_resource.view_desc).value == S_OK);
									//view_map[write_resource.view_desc.bind_flag] = hnd_view;

									break;
								}
								case zorya::UNORDERED_ACCESS:
								case zorya::SHADER_RESOURCE:
								default:
									zassert(false);
									break;
								}

								gpu_resources.at(write_resource.gpu_res_hnd) = hnd_view;

							//}

						} else
						{
							continue;
						}
					}

				}
			}

			pool.clear();
			is_recompile_required = false;
		}
	}

	void Render_Graph::execute(Render_Command_List& cmd_list)
	{
		int i = 0;
		for (auto& render_pass : render_passes)
		{
			if (skip_pass.at(i) == 0)
			{
				cmd_list.begin_event(render_pass.name);
				render_pass.pass_callback(cmd_list, registry);
				cmd_list.end_event();
			}
			i += 1;
		}

		rhi.m_context->ExecuteCommandList(cmd_list.finish_command_list(), false);

		clear_setup();
	}

	void Render_Graph::clear_setup()
	{
		render_passes.clear();
		graph_resource_meta.clear();
		graph_resources.clear();
		graph_resource_producers.clear();

		builder.clear();
	}

	Render_Graph_Resource Render_Graph_Builder::read(Render_Graph_Resource rg_resource, Bind_Flag bind_flag, u32 slice_index_start, u32 slice_size)
	{
		graph_resource_meta.at(rg_resource.desc_hnd).bind_flags |= bind_flag;
		auto in_resource_hnd = rg_resource.resource_hnd;

		if (rg_resource.ref_count == -1)
		{
			graph_resources.at(in_resource_hnd).ref_count = 0;
		}

		auto& in_resource = graph_resources.at(in_resource_hnd);
		in_resource.ref_count += 1;

		u32 gpu_res_hnd = num_gpu_resources;
		num_gpu_resources += 1;

		u32 current_pass_index = render_passes.size() - 1;
		auto& render_pass = render_passes.at(current_pass_index);
		render_pass.pass_read_resources.emplace_back(Render_Pass_Resource{ in_resource_hnd, gpu_res_hnd, bind_flag, slice_index_start, slice_size });
		render_pass.input_resources.push_back(in_resource_hnd);

		return Render_Graph_Resource{in_resource.desc_hnd, in_resource.resource_hnd, gpu_res_hnd , in_resource.ref_count};
	}

	Render_Graph_Resource Render_Graph_Builder::write(Render_Graph_Resource rg_resource, Bind_Flag bind_flag, u32 slice_index_start, u32 slice_size)
	{
		graph_resource_meta.at(rg_resource.desc_hnd).bind_flags |= bind_flag;
		auto in_resource_hnd = rg_resource.resource_hnd;

		if (rg_resource.ref_count == -1)
		{
			graph_resources.at(in_resource_hnd).ref_count = 0;
		}
		
		graph_resources.at(in_resource_hnd).ref_count += 1;
				
		u32 gpu_res_hnd = num_gpu_resources;
		num_gpu_resources += 1;

		//TODO: warning cast of graph_resources size
		u32 out_resource_hnd = (u32)graph_resources.size();
		Render_Graph_Resource out_resource{ rg_resource.desc_hnd, out_resource_hnd, gpu_res_hnd, 0 };
		graph_resources.push_back(out_resource);

		//TODO: update resource version after write?
		u32 current_pass_index = render_passes.size() - 1;
		graph_resources_producers.emplace_back().push_back(current_pass_index);

		auto& render_pass = render_passes.at(current_pass_index);
		render_pass.input_resources.push_back(in_resource_hnd);
		render_pass.pass_write_resources.emplace_back(Render_Pass_Resource{ out_resource_hnd, gpu_res_hnd, bind_flag, slice_index_start, slice_size });
		render_pass.ref_count += 1;

		return out_resource;
	}

	Render_Graph_Resource Render_Graph_Builder::create(const Render_Graph_Resource_Desc& resource_desc)
	{
		u32 index = graph_resource_meta.size();
		graph_resource_meta.emplace_back(Render_Graph_Resource_Metadata{ resource_desc, 0 });

		//TODO: warning! casting size; could introduce bugs
		Render_Graph_Resource rg_resource{ index, (u32)graph_resources.size(), (u32)-1, -1};
		graph_resources.emplace_back(rg_resource);
		graph_resources_producers.emplace_back();

		return rg_resource;
	}
	
	void Render_Graph_Builder::build(std::vector<Render_Resource_Handle>& gpu_resources)
	{
		gpu_resources.clear();
		gpu_resources.resize(num_gpu_resources);
	}

	void Render_Graph_Builder::build_pass()
	{
		auto& render_pass = render_passes.back();
		for (auto& read_reasource : render_pass.pass_read_resources)
		{
			for (auto& write_reasource : render_pass.pass_write_resources)
			{
				read_reasource.view_desc.is_read_only = true;
				if (graph_resources[read_reasource.resource_hnd].desc_hnd == graph_resources[write_reasource.resource_hnd].desc_hnd)
				{
					read_reasource.view_desc.is_read_only = false;
					break;
				}
			}
		}

		int i = 0;
	}

	void Render_Graph_Builder::clear()
	{
		num_gpu_resources = 0;
	}
}