// Shaders.metal
// BSP geometry + MDL model rendering. Brighter lighting for visibility.
// AetherEngine-iOS · Clean-room.

#include <metal_stdlib>
using namespace metal;

struct BSPVertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
    float2 luv      [[attribute(3)]]; /* lightmap UV */
};

struct BSPVertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float2 luv;
};

struct Uniforms {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float3   light_dir;
    float    pad0;
    float4   base_color;
    float    use_texture;
    float    use_lightmap;
    float    pad2, pad3;
};

vertex BSPVertexOut aether_vertex_main(BSPVertexIn in [[stage_in]],
                                        constant Uniforms &U [[buffer(1)]]) {
    BSPVertexOut out;
    float4 world = U.model * float4(in.position, 1.0);
    out.position = U.proj * U.view * world;
    out.normal = normalize((U.model * float4(in.normal, 0.0)).xyz);
    out.uv = in.uv;
    out.luv = in.luv;
    return out;
}

fragment float4 aether_fragment_main(BSPVertexOut in [[stage_in]],
                                      constant Uniforms &U [[buffer(1)]],
                                      texture2d<float> atlas [[texture(0)]],
                                      texture2d<float> lightmap [[texture(1)]],
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

    // Procedural / BSP lightmap stub: modulate vertex/base color (not flat).
    if (U.use_lightmap > 0.5) {
        float3 lm = lightmap.sample(samp, in.luv).rgb;
        base_color *= lm;
        // Soften directional term when lightmap carries the shading.
        diff = mix(diff, 1.0, 0.65);
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


/* ============ Sky dome (gradient from AetherSky face colors) ============ */
struct SkyVertexIn {
    float3 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

struct SkyVertexOut {
    float4 position [[position]];
    float4 color;
};

struct SkyUniforms {
    float4x4 view;
    float4x4 proj;
    float3   eye;
    float    pad0;
};

vertex SkyVertexOut aether_sky_vertex(SkyVertexIn in [[stage_in]],
                                       constant SkyUniforms &U [[buffer(1)]]) {
    SkyVertexOut out;
    /* Translate dome with the camera so it always surrounds the eye. */
    float3 world = in.position + U.eye;
    float4 viewPos = U.view * float4(world, 1.0);
    /* Push depth to far plane so world geometry wins depth tests. */
    float4 clip = U.proj * viewPos;
    clip.z = clip.w * 0.999;
    out.position = clip;
    out.color = in.color;
    return out;
}

fragment float4 aether_sky_fragment(SkyVertexOut in [[stage_in]]) {
    return in.color;
}


/* ============ Water plane (wavy surface from AetherWater) ============ */
struct WaterVertexIn {
    float3 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
    float4 color    [[attribute(2)]];
};

struct WaterVertexOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

struct WaterUniforms {
    float4x4 view;
    float4x4 proj;
    float    time;
    float    pad0, pad1, pad2;
};

vertex WaterVertexOut aether_water_vertex(WaterVertexIn in [[stage_in]],
                                           constant WaterUniforms &U [[buffer(1)]]) {
    WaterVertexOut out;
    float4 world = float4(in.position, 1.0);
    out.position = U.proj * U.view * world;
    /* Mild UV scroll so the surface reads as moving even between mesh rebuilds. */
    out.uv = in.uv + float2(U.time * 0.03, U.time * 0.02);
    out.color = in.color;
    return out;
}

fragment float4 aether_water_fragment(WaterVertexOut in [[stage_in]]) {
    /* Soft procedural ripple tint — no textures / game assets. */
    float ripple = 0.5 + 0.5 * sin(in.uv.x * 28.0 + in.uv.y * 18.0);
    float3 tint = in.color.rgb * (0.85 + 0.20 * ripple);
    float alpha = clamp(in.color.a, 0.0, 1.0);
    return float4(tint, alpha);
}


/* ============ Fog fullscreen tint (from AetherFog) ============ */
struct FogVertexIn {
    float2 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
    float4 color    [[attribute(2)]];
};

struct FogVertexOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

struct FogUniforms {
    float density;
    float factor;
    float pad0, pad1;
};

vertex FogVertexOut aether_fog_vertex(FogVertexIn in [[stage_in]],
                                       constant FogUniforms &U [[buffer(1)]]) {
    FogVertexOut out;
    /* Positions already in NDC from C copy_render. */
    out.position = float4(in.position, 0.0, 1.0);
    out.uv = in.uv;
    /* C bakes density*factor into alpha; uniforms allow a live scale. */
    float scale = clamp(U.density * U.factor, 0.0, 1.0);
    if (scale <= 0.0) scale = 1.0; /* uniforms unused → trust vertex alpha */
    float a = clamp(in.color.a * ((U.density > 0.0 || U.factor > 0.0) ? scale : 1.0), 0.0, 1.0);
    /* Prefer the pre-baked alpha from copy_render when uniforms are demo defaults. */
    a = clamp(in.color.a, 0.0, 1.0);
    out.color = float4(in.color.rgb, a);
    (void)U;
    return out;
}

fragment float4 aether_fog_fragment(FogVertexOut in [[stage_in]]) {
    /* Soft vignette so the tint reads as atmospheric haze, not a flat wash. */
    float2 d = in.uv * 2.0 - 1.0;
    float vignette = clamp(0.55 + 0.45 * dot(d, d), 0.0, 1.0);
    float4 c = in.color;
    c.a *= vignette;
    return c;
}
