#ifndef RESOURCE_CACHE_H_
#define RESOURCE_CACHE_H_

#include <vector>
#include <cstdint>

#include "renderer/frontend/Material.h"
#include "renderer/frontend/DiffusionProfile.h"
#include "renderer/frontend/FrontendHandles.h"

#include "reflection/Reflection.h"

namespace zorya
{

	class Resource_Cache
	{
	public:
		Resource_Cache();
		~Resource_Cache();

		void release_all_resources();

		//Material_Cache_Handle_t alloc_material(const Reflection_Base* mat_desc, Material_Cache_Handle_t& update_opts);
		Diffusion_Profile_Handle alloc_diffusion_profile(const Asset_Import_Config* ass_imp_config);

		Material_Cache_Handle_t alloc_material(const Material_Cache_Handle_t& update_opts);
		void dealloc_material(Material_Cache_Handle_t& hnd_material_cache);
		void update_material_smoothness(const Material_Cache_Handle_t hnd_material_cache, float smoothness);
		void update_material_albedo_map(const Material_Cache_Handle_t hnd_material_cache, const wchar_t* albedo_map_path);

		std::vector<Material> m_material_cache;
		std::vector<Diffusion_Profile> m_diffusion_profiles;

	private:
		std::vector<int> m_free_material_cache_indices;

	};

	extern Resource_Cache resource_cache;

}
#endif