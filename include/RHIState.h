#ifndef RBSTATE_H_
#define RBSTATE_H_

#include <cstdint>

typedef std::uint64_t RHIState;


constexpr int MAX_STATE_VARIANTS = 7;

constexpr RHIState RASTER_STATE_MASK = 127;

constexpr RHIState RASTER_CULL_MODE_MASK = 3 << 0;
constexpr RHIState RASTER_CULL_FRONT = 0;
constexpr RHIState RASTER_CULL_BACK = 1;
constexpr RHIState RASTER_CULL_NONE = 2;

constexpr RHIState RASTER_FILL_MODE_MASK = 1 << 2;
constexpr RHIState RASTER_FILL_MODE_NEG_MASK = ~(1 << 2);
constexpr RHIState RASTER_FILL_MODE_SOLID = 0 << 2;
constexpr RHIState RASTER_FILL_MODE_WIREFRAME = 1 << 2;

constexpr RHIState RASTER_ANTIALIAS_MASK = 1 << 3;
constexpr RHIState RASTER_ANTIALIAS_NEG_MASK = ~(1 << 3);

constexpr RHIState RASTER_FCW_MASK = 1 << 4;
constexpr RHIState RASTER_FCW_NEG_MASK = ~(1 << 4);

constexpr RHIState RASTER_MULTISAMPLE_MASK = 1 << 5;
constexpr RHIState RASTER_MULTISAMPLE_NEG_MASK = ~(1 << 5);

constexpr RHIState RASTER_SCISSOR_MASK = 1 << 6;
constexpr RHIState RASTER_SCISSOR_NEG_MASK = ~(1 << 6);

constexpr RHIState DEPTH_STENCIL_STATE_MASK = RHIState(0xffffff) << 9;

constexpr RHIState DEPTH_ENABLE_MASK = 1 << 9;
constexpr RHIState DEPTH_ENABLE_NEG_MASK = ~(1 << 9);

constexpr RHIState DEPTH_WRITE_ENABLE_MASK = 1 << 10;
constexpr RHIState DEPTH_WRITE_ENABLE_NEG_MASK = ~(1 << 10);

constexpr RHIState DEPTH_COMP_MASK = 0xf << 11;
constexpr RHIState DEPTH_COMP_LESS = 0 << 11;
constexpr RHIState DEPTH_COMP_LESS_NEG = ~(1 << 11);
constexpr RHIState DEPTH_COMP_LESS_EQ = 1 << 11;

#define RHI_RS_SET_CULL_FRONT(state) state&=(~RASTER_CULL_BACK);
#define RHI_RS_SET_CULL_FRONT_RET(state) state&(~RASTER_CULL_BACK);
#define RHI_RS_SET_CULL_BACK(state) state|=RASTER_CULL_BACK;
#define RHI_RS_SET_CULL_BACK_RET(state) state|RASTER_CULL_BACK;

#define RHI_RS_SET_SOLID_FILL(state) state&=RASTER_FILL_MODE_NEG_MASK;
#define RHI_RS_SET_SOLID_FILL_RET(state) state&RASTER_FILL_MODE_NEG_MASK;

#define RHI_RS_SET_WIREFRAME_FILL(state) state|=RASTER_FILL_MODE_MASK;
#define RHI_RS_SET_WIREFRAME_FILL_RET(state) state|RASTER_FILL_MODE_MASK;

#define RHI_RS_ENABLE_ANTIALIAS(state) state|=RASTER_ANTIALIAS_MASK;
#define RHI_RS_ENABLE_ANTIALIAS_RET(state) state|RASTER_ANTIALIAS_MASK;

#define RHI_RS_DISABLE_ANTIALIAS(state) state&=RASTER_ANTIALIAS_NEG_MASK;
#define RHI_RS_DISABLE_ANTIALIAS_RET(state) state&RASTER_ANTIALIAS_NEG_MASK;

#define RHI_RS_SET_FCW(state) state|=RASTER_FCW_MASK;
#define RHI_RS_SET_FCW_RET(state) state|RASTER_FCW_MASK;

#define RHI_RS_SET_FCCW(state) state&=RASTER_FCW_NEG_MASK;
#define RHI_RS_SET_FCCW_RET(state) state&RASTER_FCW_NEG_MASK;

#define RHI_RS_ENABLE_MULTISAMPLE(state) state|=RASTER_MULTISAMPLE_MASK;
#define RHI_RS_ENABLE_MULTISAMPLE_RET(state) state|RASTER_MULTISAMPLE_MASK;

#define RHI_RS_DISABLE_MULTISAMPLE(state) state&=RASTER_MULTISAMPLE_NEG_MASK;
#define RHI_RS_DISABLE_MULTISAMPLE_RET(state) state&RASTER_MULTISAMPLE_NEG_MASK;

#define RHI_RS_ENABLE_SCISSOR(state) state|=RASTER_SCISSOR_MASK;
#define RHI_RS_ENABLE_SCISSOR_RET(state) state|RASTER_SCISSOR_MASK;

#define RHI_RS_DISABLE_SCISSOR(state) state&=RASTER_SCISSOR_NEG_MASK;
#define RHI_RS_DISABLE_SCISSOR_RET(state) state&RASTER_SCISSOR_NEG_MASK;

#define RHI_OM_DS_ENABLE_DEPTH(state) state|=DEPTH_ENABLE_MASK;
#define RHI_OM_DS_ENABLE_DEPTH_RET(state) state|DEPTH_ENABLE_MASK;

#define RHI_OM_DS_DISABLE_DEPTH(state) state&=DEPTH_ENABLE_NEG_MASK;
#define RHI_OM_DS_DISABLE_DEPTH_RET(state) state&DEPTH_ENABLE_NEG_MASK;

#define RHI_OM_DS_SET_DEPTH_COMP_LESS(state) state&=DEPTH_COMP_LESS_NEG;
#define RHI_OM_DS_SET_DEPTH_COMP_LESS_RET(state) state&DEPTH_COMP_LESS_NEG;

#define RHI_OM_DS_SET_DEPTH_COMP_LESS_EQ(state) state|=DEPTH_COMP_LESS_EQ;
#define RHI_OM_DS_SET_DEPTH_COMP_LESS_EQ_RET(state) state|DEPTH_COMP_LESS_EQ;

#define RHI_OM_DS_ENABLE_DEPTH_WRITE(state) state|=DEPTH_WRITE_ENABLE_MASK;
#define RHI_OM_DS_ENABLE_DEPTH_WRITE_RET(state) state|DEPTH_WRITE_ENABLE_MASK;

#define RHI_OM_DS_DISABLE_DEPTH_WRITE(state) state&=DEPTH_WRITE_ENABLE_NEG_MASK;
#define RHI_OM_DS_DISABLE_DEPTH_WRITE_RET(state) state&DEPTH_WRITE_ENABLE_NEG_MASK;

extern constexpr RHIState RHI_DEFAULT_STATE();
extern RHIState rhiState;


#endif