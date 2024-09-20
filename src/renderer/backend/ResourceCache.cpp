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
	constexpr int MAX_NUMBER_MATERIALS = 256;

	namespace wrl = Microsoft::WRL;

	Resource_Cache resource_cache;

	Resource_Cache::Resource_Cache()
	{
		m_material_cache.reserve(MAX_NUMBER_MATERIALS);
		m_material_cache.emplace_back();
		m_diffusion_profiles.reserve(MAX_NUMBER_MATERIALS);
		m_diffusion_profiles.emplace_back();
	}

	Resource_Cache::~Resource_Cache()
	{}

	void Resource_Cache::release_all_resources()
	{
		for (Material& mat : m_material_cache)
		{
			//if (mat.albedo_map) mat.albedo_map->Release();
			//if (mat.smoothness_map) mat.metalness_map->Release();
			//if (mat.smoothness_map) mat.smoothness_map->Release();
			//mat.model.free_shader();
			for (auto& resource : mat.resources)
			{
				switch (resource.m_type)
				{
				case(Resource_Type::ZRY_TEX):
				{
					//TODO: release resources (need to actually move material storage in RendererBackend)
				}
				default:
					break;
				}
			}
		}
	}

	//Material_Cache_Handle_t Resource_Cache::alloc_material(const Reflection_Base* material_desc, Material_Cache_Handle_t& hnd_material_cache)
	//{
	//	Material* m;
	//	auto& standard_material_desc = static_cast<Reflection_Container<Standard_Material_Desc>*>(const_cast<Reflection_Base*>(material_desc))->reflected_struct;
	//	int material_index = hnd_material_cache.index;

	//	if ((hnd_material_cache.is_cached & IS_FIRST_MAT_ALLOC) == IS_FIRST_MAT_ALLOC)
	//	{
	//		if (m_free_material_cache_indices.size() > 0)
	//		{
	//			material_index = m_free_material_cache_indices.back();
	//			m_free_material_cache_indices.pop_back();
	//		}
	//		else
	//		{
	//			m_material_cache.push_back(Material{});
	//			material_index = m_material_cache.size() - 1;
	//		}

	//		m = &m_material_cache.at(material_index);
	//		m->model = Pixel_Shader::create(standard_material_desc.shader_type);
	//	}
	//	else
	//	{
	//		m = &m_material_cache.at(material_index);
	//	}

	//	//m->model.shader = shaders.pixelShaders.at((std::uint8_t)standardMaterialDesc.shaderType);
	//	//ShaderBytecode shaderBytecode = shaders.pixelShaderBytecodeBuffers[(std::uint8_t)standardMaterialDesc.shaderType];

	//	//m->matPrms.hasAlbedoMap = false;
	//	//m->matPrms.hasMetalnessMap = false;
	//	//m->matPrms.hasNormalMap = false;
	//	//m->matPrms.hasSmoothnessMap = false;

	//	if ((hnd_material_cache.is_cached & UPDATE_MAT_MAPS))
	//	{
	//		//char new_path[MAX_PATH];
	//		//wcstombs(new_path, standard_material_desc.albedo_path, MAX_PATH);
	//		//rhi.load_texture2(Texture2D_Asset::create(new_path), &m->albedo_map, true);
	//		rhi.load_texture(standard_material_desc.albedo_path, &m->albedo_map, true);
	//		rhi.load_texture(standard_material_desc.normal_path, &m->normal_map, false);
	//		if ((standard_material_desc.union_tags & METALNESS_IS_MAP) == METALNESS_IS_MAP)
	//		{
	//			rhi.load_texture(standard_material_desc.metalness_map, &m->metalness_map, false);
	//			m->mat_prms.has_metalness_map = true;
	//		}
	//		if ((standard_material_desc.union_tags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP)
	//		{
	//			rhi.load_texture(standard_material_desc.smoothness_map, &m->smoothness_map, false);
	//			m->mat_prms.has_smoothness_map = true;
	//		}
	//	}

	//	if ((hnd_material_cache.is_cached & UPDATE_MAT_PRMS))
	//	{
	//		m->mat_prms.has_albedo_map = m->albedo_map != nullptr;
	//		m->mat_prms.has_normal_map = m->normal_map != nullptr;
	//		m->mat_prms.has_metalness_map = m->metalness_map != nullptr;

	//		m->mat_prms.base_color = standard_material_desc.base_color;
	//		m->mat_prms.sss_model_id = (int)standard_material_desc.selected_sss_model;

	//		switch (standard_material_desc.selected_sss_model)
	//		{
	//		case SSS_MODEL::GOLUBEV:
	//		{
	//			m->sss_prms.subsurface_albedo = standard_material_desc.sss_model.subsurface_albedo;
	//			m->sss_prms.mean_free_path_color = standard_material_desc.sss_model.mean_free_path_color;
	//			m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
	//			m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
	//			m->sss_prms.num_samples = max(1, standard_material_desc.sss_model.num_samples);
	//			m->sss_prms.num_supersamples = max(1, standard_material_desc.sss_model.num_supersamples);

	//			srand(42);

	//			std::random_device rd;
	//			std::mt19937 gen(6.0f);
	//			std::uniform_real_distribution<> dis(0, 1.0);//uniform distribution between 0 and 1

	//			OutputDebugString("Randommly Generated Numbers Golubev:\n");
	//			for (int i = 0; i < 64; i++)
	//			{
	//				m->sss_prms.samples[i] = dis(gen);
	//				OutputDebugString((std::to_string(m->sss_prms.samples[i]).append(" ")).c_str());
	//			}
	//			OutputDebugString("\n");

	//			//int to_next_float4 = 4 - (m->sss_prms.num_samples % 4);
	//			std::sort(m->sss_prms.samples, m->sss_prms.samples + (m->sss_prms.num_samples)); //(int)(max(0.0f, m->sssPrms.scale * 4.0f)));
	//			std::sort(m->sss_prms.samples + 32, m->sss_prms.samples + 32 + m->sss_prms.num_supersamples);
	//			//std::sort(m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)), m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)) + (int)(std::trunc(max(0.0f, standardMaterialDesc.sssModel.subsurfaceAlbedo.y) * 255.0f) * 4.0f));

	//			//for (int i = 0; i < 64; i++) {
	//			//	m->matPrms.samples[i] = (float)(rand() % 10000) / 10000.0f;
	//			//}

	//			break;
	//		}
	//		case SSS_MODEL::JIMENEZ_GAUSS:
	//		{
	//			m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
	//			m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
	//			m->sss_prms.subsurface_albedo = standard_material_desc.sss_model.subsurface_albedo;
	//			break;
	//		}
	//		case SSS_MODEL::JIMENEZ_SEPARABLE:
	//		{
	//			m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
	//			m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
	//			break;
	//		}
	//		default:
	//			if (standard_material_desc.selected_sss_model != SSS_MODEL::NONE)
	//			{
	//				OutputDebugString("ERROR :: ResourceCache.cpp :: Specified invalid sss model\n");
	//			}

	//			m->mat_prms.sss_model_id = 0;

	//			break;
	//		}

	//		if ((standard_material_desc.union_tags & METALNESS_IS_MAP) == 0)
	//		{
	//			m->mat_prms.metalness = standard_material_desc.metalness_value;
	//			m->mat_prms.has_metalness_map = false;
	//		}

	//		if ((standard_material_desc.union_tags & SMOOTHNESS_IS_MAP) == 0)
	//		{
	//			m->mat_prms.roughness = 1.0f - standard_material_desc.smoothness_value;
	//			m->mat_prms.has_smoothness_map = false;
	//		}

	//	}

	//	return Material_Cache_Handle_t{ (std::uint16_t)material_index, NO_UPDATE_MAT };
	//}

	//TODO:implement material dealloc
	void Resource_Cache::dealloc_material(Material_Cache_Handle_t& hnd_material_cache)
	{
		//TODO: what about deallocating the gpu resources (e.g. texture, ...)
		m_free_material_cache_indices.push_back(hnd_material_cache.index);
		//Material& cached_material = m_material_cache.at(hnd_material_cache.index);
		Material& cached_material = m_material_cache.at(hnd_material_cache.index);
		//cached_material.model.free_shader();
		cached_material.shader.free_shader();
	}

	void Resource_Cache::update_material_smoothness(const Material_Cache_Handle_t hnd_material_cache, float smoothness)
	{
		assert(hnd_material_cache.is_cached);
		assert(smoothness >= 0.0f && smoothness <= 1.0f);
		//Material& cached_material = m_material_cache.at(hnd_material_cache.index);
		Material& cached_material = m_material_cache.at(hnd_material_cache.index);
		//cached_material.mat_prms.roughness = smoothness;
	}

	Diffusion_Profile_Handle Resource_Cache::alloc_diffusion_profile(const Asset_Import_Config* ass_imp_config)
	{
		auto& profile = m_diffusion_profiles.emplace_back(Diffusion_Profile(ass_imp_config));
		u16 index = (u16)m_diffusion_profiles.size() - 1;
		profile.hnd = Diffusion_Profile_Handle{ index };

		return profile.hnd;
	}

	//TODO: why if I pass the handle by reference, do I also return a new handle?
	Material_Cache_Handle_t Resource_Cache::alloc_material(const Material_Cache_Handle_t& hnd_material_cache)
	{
		Material* m;
		int material_index = hnd_material_cache.index;

		if ((hnd_material_cache.is_cached & IS_FIRST_MAT_ALLOC) == IS_FIRST_MAT_ALLOC)
		{
			assert(m_material_cache.size() < MAX_NUMBER_MATERIALS);
			m_material_cache.emplace_back();
			material_index = m_material_cache.size() - 1;

			m = &m_material_cache.at(material_index);
			m->asset_type = Asset_Type::MATERIAL;
			m->shader = Pixel_Shader::create(PShader_ID::STANDARD);
			m->resources = m->shader.build_uniforms_struct();
			m->material_hnd = Material_Cache_Handle_t{ (std::uint16_t)material_index, NO_UPDATE_MAT };
			m->shading_model = Shading_Model::DEFAULT_LIT;

		} else
		{
			m = &m_material_cache.at(material_index);
		}

		//m->model.shader = shaders.pixelShaders.at((std::uint8_t)standardMaterialDesc.shaderType);
		//ShaderBytecode shaderBytecode = shaders.pixelShaderBytecodeBuffers[(std::uint8_t)standardMaterialDesc.shaderType];

		//m->matPrms.hasAlbedoMap = false;
		//m->matPrms.hasMetalnessMap = false;
		//m->matPrms.hasNormalMap = false;
		//m->matPrms.hasSmoothnessMap = false;

		//if ((hnd_material_cache.is_cached & UPDATE_MAT_MAPS))
		//{
		//	rhi.load_texture(standard_material_desc.albedo_path, &m->albedo_map, true);
		//	rhi.load_texture(standard_material_desc.normal_path, &m->normal_map, false);
		//	if ((standard_material_desc.union_tags & METALNESS_IS_MAP) == METALNESS_IS_MAP)
		//	{
		//		rhi.load_texture(standard_material_desc.metalness_map, &m->metalness_map, false);
		//		m->mat_prms.has_metalness_map = true;
		//	}
		//	if ((standard_material_desc.union_tags & SMOOTHNESS_IS_MAP) == SMOOTHNESS_IS_MAP)
		//	{
		//		rhi.load_texture(standard_material_desc.smoothness_map, &m->smoothness_map, false);
		//		m->mat_prms.has_smoothness_map = true;
		//	}
		//}

		//if ((hnd_material_cache.is_cached & UPDATE_MAT_PRMS))
		//{
		//	m->mat_prms.has_albedo_map = m->albedo_map != nullptr;
		//	m->mat_prms.has_normal_map = m->normal_map != nullptr;
		//	m->mat_prms.has_metalness_map = m->metalness_map != nullptr;

		//	m->mat_prms.base_color = standard_material_desc.base_color;
		//	m->mat_prms.sss_model_id = (int)standard_material_desc.selected_sss_model;

		//	switch (standard_material_desc.selected_sss_model)
		//	{
		//	case SSS_MODEL::GOLUBEV:
		//	{
		//		m->sss_prms.subsurface_albedo = standard_material_desc.sss_model.subsurface_albedo;
		//		m->sss_prms.mean_free_path_color = standard_material_desc.sss_model.mean_free_path_color;
		//		m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
		//		m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
		//		m->sss_prms.num_samples = max(1, standard_material_desc.sss_model.num_samples);
		//		m->sss_prms.num_supersamples = max(1, standard_material_desc.sss_model.num_supersamples);

		//		srand(42);

		//		std::random_device rd;
		//		std::mt19937 gen(6.0f);
		//		std::uniform_real_distribution<> dis(0, 1.0);//uniform distribution between 0 and 1

		//		OutputDebugString("Randommly Generated Numbers Golubev:\n");
		//		for (int i = 0; i < 64; i++)
		//		{
		//			m->sss_prms.samples[i] = dis(gen);
		//			OutputDebugString((std::to_string(m->sss_prms.samples[i]).append(" ")).c_str());
		//		}
		//		OutputDebugString("\n");

		//		//int to_next_float4 = 4 - (m->sss_prms.num_samples % 4);
		//		std::sort(m->sss_prms.samples, m->sss_prms.samples + (m->sss_prms.num_samples)); //(int)(max(0.0f, m->sssPrms.scale * 4.0f)));
		//		std::sort(m->sss_prms.samples + 32, m->sss_prms.samples + 32 + m->sss_prms.num_supersamples);
		//		//std::sort(m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)), m->sssPrms.samples + (int)(max(0.0f, m->sssPrms.scale * 4.0f)) + (int)(std::trunc(max(0.0f, standardMaterialDesc.sssModel.subsurfaceAlbedo.y) * 255.0f) * 4.0f));

		//		//for (int i = 0; i < 64; i++) {
		//		//	m->matPrms.samples[i] = (float)(rand() % 10000) / 10000.0f;
		//		//}

		//		break;
		//	}
		//	case SSS_MODEL::JIMENEZ_GAUSS:
		//	{
		//		m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
		//		m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
		//		m->sss_prms.subsurface_albedo = standard_material_desc.sss_model.subsurface_albedo;
		//		break;
		//	}
		//	case SSS_MODEL::JIMENEZ_SEPARABLE:
		//	{
		//		m->sss_prms.scale = fmax(0.0f, standard_material_desc.sss_model.scale);
		//		m->sss_prms.mean_free_path_dist = standard_material_desc.sss_model.mean_free_path_distance; //from cm to m
		//		break;
		//	}
		//	default:
		//		if (standard_material_desc.selected_sss_model != SSS_MODEL::NONE)
		//		{
		//			OutputDebugString("ERROR :: ResourceCache.cpp :: Specified invalid sss model\n");
		//		}

		//		m->mat_prms.sss_model_id = 0;

		//		break;
		//	}

		//	if ((standard_material_desc.union_tags & METALNESS_IS_MAP) == 0)
		//	{
		//		m->mat_prms.metalness = standard_material_desc.metalness_value;
		//		m->mat_prms.has_metalness_map = false;
		//	}

		//	if ((standard_material_desc.union_tags & SMOOTHNESS_IS_MAP) == 0)
		//	{
		//		m->mat_prms.roughness = 1.0f - standard_material_desc.smoothness_value;
		//		m->mat_prms.has_smoothness_map = false;
		//	}

		//}

		return Material_Cache_Handle_t{ (std::uint16_t)material_index, NO_UPDATE_MAT };
	}


	void Resource_Cache::update_material_albedo_map(const Material_Cache_Handle_t hnd_material_cache, const wchar_t* albedo_map_path)
	{}

}