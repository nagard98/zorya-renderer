cbuffer camera_cb : register(b1)
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 proj_matrix;
};

float4 vs(float4 vert_pos : POSITION) : SV_Position
{
    return mul(vert_pos, mul(world_matrix, mul(view_matrix, proj_matrix)));
}