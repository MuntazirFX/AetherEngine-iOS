// Shaders.metal
// BSP geometry + MDL model rendering. STEP 16C.
// AetherEngine-iOS · Clean-room.

#include <metal_stdlib>
using namespace metal;

/* ============ Shared uniform struct ============ */
struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3   light_dir;
    float    pad0;
    float4   base_color;
    float    use_texture;
    float    pad1, pad2, pad3;
};

/* ============ BSP shaders ============ */
struct BspVertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
};

struct BspVertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
};

vertex BspVertexOut aether_vertex_main(BspVertexIn in [[stage_in]],
                                        constant Uniforms &U [[buffer(1)]]) {
    BspVertexOut out;
    float4 world = U.model * float4(in.position, 1.0);
    out.position = U.proj * U.view * world;
    out.normal = normalize((U.model * float4(in.normal, 0.0)).xyz);
    out.uv = in.uv;
    return out;
}

fragment float4 aether_fragment_main(BspVertexOut in [[stage_in]],
                                      constant Uniforms &U [[buffer(1)]],
                                      texture2d<float> atlas [[texture(0)]],
                                      sampler samp [[sampler(0)]]) {
    float3 N = normalize(in.normal);
    float  ndl = max(dot(N, normalize(U.light_dir)), 0.0);
    float  ambient = 0.30;
    float  diff = ambient + ndl * 0.70;

    float3 base_color;
    if (U.use_texture > 0.5) {
        float4 tex = atlas.sample(samp, in.uv);
        if (tex.a < 0.5) base_color = U.base_color.rgb;
        else             base_color = tex.rgb;
    } else {
        float3 tint = float3(0.5 + 0.5*N.x, 0.5 + 0.5*N.y, 0.5 + 0.5*N.z);
        base_color = U.base_color.rgb * tint;
    }

    float3 color = base_color * diff;
    return float4(color, 1.0);
}

/* ============ MDL model shaders (STEP 16C) ============ */
struct MdlVertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
};

struct MdlVertexOut {
    float4 position [[position]];
    float3 normal;
};

vertex MdlVertexOut aether_model_vertex(MdlVertexIn in [[stage_in]],
                                         constant Uniforms &U [[buffer(1)]]) {
    MdlVertexOut out;
    /* GoldSrc MDL models are Z-up; swap to our world axes. */
    float3 p = float3(in.position.x, in.position.z, in.position.y);
    float3 n = float3(in.normal.x,   in.normal.z,   in.normal.y);

    float4 world = U.model * float4(p, 1.0);
    out.position = U.proj * U.view * world;
    out.normal = normalize((U.model * float4(n, 0.0)).xyz);
    return out;
}

fragment float4 aether_model_fragment(MdlVertexOut in [[stage_in]],
                                       constant Uniforms &U [[buffer(1)]]) {
    float3 N = normalize(in.normal);
    float3 L = normalize(U.light_dir);
    float  ndl = max(dot(N, L), 0.0);

    float3 base = U.base_color.rgb;
    float3 color = base * (0.40 + 0.60 * ndl);

    /* Simple rim light for silhouette highlight */
    float rim = 1.0 - max(dot(N, float3(0, 0, 1)), 0.0);
    color += float3(0.15, 0.20, 0.30) * rim * 0.5;

    return float4(color, 1.0);
}
