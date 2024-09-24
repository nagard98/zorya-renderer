#ifndef PIPELINE_STATE_MANAGER_H_
#define PIPELINE_STATE_MANAGER_H_

#include <vector>
#include "renderer/backend/rhi/RenderDeviceHandles.h"
#include "renderer/frontend/Material.h"

namespace zorya
{
	enum Pipeline_State
	{
		LIGHTING,
		SKYBOX,
		SHADOW_MAPPING,
		SHADOW_MASK,
		COMPOSIT,
		SSS_GOLUBEV,
		SSS_JIMENEZ_SEPARABLE,
		SSS_JIMENEZ_GAUSS_HOR,
		SSS_JIMENEZ_GAUSS_VERT,
		NUM_PIPELINE_STATES
	};


	class Pipeline_State_Manager
	{
	public:
		Pipeline_State_Manager();

		HRESULT init();
		void shutdown();

		PSO_Handle get(Shading_Model shading_model_id) const;
		PSO_Handle get(Pipeline_State pipeline_state_id) const;

	private:
		std::vector<PSO_Handle> m_shading_models;
		std::vector<PSO_Handle> m_pipeline_states;
	};


	extern Pipeline_State_Manager pipeline_state_manager;
}

#endif // !PIPELINE_STATE_MANAGER_H_
