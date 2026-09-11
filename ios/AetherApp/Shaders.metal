// Shaders.metal
// Solid-color BSP geometry rendering. STEP 12.
// AetherEngine-iOS · Clean-room.

#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
};

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3   light_dir;
    float    pad0;
    float4   base_color;
};

vertex VertexOut aether_vertex_main(VertexIn in [[stage_in]],
                                     constant Uniforms &U [[buffer(1)]]) {
    VertexOut out;
    float4 world = U.model * float4(in.position, 1.0);
    out.position = U.proj * U.view * world;
    out.normal = normalize((U.model * float4(in.normal, 0.0)).xyz);
    out.uv = in.uv;
    return out;
}

fragment float4 aether_fragment_main(VertexOut in [[stage_in]],
                                      constant Uniforms &U [[buffer(1)]]) {
    // Simple Lambert lighting with a fixed light direction.
    float3 N = normalize(in.normal);
    float  ndl = max(dot(N, normalize(U.light_dir)), 0.0);
    float  ambient = 0.25;
    float  diff = ambient + ndl * 0.75;
    // Slight tint by normal for debug — R/G/B per axis.
    float3 tint = float3(0.5 + 0.5 * N.x, 0.5 + 0.5 * N.y, 0.5 + 0.5 * N.z);
    float3 color = U.base_color.rgb * tint * diff;
    return float4(color, 1.0);
}
