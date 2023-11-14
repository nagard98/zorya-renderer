#include "RHIState.h"


constexpr RHIState RHI_DEFAULT_STATE() {
	constexpr RHIState cullMode = RHI_RS_SET_CULL_BACK_RET(0);
	constexpr RHIState fillMode = RHI_RS_SET_SOLID_FILL_RET(0);
	constexpr RHIState antialiasing= RHI_RS_DISABLE_ANTIALIAS_RET(0);
	constexpr RHIState fcw = RHI_RS_SET_FCW_RET(0);
	constexpr RHIState multisample = RHI_RS_DISABLE_MULTISAMPLE_RET(0);
	constexpr RHIState scissor = RHI_RS_DISABLE_SCISSOR_RET(0);

	constexpr RHIState depthEnable = RHI_OM_DS_ENABLE_DEPTH_RET(0);
	constexpr RHIState depthWriteEnable = RHI_OM_DS_ENABLE_DEPTH_WRITE_RET(0);

	return (cullMode | fillMode | antialiasing | fcw | multisample | scissor | depthEnable | depthWriteEnable );
}


RHIState rhiState = RHI_DEFAULT_STATE();

