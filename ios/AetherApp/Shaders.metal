// Shaders.metal
// BSP geometry + MDL model rendering. Brighter lighting for visibility.
// AetherEngine-iOS · Clean-room.

#include <metal_stdlib>
using namespace metal;

struct BSPVertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
};

struct BSPVertexOut {
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
    float    use_texture;
    float    pad1, pad2, pad3;
};

vertex BSPVertexOut aether_vertex_main(BSPVertexIn in [[stage_in]],
                                        constant Uniforms &U [[buffer(1)]]) {
    BSPVertexOut out;
    float4 world = U.model * float4(in.position, 1.0);
    out.position = U.proj * U.view * world;
    out.normal = normalize((U.model * float4(in.normal, 0.0)).xyz);
    out.uv = in.uv;
    return out;
}

fragment float4 aether_fragment_main(BSPVertexOut in [[stage_in]],
                                      constant Uniforms &U [[buffer(1)]],
                                      texture2d<float> atlas [[texture(0)]],
                                      sampler samp [[sampler(0)]]) {
    // Brighter lighting — changed ambient 0.30 → 0.55, diffuse 0.70 → 0.60
    float3 N = normalize(in.normal);
    float  ndl = max(dot(N, normalize(U.light_dir)), 0.0);
    // Two-sided lighting: if back-facing, flip normal
    float  ndl_abs = max(abs(dot(N, normalize(U.light_dir))), 0.0);
    float  ambient = 0.55;
    float  diff = ambient + ndl_abs * 0.60;
    // Clamp to 1.0
    if (diff > 1.0) diff = 1.0;

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

/* ============ MDL model shaders ============ */
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
    float  ndl_abs = max(abs(dot(N, L)), 0.0);

    // Brighter: 0.60 ambient + 0.50 diffuse
    float3 base = U.base_color.rgb;
    float3 color = base * (0.60 + 0.50 * ndl_abs);
    if (color.r > 1.0) color.r = 1.0;
    if (color.g > 1.0) color.g = 1.0;
    if (color.b > 1.0) color.b = 1.0;

    // Rim light
    float rim = 1.0 - abs(dot(N, float3(0,0,1)));
    color += float3(0.10, 0.15, 0.20) * rim * 0.5;

    return float4(color, 1.0);
}

/* ============ Particle point sprites (engine state driven) ============ */
struct ParticleVertexIn {
    float3 position [[attribute(0)]];
    float  size     [[attribute(1)]];
    float4 color    [[attribute(2)]];
};

struct ParticleVertexOut {
    float4 position [[position]];
    float  pointSize [[point_size]];
    float4 color;
};

struct ParticleUniforms {
    float4x4 view;
    float4x4 proj;
};

vertex ParticleVertexOut aether_particle_vertex(ParticleVertexIn in [[stage_in]],
                                                 constant ParticleUniforms &U [[buffer(1)]]) {
    ParticleVertexOut out;
    float4 clip = U.proj * U.view * float4(in.position, 1.0);
    out.position = clip;
    /* Perspective-aware point size in pixels (placeholder scale). */
    float w = max(abs(clip.w), 1.0);
    out.pointSize = clamp(in.size * 180.0 / w, 2.0, 48.0);
    out.color = in.color;
    return out;
}

fragment float4 aether_particle_fragment(ParticleVertexOut in [[stage_in]],
                                          float2 pc [[point_coord]]) {
    float2 d = pc * 2.0 - 1.0;
    float r2 = dot(d, d);
    if (r2 > 1.0) discard_fragment();
    float soft = 1.0 - r2;
    float4 c = in.color;
    c.a *= soft * soft;
    return c;
}
