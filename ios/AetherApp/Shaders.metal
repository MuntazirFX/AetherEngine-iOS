// Shaders.metal
// Texture-mapped BSP geometry rendering. STEP 15B-2.
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
    float    use_texture;   // 0 = solid colour, 1 = sample atlas
    float    pad1, pad2, pad3;
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
                                      constant Uniforms &U [[buffer(1)]],
                                      texture2d<float> atlas [[texture(0)]],
                                      sampler samp [[sampler(0)]]) {
    // Simple Lambert lighting
    float3 N = normalize(in.normal);
    float  ndl = max(dot(N, normalize(U.light_dir)), 0.0);
    float  ambient = 0.30;
    float  diff = ambient + ndl * 0.70;

    float3 base_color;
    if (U.use_texture > 0.5) {
        // Sample atlas at baked UV
        float4 tex = atlas.sample(samp, in.uv);
        // If UV wrapped outside, or alpha is 0 (empty atlas slot), fallback
        if (tex.a < 0.5) {
            base_color = U.base_color.rgb;
        } else {
            base_color = tex.rgb;
        }
    } else {
        // Fallback: normal-based tint for debug
        float3 tint = float3(0.5 + 0.5 * N.x, 0.5 + 0.5 * N.y, 0.5 + 0.5 * N.z);
        base_color = U.base_color.rgb * tint;
    }

    float3 color = base_color * diff;
    return float4(color, 1.0);
}
