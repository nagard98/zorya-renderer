#include "PipelineStateObject.h"
#include "xxhash.h"

uint64_t zorya::hash(const PSO_Desc& pso_desc)
{
	XXH64_state_t* state = XXH64_createState();

	const XXH64_hash_t seed = 0;
	if(XXH64_reset(state, seed) == XXH_ERROR) abort();

	if (XXH64_update(state, pso_desc.pixel_shader_bytecode.bytecode, pso_desc.pixel_shader_bytecode.size_in_bytes) == XXH_ERROR) abort();
	if (XXH64_update(state, pso_desc.vertex_shader_bytecode.bytecode, pso_desc.vertex_shader_bytecode.size_in_bytes) == XXH_ERROR) abort();
	if (XXH64_update(state, &pso_desc.depth_stencil_desc, sizeof(pso_desc.depth_stencil_desc)) == XXH_ERROR) abort();
	if (XXH64_update(state, &pso_desc.input_elements_desc, sizeof(*pso_desc.input_elements_desc) * pso_desc.num_elements) == XXH_ERROR) abort();
	if (XXH64_update(state, &pso_desc.stencil_ref_value, sizeof(pso_desc.stencil_ref_value)) == XXH_ERROR) abort();
	if (XXH64_update(state, &pso_desc.blend_desc, sizeof(pso_desc.blend_desc)) == XXH_ERROR) abort();

	const XXH64_hash_t hash = XXH64_digest(state);

	return hash;
}
