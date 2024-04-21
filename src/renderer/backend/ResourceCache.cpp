#include "renderer/backend/ResourceCache.h"
#include "renderer/backend/rhi/RenderHardwareInterface.h"

#include "renderer/frontend/Shader.h"
#include "renderer/frontend/Material.h"

#include <d3d11_1.h>
#include <wrl/client.h>
#include <cstdint>
#include <cassert>
#include <variant>
#include <cstdlib>
#include <iostream>
#include <random>

namespace zorya
{
	namespace wrl = Microsoft::WRL;

	Resource_Cache resource_cache;

	Resource_Cache::Resource_Cache()
	{}

	Resource_Cache::~Resource_Cache()
	{}

	void Resource_Cache::release_all_resources()
	{
		for (Material& mat : m_material_cache)
		{
			if (mat.albedo_map.resource_view) mat.albedo_map.resource_view->Release();
			if (mat.smoothness_map.resource_view) mat.metalness_map.resource_view->Release();
			if (mat.smoothness_map.resource_view) mat.smoothness_map.resource_view->Release();
			mat.model.free_shader();
		}
	}

	Material_Cache_Handle_t Resource_Cache::alloc_material(const Reflection_Base* material_desc, Material_Cache_Handle_t& hnd_material_cache)
	{
		Material* m;
		auto& standard_material_desc = static_cast<Reflection_Container<Standard_Material_Desc>*>(const_cast<Reflection_Base*>(material_desc))->reflected_struct;
		int material_index = hnd_material_cache.index;

		if ((hnd_material_cache.is_cached & IS_FIRST_MAT_ALLOC) == IS_FIRST_MAT_ALLOC)
		{
			if (m_free_material_cache_indices.size() > 0)
			{
				material_index = m_free_material_cache_indices.back();
				m_free_material_cache_indices.pop_back();
			}
			else
			{
				m_material_cache.push_back(Material{});
				material_index = m_material_cache.size() - 1;
			}

			m = &m_material_cache.at(material_index);
			m->model = Pixel_Shader::create(standard_material_desc.shader_type);
		}
		else
		{
			m = &m_material_cache.at(material_index);
		}

		//m->model.shader = shaders.pixelShaders.at((std::uint8_t)standardMaterialDesc.shaderType);
		//ShaderBytecode shaderBytecode = shaders.pixelShaderBytecodeBuffers[(std::uint8_t)standardMaterialDesc.shaderType];

		//m->matPrms.hasAlbedoMap = false;
		//m->matPrms.hasMetalnessMap = false;
		//m->matPrms.hasNormalMap = false;
		//m->matPrms.hasSmoothnessMap = false;

		if ((hnd_material_cache.is_cached & UPDATE_MAT_MAPS))
		{
			rhi.load_texture(standard_material_desc.albedo_path, m->albedo_map, false);
			rhi.load_texture(standard_material_desc.normal_path, m->normal_map, false);
			if ((standard_material_desc.union_tags & METALNESS_IS_MAP) == METALNESS_IS_MAP)
			{
				rhi.load_texture(standard_material_desc.metalness_map, m->metalness_map, false);
				m->mat_prms.has_metalness_map = true;
			}
			if ((standard_material_desc.union_tags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP)
			{
				rhi.load_texture(standard_material_desc.smoothness_map, m->smoothness_map, false);
				m->mat_prms.has_smoothness_map = true;
			}
		}

		if ((hnd_material_cache.is_cached & UPDATE_MAT_PRMS))
		{
			m->mat_prms.has_albedo_map = m->albedo_map.resource_view != nullptr;
			m->mat_prms.has_normal_map = m->normal_map.resource_view != nullptr;
			m->mat_prms.has_metalness_map = m->metalness_map.resource_view != nullptr;

			m->mat_prms.base_color = standard_material_desc.base_color;
			m->mat_prms.sss_model_id = (int)standard_material_desc.selected_sss_model;

			switch (standard_material_desc.selected_sss_model)
			{
			case SSS_MODEL::GOLUBEV:
			{
				m->sss_prms.subsurface_albedo = standard_material_desc.sss_model.subsurface_albedo;
				m->sss_prms.mean_free_path_color = standard_material_desc.sss_model.mean_free_path_color;
				m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
				m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
				m->sss_prms.num_samples = max(1, standard_material_desc.sss_model.num_samples);

				srand(42);

				std::random_device rd;
				std::mt19937 gen(6.0f);
				std::uniform_real_distribution<> dis(0, 1.0);//uniform distribution between 0 and 1

				for (int i = 0; i < m->sss_prms.num_samples; i++)
				{
					m->sss_prms.samples[i] = dis(gen);
					OutputDebugString((std::to_string(m->sss_prms.samples[i]).append(", ")).c_str());
				}

				std::sort(m->sss_prms.samples, m->sss_prms.samples + (m->sss_prms.num_samples - 1)); //(int)(max(0.0f, m->sssPrms.scale * 4.0f)));
				//std::sort(m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)), m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)) + (int)(std::trunc(max(0.0f, standardMaterialDesc.sssModel.subsurfaceAlbedo.y) * 255.0f) * 4.0f));

				//for (int i = 0; i < 64; i++) {
				//	m->matPrms.samples[i] = (float)(rand() % 10000) / 10000.0f;
				//}

				break;
			}
			default:
			{
				m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
				m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
				break;
			}

			}

			if ((standard_material_desc.union_tags & METALNESS_IS_MAP) == 0)
			{
				m->mat_prms.metalness = standard_material_desc.metalness_value;
				m->mat_prms.has_metalness_map = false;
			}

			if ((standard_material_desc.union_tags & SMOOTHNESS_IS_MAP) == 0)
			{
				m->mat_prms.roughness = 1.0f - standard_material_desc.smoothness_value;
				m->mat_prms.has_smoothness_map = false;
			}

		}

		return Material_Cache_Handle_t{ (std::uint16_t)material_index, NO_UPDATE_MAT };
	}

	//TODO:implement material dealloc
	void Resource_Cache::dealloc_material(Material_Cache_Handle_t& hnd_material_cache)
	{
		//TODO: what about deallocating the gpu resources (e.g. texture, ...)
		m_free_material_cache_indices.push_back(hnd_material_cache.index);
		Material& cached_material = m_material_cache.at(hnd_material_cache.index);
		cached_material.model.free_shader();
	}

	void Resource_Cache::update_material_smoothness(const Material_Cache_Handle_t hnd_material_cache, float smoothness)
	{
		assert(hnd_material_cache.is_cached);
		assert(smoothness >= 0.0f && smoothness <= 1.0f);
		Material& cached_material = m_material_cache.at(hnd_material_cache.index);
		cached_material.mat_prms.roughness = smoothness;
	}

	void Resource_Cache::update_material_albedo_map(const Material_Cache_Handle_t hnd_material_cache, const wchar_t* albedo_map_path)
	{}

}