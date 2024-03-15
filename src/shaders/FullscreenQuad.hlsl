float4 vs(uint vId : SV_VertexID) : SV_Position
{
    float2 texCoord = float2(vId & 1, vId >> 1);
    return float4((texCoord.x - 0.5f) * 2.0f, -(texCoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);
}