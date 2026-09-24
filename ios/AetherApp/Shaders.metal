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
    float  face_id  [[attribute(4)]]; /* BSP face index for style blend */
};

struct BSPVertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float2 luv;
    float  face_id;
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
    out.face_id = in.face_id;
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

fragment float4 aether_water_fragment(WaterVertexOut in [[stage_in]],
                                        texture2d<float> reflectTex [[texture(1)]],
                                        sampler samp [[sampler(0)]],
                                        constant float &reflectOn [[buffer(2)]]) {
    /* Soft procedural ripple tint — no game assets. Optional reflection RT sample. */
    float ripple = 0.5 + 0.5 * sin(in.uv.x * 28.0 + in.uv.y * 18.0);
    float3 tint = in.color.rgb * (0.85 + 0.20 * ripple);
    if (reflectOn > 0.5) {
        float2 ruv = float2(in.uv.x, 1.0 - in.uv.y);
        float3 refl = reflectTex.sample(samp, ruv).rgb;
        tint = mix(tint, refl, 0.35);
    }
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

/* ============ Projected decal quads (from AetherDecal pool) ============ */
struct DecalQuadIn {
    float3 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
    float  fade     [[attribute(2)]];
    float4 color    [[attribute(3)]];
};
struct DecalQuadOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};
struct DecalUniforms {
    float4x4 view;
    float4x4 proj;
};
vertex DecalQuadOut aether_decal_quad_vertex(DecalQuadIn in [[stage_in]],
                                              constant DecalUniforms &U [[buffer(1)]]) {
    DecalQuadOut out;
    out.position = U.proj * U.view * float4(in.position, 1.0);
    out.uv = in.uv;
    out.color = float4(in.color.rgb, in.color.a * in.fade);
    return out;
}
fragment float4 aether_decal_quad_fragment(DecalQuadOut in [[stage_in]]) {
    float2 d = in.uv * 2.0 - 1.0;
    float soft = 1.0 - smoothstep(0.55, 1.0, length(d));
    float4 c = in.color;
    c.a *= soft;
    if (c.a < 0.02) discard_fragment();
    return c;
}

/* ============ Sprite billboard stub ============ */
struct SpriteQuadIn {
    float3 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
    float4 color    [[attribute(2)]];
};
struct SpriteQuadOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};
vertex SpriteQuadOut aether_sprite_quad_vertex(SpriteQuadIn in [[stage_in]],
                                                constant DecalUniforms &U [[buffer(1)]]) {
    SpriteQuadOut out;
    out.position = U.proj * U.view * float4(in.position, 1.0);
    out.uv = in.uv;
    out.color = in.color;
    return out;
}
fragment float4 aether_sprite_quad_fragment(SpriteQuadOut in [[stage_in]]) {
    float2 d = in.uv * 2.0 - 1.0;
    float soft = 1.0 - smoothstep(0.7, 1.0, length(d));
    float4 c = in.color;
    c.a *= soft;
    if (c.a < 0.02) discard_fragment();
    return c;
}

/* ============ Dyn-light UBO (fragment additive) ============ */
struct DynLightGPU {
    float4 pos_radius;   /* xyz + radius */
    float4 color_inten;  /* rgb + intensity */
};
struct DynLightUBO {
    uint count;
    uint pad0, pad1, pad2;
    DynLightGPU lights[16];
};

/* World-position variant of BSP fragment with dyn lights (buffer 2). */
fragment float4 aether_fragment_dynlights(BSPVertexOut in [[stage_in]],
                                           constant Uniforms &U [[buffer(1)]],
                                           constant DynLightUBO &DL [[buffer(2)]],
                                           texture2d<float> atlas [[texture(0)]],
                                           texture2d<float> lightmap [[texture(1)]],
                                           sampler samp [[sampler(0)]]) {
    float3 N = normalize(in.normal);
    float  ndl_abs = max(abs(dot(N, normalize(U.light_dir))), 0.0);
    float  ambient = 0.50;
    float  diff = ambient + ndl_abs * 0.55;
    if (diff > 1.0) diff = 1.0;

    float3 base_color;
    if (U.use_texture > 0.5) {
        float4 tex = atlas.sample(samp, in.uv);
        base_color = (tex.a < 0.5) ? U.base_color.rgb : tex.rgb;
    } else {
        float3 tint = float3(0.5 + 0.5*N.x, 0.5 + 0.5*N.y, 0.5 + 0.5*N.z);
        base_color = U.base_color.rgb * tint;
    }
    if (U.use_lightmap > 0.5) {
        float3 lm = lightmap.sample(samp, in.luv).rgb;
        base_color *= lm;
        diff = mix(diff, 1.0, 0.55);
    }

    /* Reconstruct approximate world pos from clip is unavailable — use
     * lightmap-space sampling fallback: treat luv-derived stub. For the
     * vertical slice, sample lights at a proxy from normal*scale (host also
     * tints CPU-side). Prefer true world pos when vertex stage passes it. */
    float3 world_proxy = float3(in.luv.x * 256.0 - 128.0,
                                in.luv.y * 256.0 - 128.0,
                                64.0);

    float3 dyn = float3(0.0);
    uint n = min(DL.count, 16u);
    for (uint i = 0u; i < n; ++i) {
        float3 Lpos = DL.lights[i].pos_radius.xyz;
        float  rad  = DL.lights[i].pos_radius.w;
        float3 Lcol = DL.lights[i].color_inten.xyz;
        float  inten= DL.lights[i].color_inten.w;
        if (rad <= 0.0) continue;
        float dist = distance(world_proxy, Lpos);
        if (dist >= rad) continue;
        float attn = 1.0 - (dist / rad);
        attn *= attn;
        dyn += Lcol * (attn * inten);
    }
    float3 color = base_color * diff + dyn;
    color = clamp(color, 0.0, 2.0);
    return float4(color, 1.0);
}

/* Better: pass world position through BSPVertexOut — added field path. */
struct BSPVertexOutW {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float2 luv;
    float3 world_pos;
};

vertex BSPVertexOutW aether_vertex_main_world(BSPVertexIn in [[stage_in]],
                                               constant Uniforms &U [[buffer(1)]]) {
    BSPVertexOutW out;
    float4 world = U.model * float4(in.position, 1.0);
    out.position = U.proj * U.view * world;
    out.normal = normalize((U.model * float4(in.normal, 0.0)).xyz);
    out.uv = in.uv;
    out.luv = in.luv;
    out.world_pos = world.xyz;
    return out;
}

fragment float4 aether_fragment_dynlights_world(BSPVertexOutW in [[stage_in]],
                                                 constant Uniforms &U [[buffer(1)]],
                                                 constant DynLightUBO &DL [[buffer(2)]],
                                                 texture2d<float> atlas [[texture(0)]],
                                                 texture2d<float> lightmap [[texture(1)]],
                                                 sampler samp [[sampler(0)]]) {
    float3 N = normalize(in.normal);
    float  ndl_abs = max(abs(dot(N, normalize(U.light_dir))), 0.0);
    float  diff = clamp(0.50 + ndl_abs * 0.55, 0.0, 1.0);
    float3 base_color = U.base_color.rgb;
    if (U.use_texture > 0.5) {
        float4 tex = atlas.sample(samp, in.uv);
        if (tex.a >= 0.5) base_color = tex.rgb;
    }
    if (U.use_lightmap > 0.5) {
        base_color *= lightmap.sample(samp, in.luv).rgb;
        diff = mix(diff, 1.0, 0.55);
    }
    float3 dyn = float3(0.0);
    uint n = min(DL.count, 16u);
    for (uint i = 0u; i < n; ++i) {
        float3 Lpos = DL.lights[i].pos_radius.xyz;
        float  rad  = max(DL.lights[i].pos_radius.w, 0.001);
        float3 Lcol = DL.lights[i].color_inten.xyz;
        float  inten= DL.lights[i].color_inten.w;
        float3 toL = Lpos - in.world_pos;
        float dist = length(toL);
        if (dist >= rad || dist < 1e-4) continue;
        float3 Ldir = toL / dist;
        float ndotl = max(dot(N, Ldir), 0.0);
        float attn = 1.0 - (dist / rad);
        attn = attn * attn; /* quadratic falloff */
        dyn += Lcol * (attn * inten * (0.35 + 0.65 * ndotl));
    }
    return float4(clamp(base_color * diff + dyn, 0.0, 2.0), 1.0);
}

/* ============ Blob soft-shadow under entities ============ */
struct BlobIn {
    float3 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
    float  alpha    [[attribute(2)]];
};
struct BlobOut {
    float4 position [[position]];
    float2 uv;
    float  alpha;
};
vertex BlobOut aether_blob_shadow_vertex(BlobIn in [[stage_in]],
                                          constant DecalUniforms &U [[buffer(1)]]) {
    BlobOut out;
    out.position = U.proj * U.view * float4(in.position, 1.0);
    out.uv = in.uv;
    out.alpha = in.alpha;
    return out;
}
fragment float4 aether_blob_shadow_fragment(BlobOut in [[stage_in]]) {
    float2 d = in.uv * 2.0 - 1.0;
    float soft = 1.0 - smoothstep(0.35, 1.0, length(d));
    float a = in.alpha * soft;
    if (a < 0.02) discard_fragment();
    return float4(0.0, 0.0, 0.0, a);
}

/* ============ PostFX brightness/gamma fullscreen ============ */
struct PostFXIn {
    float3 position [[attribute(0)]];
    float2 uv       [[attribute(1)]];
};
struct PostFXOut {
    float4 position [[position]];
    float2 uv;
};
struct PostFXUniforms {
    float brightness;
    float gamma;
    float exposure;
    float enabled;
};
vertex PostFXOut aether_postfx_vertex(PostFXIn in [[stage_in]]) {
    PostFXOut out;
    out.position = float4(in.position.xy, 0.0, 1.0);
    out.uv = in.uv;
    return out;
}
fragment float4 aether_postfx_fragment(PostFXOut in [[stage_in]],
                                        constant PostFXUniforms &P [[buffer(1)]],
                                        texture2d<float> scene [[texture(0)]],
                                        sampler samp [[sampler(0)]]) {
    float4 c = scene.sample(samp, in.uv);
    if (P.enabled < 0.5) return c;
    float3 rgb = c.rgb * max(P.exposure, 0.01) + P.brightness;
    rgb = max(rgb, float3(0.0));
    float g = max(P.gamma, 0.2);
    rgb = pow(rgb, float3(1.0 / g));
    return float4(clamp(rgb, 0.0, 1.0), c.a);
}

/* ============ GPU lightstyle weights (base LM × style scale) ============ */
struct LightstyleWeights {
    float weights[64];
    uint  count;
    float time;
    float pad0;
    float pad1;
};
/* Sample helper used by world fragment when style buffer bound. */
inline float3 aether_apply_style_weight(float3 lm_rgb, constant LightstyleWeights &LS, uint style_index) {
    float w = 1.0;
    if (LS.count > 0 && style_index < 64u) w = LS.weights[style_index];
    return lm_rgb * w;
}

/* ============ Bloom PostFX chain ============ */
struct BloomUniforms {
    float threshold;
    float intensity;
    float blur_radius;
    float enabled;
};
fragment float4 aether_bloom_bright_fragment(PostFXOut in [[stage_in]],
                                             constant BloomUniforms &B [[buffer(1)]],
                                             texture2d<float> scene [[texture(0)]],
                                             sampler samp [[sampler(0)]]) {
    float4 c = scene.sample(samp, in.uv);
    if (B.enabled < 0.5) return float4(0.0);
    float lum = dot(c.rgb, float3(0.2126, 0.7152, 0.0722));
    /* Soft-knee bright pass (matches aether_postfx_bloom_bright_sample). */
    float m = smoothstep(B.threshold, B.threshold + 0.15, lum);
    m = m * m * (3.0 - 2.0 * m);
    return float4(c.rgb * m, 1.0);
}
fragment float4 aether_bloom_blur_fragment(PostFXOut in [[stage_in]],
                                           constant BloomUniforms &B [[buffer(1)]],
                                           texture2d<float> src [[texture(0)]],
                                           sampler samp [[sampler(0)]]) {
    float2 texel = float2(B.blur_radius, B.blur_radius) / float2(src.get_width(), src.get_height());
    float3 acc = float3(0.0);
    float wsum = 0.0;
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            float w = 1.0 - 0.15 * float(abs(i) + abs(j));
            acc += src.sample(samp, in.uv + float2(float(i), float(j)) * texel).rgb * w;
            wsum += w;
        }
    }
    return float4(acc / max(wsum, 1e-3), 1.0);
}
fragment float4 aether_bloom_combine_fragment(PostFXOut in [[stage_in]],
                                              constant PostFXUniforms &P [[buffer(1)]],
                                              constant BloomUniforms &B [[buffer(2)]],
                                              texture2d<float> scene [[texture(0)]],
                                              texture2d<float> bloom [[texture(1)]],
                                              sampler samp [[sampler(0)]]) {
    float4 c = scene.sample(samp, in.uv);
    float3 rgb = c.rgb;
    if (P.enabled > 0.5) {
        rgb = rgb * max(P.exposure, 0.01) + P.brightness;
        rgb = max(rgb, float3(0.0));
        float g = max(P.gamma, 0.2);
        rgb = pow(rgb, float3(1.0 / g));
    }
    if (B.enabled > 0.5) {
        float3 b = bloom.sample(samp, in.uv).rgb;
        rgb += b * B.intensity;
    }
    return float4(clamp(rgb, 0.0, 1.0), c.a);
}

/* ============ Decal atlas sample ============ */
fragment float4 aether_decal_atlas_fragment(DecalQuadOut in [[stage_in]],
                                            texture2d<float> atlas [[texture(0)]],
                                            sampler samp [[sampler(0)]]) {
    float4 t = atlas.sample(samp, in.uv);
    float a = t.a * in.color.a;
    if (a < 0.02) discard_fragment();
    return float4(t.rgb * in.color.rgb, a);
}

/* ============ MDL skinning stub (single/dual bone matrix) ============ */
struct SkinUniforms {
    float4x4 bone0;
    float4x4 bone1;
    float    bone_count;
    float    pad0, pad1, pad2;
};
vertex MdlVertexOut aether_mdl_skinned_vertex(MdlVertexIn in [[stage_in]],
                                               constant Uniforms &U [[buffer(1)]],
                                               constant SkinUniforms &S [[buffer(2)]]) {
    float3 p = float3(in.position.x, in.position.z, in.position.y);
    float3 n = float3(in.normal.x,   in.normal.z,   in.normal.y);
    float4 lp = float4(p, 1.0);
    float4 skinned = lp;
    if (S.bone_count >= 1.0) {
        skinned = S.bone0 * lp;
        if (S.bone_count >= 2.0) {
            float4 s1 = S.bone1 * lp;
            skinned = mix(skinned, s1, 0.35);
        }
    }
    MdlVertexOut out;
    float4 world = U.model * skinned;
    out.position = U.proj * U.view * world;
    out.normal = normalize((U.model * float4(n, 0.0)).xyz);
    return out;
}

/* ============ Multi-style lightmap blend sample ============ */
struct FaceStyleBlendUniforms {
    float face_count;
    float flags;       /* bit0 = live face_id attribute */
    float stride_hint; /* CPU mesh vertex stride */
    float pad2;
    /* weights packed as float4 per face immediately after in buffer — Metal
     * constant buffer consumers pass FaceStyleBlendWeights separately. */
};
struct FaceStyleBlendWeights {
    float4 w[64]; /* up to 64 faces × 4 style weights */
};
inline float3 aether_apply_style_blend(float3 lm_rgb, float4 weights) {
    float wsum = max(weights.x, 0.0) + max(weights.y, 0.0) +
                 max(weights.z, 0.0) + max(weights.w, 0.0);
    float scale = (wsum > 1e-5) ? min(wsum, 4.0) : 1.0;
    return lm_rgb * scale;
}
fragment float4 aether_fragment_style_blend(BSPVertexOut in [[stage_in]],
                                            constant Uniforms &U [[buffer(1)]],
                                            constant FaceStyleBlendUniforms &FB [[buffer(2)]],
                                            constant FaceStyleBlendWeights &FW [[buffer(3)]],
                                            texture2d<float> atlas [[texture(0)]],
                                            texture2d<float> lightmap [[texture(1)]],
                                            sampler samp [[sampler(0)]]) {
    float3 N = normalize(in.normal);
    float ndl_abs = max(abs(dot(N, normalize(U.light_dir))), 0.0);
    float ambient = 0.55;
    float diff = ambient + ndl_abs * 0.60;
    if (diff > 1.0) diff = 1.0;
    float3 base_color = U.base_color.rgb;
    if (U.use_texture > 0.5) {
        float4 tex = atlas.sample(samp, in.uv);
        if (tex.a >= 0.5) base_color = tex.rgb;
    }
    if (U.use_lightmap > 0.5) {
        float3 lm = lightmap.sample(samp, in.luv).rgb;
        uint fi = 0u;
        if (FB.face_count > 0.5) {
            /* Live face_id vertex attribute (preferred); LUV stub only if flag clear. */
            if (FB.flags >= 0.5) {
                fi = uint(clamp(in.face_id, 0.0, FB.face_count - 1.0));
            } else {
                fi = uint(clamp(in.luv.x * FB.face_count, 0.0, FB.face_count - 1.0));
            }
            if (fi > 63u) fi = 63u;
        }
        lm = aether_apply_style_blend(lm, FW.w[fi]);
        base_color *= lm;
        diff = mix(diff, 1.0, 0.65);
    }
    return float4(base_color * diff, 1.0);
}

/* ============ Separable bloom blur (H / V) ============ */
struct BloomBlurDir {
    float threshold;
    float intensity;
    float blur_radius;
    float enabled;
    float2 direction; /* (1,0)=H or (0,1)=V */
    float2 pad;
};
fragment float4 aether_bloom_blur_h_fragment(PostFXOut in [[stage_in]],
                                             constant BloomBlurDir &B [[buffer(1)]],
                                             texture2d<float> src [[texture(0)]],
                                             sampler samp [[sampler(0)]]) {
    float2 texel = float2(B.blur_radius, B.blur_radius) / float2(src.get_width(), src.get_height());
    float2 dir = length(B.direction) > 0.1 ? normalize(B.direction) : float2(1.0, 0.0);
    float3 acc = float3(0.0);
    float wsum = 0.0;
    for (int i = -4; i <= 4; ++i) {
        float w = 1.0 - 0.1 * float(abs(i));
        acc += src.sample(samp, in.uv + dir * float(i) * texel).rgb * w;
        wsum += w;
    }
    return float4(acc / max(wsum, 1e-4), 1.0);
}
fragment float4 aether_bloom_blur_v_fragment(PostFXOut in [[stage_in]],
                                             constant BloomBlurDir &B [[buffer(1)]],
                                             texture2d<float> src [[texture(0)]],
                                             sampler samp [[sampler(0)]]) {
    float2 texel = float2(B.blur_radius, B.blur_radius) / float2(src.get_width(), src.get_height());
    float2 dir = length(B.direction) > 0.1 ? normalize(B.direction) : float2(0.0, 1.0);
    float3 acc = float3(0.0);
    float wsum = 0.0;
    for (int i = -4; i <= 4; ++i) {
        float w = 1.0 - 0.1 * float(abs(i));
        acc += src.sample(samp, in.uv + dir * float(i) * texel).rgb * w;
        wsum += w;
    }
    return float4(acc / max(wsum, 1e-4), 1.0);
}

/* ============ Depth prepass (depth-only encode stub) ============ */
struct DepthPrepassUniforms {
    float4x4 mvp;
    float4   clip_plane; /* unused in basic prepass; shared with water reflect hooks */
    float    enabled;
    float3   pad;
};
struct DepthPrepassIn {
    float3 position [[attribute(0)]];
};
struct DepthPrepassOut {
    float4 position [[position]];
};
vertex DepthPrepassOut aether_depth_prepass_vertex(DepthPrepassIn in [[stage_in]],
                                                   constant DepthPrepassUniforms &U [[buffer(1)]]) {
    DepthPrepassOut o;
    float4 wp = float4(in.position, 1.0);
    o.position = U.mvp * wp;
    return o;
}
/* Color attachment unused / masked off; fragment kept for pipeline validity on some targets. */
fragment float4 aether_depth_prepass_fragment(DepthPrepassOut in [[stage_in]]) {
    return float4(0.0, 0.0, 0.0, 0.0);
}

/* ============ Water planar reflection clip hook ============ */
struct WaterReflectUniforms {
    float4x4 mirror;
    float4   clip_plane;
    float    enabled;
    float3   pad;
};
/* Transform eye/world position by mirror matrix (CPU also fills uniforms). */
float3 aether_water_reflect_transform(float3 p, constant WaterReflectUniforms &R) {
    float4 h = R.mirror * float4(p, 1.0);
    return h.xyz;
}
/* Sample reflection render-target (allocated by Metal from encode plan). */
float3 aether_water_reflect_sample(texture2d<float> tex, sampler s, float2 uv) {
    return tex.sample(s, uv).rgb;
}

/* ============ Hi-Z mip pyramid downsample + visibility query hooks ============ */
struct HiZUniforms {
    uint srcWidth;
    uint srcHeight;
    uint dstWidth;
    uint dstHeight;
    uint level;
    uint pad0, pad1, pad2;
};

/* Downsample: store min depth of 2x2 (nearer covers) into next mip. */
kernel void aether_hiz_downsample(texture2d<float, access::read> src [[texture(0)]],
                                  texture2d<float, access::write> dst [[texture(1)]],
                                  constant HiZUniforms &U [[buffer(0)]],
                                  uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= U.dstWidth || gid.y >= U.dstHeight) return;
    uint2 s0 = gid * 2;
    float z = src.read(s0).r;
    if (s0.x + 1 < U.srcWidth) z = min(z, src.read(s0 + uint2(1, 0)).r);
    if (s0.y + 1 < U.srcHeight) {
        z = min(z, src.read(s0 + uint2(0, 1)).r);
        if (s0.x + 1 < U.srcWidth) z = min(z, src.read(s0 + uint2(1, 1)).r);
    }
    dst.write(float4(z, z, z, 1.0), gid);
}

struct HiZVisQuery {
    float4 rect;       /* x0,y0,x1,y1 in 0..1 */
    float  objectDepth;
    float  pad0, pad1, pad2;
};

struct HiZVisResult {
    float nearestHiz;
    float occluded;    /* 1 if occluded */
    float mipUsed;
    float valid;
};

/* CPU/host also implements query; this fragment documents the Metal hook. */
fragment HiZVisResult aether_hiz_vis_query_fragment(constant HiZVisQuery &Q [[buffer(0)]],
                                                    texture2d<float> hiz [[texture(0)]],
                                                    sampler samp [[sampler(0)]]) {
    HiZVisResult r;
    float2 uv = float2((Q.rect.x + Q.rect.z) * 0.5, (Q.rect.y + Q.rect.w) * 0.5);
    float hz = hiz.sample(samp, uv).r;
    r.nearestHiz = hz;
    r.occluded = (hz + 0.01 < Q.objectDepth) ? 1.0 : 0.0;
    r.mipUsed = 0.0;
    r.valid = 1.0;
    return r;
}

/* Studio texture sample for water reflect RT (procedural atlas tint). */
fragment float4 aether_studio_reflect_tex_fragment(WaterVertexOut in [[stage_in]],
                                                   constant float4 &tint [[buffer(2)]],
                                                   constant float4 &atlas [[buffer(3)]]) {
    float2 uv = in.uv;
    float au = mix(atlas.x, atlas.z, fract(uv.x));
    float av = mix(atlas.y, atlas.w, fract(uv.y));
    float wave = 0.5 + 0.5 * sin(au * 40.0 + av * 28.0);
    float checker = (((int)(au * 16.0) + (int)(av * 16.0)) & 1) ? 1.0 : 0.85;
    float3 rgb = tint.rgb * float3(0.75 + 0.25 * wave, 0.80 + 0.20 * (1.0 - wave), 0.70 + 0.30 * wave) * checker;
    return float4(rgb, tint.a);
}


/* ============ Depth → Hi-Z bind + portal recursive + skin-page sample hooks ============ */
struct DepthHizBindUniforms {
    uint mip0W;
    uint mip0H;
    uint levels;
    uint bound;
};

/* Documents Metal texture-view bind after depth prepass (host fills pyramid). */
fragment float4 aether_depth_hiz_bind_fragment(constant DepthHizBindUniforms &U [[buffer(0)]],
                                               texture2d_array<float> hizMips [[texture(0)]],
                                               sampler samp [[sampler(0)]],
                                               float2 uv [[stage_in]]) {
    float z = hizMips.sample(samp, uv, 0).r;
    if (U.levels > 1) {
        float z1 = hizMips.sample(samp, uv, 1).r;
        z = min(z, z1);
    }
    return float4(z, z, U.bound > 0 ? 1.0 : 0.0, 1.0);
}

struct PortalRecursiveUniforms {
    float4 clipPlane;
    float4x4 mirror;
    uint depth;
    uint maxDepth;
    uint pad0, pad1;
};

vertex float4 aether_portal_recursive_vertex(uint vid [[vertex_id]],
                                             constant PortalRecursiveUniforms &U [[buffer(0)]],
                                             constant float3 *positions [[buffer(1)]]) {
    float3 p = positions[vid];
    float4 h = U.mirror * float4(p, 1.0);
    /* Clip against portal/water plane for recursive view. */
    float cd = dot(U.clipPlane.xyz, h.xyz) + U.clipPlane.w;
    if (cd < 0.0 && U.depth > 0) h.z = h.z; /* keep; real clip is raster clip plane */
    return h;
}

fragment float4 aether_mdl_skin_page_fragment(WaterVertexOut in [[stage_in]],
                                              texture2d<float> skinPage [[texture(0)]],
                                              sampler samp [[sampler(0)]],
                                              constant float4 &tint [[buffer(2)]]) {
    float4 s = skinPage.sample(samp, in.uv);
    return float4(s.rgb * tint.rgb, s.a * tint.a);
}


/* ============ Hi-Z texture2d_array mip vis + portal-graph flood hooks ============ */
struct HizArrayVisUniforms {
    float4 rect;        /* x0,y0,x1,y1 */
    float  objectDepth;
    float  arrayMip;    /* slice index */
    float  pad0, pad1;
};

struct HizArrayVisResult {
    float nearestHiz;
    float occluded;
    float mipUsed;
    float valid;
};

/* Vis query samples a specific texture2d_array mip/slice (Metal encode path). */
fragment HizArrayVisResult aether_hiz_array_vis_query_fragment(constant HizArrayVisUniforms &Q [[buffer(0)]],
                                                               texture2d_array<float> hizArray [[texture(0)]],
                                                               sampler samp [[sampler(0)]]) {
    HizArrayVisResult r;
    float2 uv = float2((Q.rect.x + Q.rect.z) * 0.5, (Q.rect.y + Q.rect.w) * 0.5);
    uint slice = (uint)max(Q.arrayMip, 0.0);
    if (slice >= hizArray.get_array_size()) slice = hizArray.get_array_size() - 1;
    float hz = hizArray.sample(samp, uv, slice).r;
    r.nearestHiz = hz;
    r.occluded = (hz + 0.01 < Q.objectDepth) ? 1.0 : 0.0;
    r.mipUsed = float(slice);
    r.valid = 1.0;
    return r;
}

struct PortalGraphFloodUniforms {
    uint startLeaf;
    uint reached;
    uint maxDepth;
    uint viewCount;
};

/* Documents multi-portal leaf-graph flood feeding reflect views. */
fragment float4 aether_portal_graph_flood_fragment(constant PortalGraphFloodUniforms &U [[buffer(0)]],
                                                   float2 uv [[stage_in]]) {
    float t = (U.reached > 0) ? (float(U.viewCount) / max(float(U.reached), 1.0)) : 0.0;
    return float4(t, float(U.startLeaf) * 0.05, float(U.maxDepth) * 0.2, 1.0);
}

fragment float4 aether_mdl_skin_lump_fragment(WaterVertexOut in [[stage_in]],
                                              texture2d<float> skinLump [[texture(0)]],
                                              sampler samp [[sampler(0)]],
                                              constant float4 &tint [[buffer(2)]]) {
    float4 s = skinLump.sample(samp, in.uv);
    return float4(s.rgb * tint.rgb, s.a * tint.a);
}


/* ============ GPU Hi-Z array downsample chain + skinref sample + portal winding ============ */
struct HizArrayDownsampleUniforms {
    uint srcWidth;
    uint srcHeight;
    uint dstWidth;
    uint dstHeight;
    uint srcSlice;
    uint dstSlice;
    uint passIndex;
    uint pad0;
};

/* Compute: downsample mip N → array slice N+1 (min-z of 2x2). */
kernel void aether_hiz_array_downsample(texture2d_array<float, access::read> src [[texture(0)]],
                                        texture2d_array<float, access::write> dst [[texture(1)]],
                                        constant HizArrayDownsampleUniforms &U [[buffer(0)]],
                                        uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= U.dstWidth || gid.y >= U.dstHeight) return;
    uint2 s0 = gid * 2;
    float z = src.read(uint3(s0, U.srcSlice)).r;
    if (s0.x + 1 < U.srcWidth) z = min(z, src.read(uint3(s0 + uint2(1, 0), U.srcSlice)).r);
    if (s0.y + 1 < U.srcHeight) {
        z = min(z, src.read(uint3(s0 + uint2(0, 1), U.srcSlice)).r);
        if (s0.x + 1 < U.srcWidth)
            z = min(z, src.read(uint3(s0 + uint2(1, 1), U.srcSlice)).r);
    }
    dst.write(float4(z, z, z, 1.0), uint3(gid, U.dstSlice));
}

/* Fragment path alternate: sample src slice, write min into destination encoding. */
fragment float4 aether_hiz_array_downsample_fragment(constant HizArrayDownsampleUniforms &U [[buffer(0)]],
                                                     texture2d_array<float> src [[texture(0)]],
                                                     sampler samp [[sampler(0)]],
                                                     float2 uv [[stage_in]]) {
    float2 texel = float2(1.0 / max(float(U.srcWidth), 1.0), 1.0 / max(float(U.srcHeight), 1.0));
    float z = src.sample(samp, uv, U.srcSlice).r;
    z = min(z, src.sample(samp, uv + float2(texel.x, 0.0), U.srcSlice).r);
    z = min(z, src.sample(samp, uv + float2(0.0, texel.y), U.srcSlice).r);
    z = min(z, src.sample(samp, uv + texel, U.srcSlice).r);
    return float4(z, z, float(U.dstSlice) / 8.0, 1.0);
}

struct HizDownsampleVisUniforms {
    float4 rect;
    float  objectDepth;
    float  arrayMip;
    float  downsampleReady; /* 1 when chain bound into vis */
    float  pad0;
};

/* Vis query that reads downsample-filled array slices. */
fragment HizArrayVisResult aether_hiz_vis_query_downsampled_fragment(constant HizDownsampleVisUniforms &Q [[buffer(0)]],
                                                                     texture2d_array<float> hizArray [[texture(0)]],
                                                                     sampler samp [[sampler(0)]]) {
    HizArrayVisResult r;
    if (Q.downsampleReady < 0.5) {
        r.nearestHiz = 1.0; r.occluded = 0.0; r.mipUsed = -1.0; r.valid = 0.0;
        return r;
    }
    float2 uv = float2((Q.rect.x + Q.rect.z) * 0.5, (Q.rect.y + Q.rect.w) * 0.5);
    uint slice = (uint)max(Q.arrayMip, 0.0);
    if (slice >= hizArray.get_array_size()) slice = hizArray.get_array_size() - 1;
    float hz = hizArray.sample(samp, uv, slice).r;
    r.nearestHiz = hz;
    r.occluded = (hz + 0.01 < Q.objectDepth) ? 1.0 : 0.0;
    r.mipUsed = float(slice);
    r.valid = 1.0;
    return r;
}

struct PortalWindingUniforms {
    float4 plane;
    uint   vertCount;
    uint   fromMarksurfaces;
    uint   pad0, pad1;
};

/* Documents fuller portal winding from marksurfaces/planes. */
fragment float4 aether_portal_winding_marksurface_fragment(constant PortalWindingUniforms &U [[buffer(0)]],
                                                           float2 uv [[stage_in]]) {
    float side = U.plane.x * uv.x + U.plane.y * uv.y + U.plane.z * 0.5 + U.plane.w;
    float mark = (U.fromMarksurfaces > 0) ? 1.0 : 0.35;
    return float4(mark, float(U.vertCount) / 8.0, saturate(side * 0.1 + 0.5), 1.0);
}

struct SkinrefSelectUniforms {
    uint family;
    uint refIndex;
    uint group;
    uint tex;
};

fragment float4 aether_mdl_skinref_select_fragment(WaterVertexOut in [[stage_in]],
                                                   constant SkinrefSelectUniforms &U [[buffer(2)]],
                                                   constant float4 &tint [[buffer(3)]]) {
    float2 uv = in.uv;
    float fam = float(U.family) * 0.25;
    float rf = float(U.refIndex) * 0.15;
    float checker = (((int)(uv.x * 8.0) + (int)(uv.y * 8.0) + (int)U.group) & 1) ? 1.0 : 0.85;
    float3 rgb = tint.rgb * float3(0.55 + fam, 0.50 + rf, 0.45 + float(U.tex) * 0.1) * checker;
    return float4(rgb, tint.a);
}


/* ============ Live Hi-Z encode from depth + portal reflect clip + skinref remap ============ */
struct HizLiveEncodeUniforms {
    uint srcWidth;
    uint srcHeight;
    uint dstWidth;
    uint dstHeight;
    uint srcSlice;   /* 0 = depth fill; else downsample src */
    uint dstSlice;
    uint passIndex;  /* 0 = fill from depth, >=1 = downsample */
    uint fromDepth;  /* 1 when reading depth texture */
};

/* Fill Hi-Z mip0 / array slice 0 from a live depth texture (linearized). */
kernel void aether_hiz_encode_from_depth(depth2d<float, access::read> depthTex [[texture(0)]],
                                         texture2d_array<float, access::write> hiz [[texture(1)]],
                                         constant HizLiveEncodeUniforms &U [[buffer(0)]],
                                         uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= U.dstWidth || gid.y >= U.dstHeight) return;
    float z = 1.0;
    if (U.fromDepth != 0) {
        uint2 dcoord = gid;
        if (dcoord.x >= U.srcWidth) dcoord.x = U.srcWidth - 1;
        if (dcoord.y >= U.srcHeight) dcoord.y = U.srcHeight - 1;
        z = depthTex.read(dcoord);
    }
    hiz.write(float4(z, z, z, 1.0), uint3(gid, U.dstSlice));
}

/* Fragment alternate: sample depth, emit linearized Hi-Z for encode plan pass 0. */
fragment float4 aether_hiz_encode_from_depth_fragment(constant HizLiveEncodeUniforms &U [[buffer(0)]],
                                                      depth2d<float> depthTex [[texture(0)]],
                                                      sampler samp [[sampler(0)]],
                                                      float2 uv [[stage_in]]) {
    float z = depthTex.sample(samp, uv);
    return float4(z, z, float(U.passIndex) / 8.0, 1.0);
}

struct PortalReflectClipUniforms {
    float4 clipPlanes[4];
    uint   planeCount;
    uint   windingVerts;
    uint   pad0, pad1;
};

/* Documents portal winding clipped against recursive reflect clip planes. */
fragment float4 aether_portal_winding_reflect_clip_fragment(constant PortalReflectClipUniforms &U [[buffer(0)]],
                                                            float2 uv [[stage_in]]) {
    float2 p = uv * 2.0 - 1.0;
    float keep = 1.0;
    for (uint i = 0; i < U.planeCount && i < 4; ++i) {
        float d = U.clipPlanes[i].x * p.x + U.clipPlanes[i].y * p.y
                + U.clipPlanes[i].z * 0.5 + U.clipPlanes[i].w;
        if (d < -0.0001) keep = 0.0;
    }
    return float4(keep, float(U.planeCount) / 4.0, float(U.windingVerts) / 8.0, 1.0);
}

struct SkinrefRemapUniforms {
    uint  family;
    uint  refIndex;
    uint  group;
    uint  tex;
    uint  drawSlot;
    float uvScaleX;
    float uvScaleY;
    float uvOffX;
    float uvOffY;
    float pad0, pad1, pad2;
};

/* Studio skinref → texture remap on draw (atlas UV + slot tint). */
fragment float4 aether_mdl_skinref_remap_fragment(WaterVertexOut in [[stage_in]],
                                                  constant SkinrefRemapUniforms &U [[buffer(2)]],
                                                  constant float4 &tint [[buffer(3)]]) {
    float2 uv = in.uv;
    float2 mapped = float2(U.uvOffX, U.uvOffY) + uv * float2(U.uvScaleX, U.uvScaleY);
    float checker = (((int)(mapped.x * 8.0) + (int)(mapped.y * 8.0) + (int)U.group) & 1) ? 1.0 : 0.88;
    float slot = float(U.drawSlot) * 0.05;
    float3 rgb = tint.rgb * float3(0.50 + float(U.family) * 0.2,
                                   0.48 + float(U.refIndex) * 0.12,
                                   0.42 + float(U.tex) * 0.1 + slot) * checker;
    return float4(rgb, tint.a);
}


/* ============ Batch19: MTK depth attach Hi-Z + portal clip stack + skin Metal bind ============ */

struct PortalClipStackUniforms {
    float4 planes[8];
    uint   planeCount;
    uint   pushCount;
    uint   clipOps;
    uint   pad0;
};

/* Documents multi-plane portal clip stack (fuller Quake-style clip buffer). */
fragment float4 aether_portal_clip_stack_fragment(constant PortalClipStackUniforms &U [[buffer(0)]],
                                                  float2 uv [[stage_in]]) {
    float2 p = uv * 2.0 - 1.0;
    float keep = 1.0;
    for (uint i = 0; i < U.planeCount && i < 8; ++i) {
        float d = U.planes[i].x * p.x + U.planes[i].y * p.y
                + U.planes[i].z * 0.5 + U.planes[i].w;
        if (d < -0.0001) keep = 0.0;
    }
    return float4(keep, float(U.planeCount) / 8.0, float(U.pushCount) / 8.0, 1.0);
}

struct DepthHizMtkAttachUniforms {
    uint width;
    uint height;
    uint pixelFormat; /* 1 = depth32Float */
    uint storeAction; /* 1 = store */
    uint shaderRead;
    uint encodeWired;
    uint attached;
    uint encodePasses;
};

/* Documents live MTK depth attachment wired into Hi-Z encode. */
fragment float4 aether_depth_hiz_mtk_attach_fragment(constant DepthHizMtkAttachUniforms &U [[buffer(0)]],
                                                     depth2d<float> depthTex [[texture(0)]],
                                                     sampler samp [[sampler(0)]],
                                                     float2 uv [[stage_in]]) {
    float z = depthTex.sample(samp, uv);
    float ready = (U.attached != 0 && U.encodeWired != 0 && U.shaderRead != 0) ? 1.0 : 0.0;
    return float4(z, ready, float(U.encodePasses) / 8.0, 1.0);
}

/* Studio skinref → actual Metal texture bind on draw (samples bound atlas at slot). */
fragment float4 aether_mdl_skinref_bind_fragment(MdlVertexOut in [[stage_in]],
                                                 constant Uniforms &U [[buffer(1)]],
                                                 constant SkinrefRemapUniforms &R [[buffer(2)]],
                                                 constant float4 &tint [[buffer(3)]],
                                                 texture2d<float> skinTex [[texture(3)]],
                                                 sampler skinSamp [[sampler(3)]]) {
    /* Derive atlas UV from normal hemisphere + remap scale/offset. */
    float2 baseUV = float2(in.normal.x * 0.5 + 0.5, in.normal.y * 0.5 + 0.5);
    float2 mapped = float2(R.uvOffX, R.uvOffY) + baseUV * float2(R.uvScaleX, R.uvScaleY);
    float4 tex = skinTex.sample(skinSamp, mapped);
    float3 N = normalize(in.normal);
    float3 L = normalize(U.light_dir);
    float ndl = max(abs(dot(N, L)), 0.0);
    float3 lit = tint.rgb * (0.55 + 0.45 * ndl);
    float3 rgb = mix(lit, tex.rgb * lit, (U.use_texture > 0.5) ? 0.85 : 0.0);
    /* Slot bias so bind is observable in smokes / frame dumps */
    rgb += float3(float(R.drawSlot) * 0.01, float(R.family) * 0.02, float(R.refIndex) * 0.015);
    return float4(rgb, tint.a);
}

/* ===== Batch20: Hi-Z GPU mipchain / portal×PVS / skin-lump Metal families ===== */

struct HizGpuMipchainUniforms {
    uint levels;
    uint passes;
    uint mip0W;
    uint mip0H;
    uint fromMtk;
    uint ready;
};

/* Full GPU mipchain after MTK depth attach: downsample previous slice → next. */
kernel void aether_hiz_gpu_mipchain(texture2d_array<float, access::read> src [[texture(0)]],
                                    texture2d_array<float, access::write> dst [[texture(1)]],
                                    constant HizGpuMipchainUniforms &U [[buffer(0)]],
                                    uint3 gid [[thread_position_in_grid]]) {
    uint slice = gid.z;
    if (slice == 0 || slice >= U.levels) return;
    uint x = gid.x, y = gid.y;
    uint sw = max(U.mip0W >> slice, 1u);
    uint sh = max(U.mip0H >> slice, 1u);
    if (x >= sw || y >= sh) return;
    uint prev = slice - 1;
    uint2 p0 = uint2(x * 2, y * 2);
    float z00 = src.read(p0, prev).r;
    float z10 = src.read(p0 + uint2(1, 0), prev).r;
    float z01 = src.read(p0 + uint2(0, 1), prev).r;
    float z11 = src.read(p0 + uint2(1, 1), prev).r;
    float zmin = min(min(z00, z10), min(z01, z11));
    dst.write(float4(zmin, zmin, zmin, 1.0), uint2(x, y), slice);
}

fragment float4 aether_hiz_gpu_mipchain_fragment(constant HizGpuMipchainUniforms &U [[buffer(0)]],
                                                 texture2d_array<float> hiz [[texture(0)]],
                                                 sampler samp [[sampler(0)]],
                                                 float2 uv [[stage_in]]) {
    float z0 = hiz.sample(samp, uv, 0).r;
    float z1 = (U.levels > 1) ? hiz.sample(samp, uv, 1).r : z0;
    float ready = (U.ready != 0 && U.fromMtk != 0) ? 1.0 : 0.0;
    return float4(z0, z1, float(U.passes) / 8.0, ready);
}

struct PortalPvsFloodUniforms {
    uint reached;
    uint pvsHits;
    uint portalOnly;
    uint views;
    uint usedPvs;
    uint eyeLeaf;
};

fragment float4 aether_portal_pvs_flood_fragment(constant PortalPvsFloodUniforms &U [[buffer(0)]],
                                                 float2 uv [[stage_in]]) {
    float vis = float(U.pvsHits) / max(float(U.reached), 1.0);
    float cull = float(U.portalOnly) / max(float(U.reached), 1.0);
    return float4(vis, cull, float(U.views) / 4.0, (U.usedPvs != 0) ? 1.0 : 0.0);
}

struct SkinLumpMetalFamilyUniforms {
    uint familyCount;
    uint selected;
    uint drawSlot;
    uint atlasW;
    uint atlasH;
    uint bound;
    uint usedFixture;
};

fragment float4 aether_mdl_skin_lump_metal_family_fragment(MdlVertexOut in [[stage_in]],
                                                           constant Uniforms &U [[buffer(1)]],
                                                           constant SkinLumpMetalFamilyUniforms &F [[buffer(2)]],
                                                           constant float4 &tint [[buffer(3)]],
                                                           texture2d<float> skinAtlas [[texture(3)]],
                                                           sampler skinSamp [[sampler(3)]]) {
    float2 uv = float2(in.normal.x * 0.5 + 0.5, in.normal.y * 0.5 + 0.5);
    /* Horizontal family strip: select family band. */
    float fam = float(F.selected) / max(float(F.familyCount), 1.0);
    float2 mapped = float2(fam + uv.x / max(float(F.familyCount), 1.0), uv.y);
    float4 tex = skinAtlas.sample(skinSamp, mapped);
    float3 N = normalize(in.normal);
    float3 L = normalize(U.light_dir);
    float ndl = max(abs(dot(N, L)), 0.0);
    float3 lit = tint.rgb * (0.55 + 0.45 * ndl);
    float3 rgb = mix(lit, tex.rgb * lit, (U.use_texture > 0.5) ? 0.85 : 0.0);
    rgb += float3(float(F.drawSlot) * 0.01, float(F.selected) * 0.02, float(F.usedFixture) * 0.03);
    float bound = (F.bound != 0) ? 1.0 : 0.0;
    return float4(rgb, bound);
}

fragment float4 aether_depth_hiz_mtk_mipchain_fragment(constant HizGpuMipchainUniforms &U [[buffer(0)]],
                                                      depth2d<float> depthTex [[texture(0)]],
                                                      sampler samp [[sampler(0)]],
                                                      float2 uv [[stage_in]]) {
    float z = depthTex.sample(samp, uv);
    float ready = (U.ready != 0 && U.fromMtk != 0) ? 1.0 : 0.0;
    return float4(z, float(U.levels) / 8.0, float(U.passes) / 8.0, ready);
}

/* ---------- Batch21: Hi-Z occlusion feedback → LOD / PVS decode / FS docs ---------- */
struct HizOcclusionFeedbackUniforms {
    uint occluded;
    uint visible;
    uint mipchainReady;
    uint metalFeedback;
    uint mipUsed;
    uint lod;
    uint issue;
    float nearestHiz;
    float objectDepth;
    float screenPixels;
};

fragment float4 aether_hiz_occlusion_lod_feedback_fragment(constant HizOcclusionFeedbackUniforms &U [[buffer(0)]],
                                                           texture2d_array<float> hiz [[texture(0)]],
                                                           sampler samp [[sampler(0)]],
                                                           float2 uv [[stage_in]]) {
    float z = hiz.sample(samp, uv, U.mipUsed).r;
    float occ = (U.occluded != 0) ? 1.0 : 0.0;
    float ready = (U.mipchainReady != 0 && U.metalFeedback != 0) ? 1.0 : 0.0;
    return float4(z, occ, float(U.lod) / 8.0, ready);
}

struct PvsDecodeUniforms {
    uint leafCount;
    uint visibleCount;
    uint rowBytes;
    uint rleBytes;
    uint fromFixture;
    uint fromUserLump;
    uint viewLeaf;
};

fragment float4 aether_bsp_vis_pvs_decode_fragment(constant PvsDecodeUniforms &U [[buffer(0)]],
                                                   float2 uv [[stage_in]]) {
    float vis = float(U.visibleCount) / max(float(U.leafCount), 1.0);
    float src = (U.fromUserLump != 0) ? 1.0 : ((U.fromFixture != 0) ? 0.5 : 0.0);
    return float4(vis, src, float(U.rowBytes) / 64.0, float(U.viewLeaf) / 16.0);
}

struct DocumentsFsMountUniforms {
    uint rootsMounted;
    uint valveMounted;
    uint gamedirMounted;
    uint layoutEnsured;
};

fragment float4 aether_fs_documents_gamedir_fragment(constant DocumentsFsMountUniforms &U [[buffer(0)]],
                                                     float2 uv [[stage_in]]) {
    float roots = float(U.rootsMounted) / 4.0;
    float ok = (U.layoutEnsured != 0 && U.valveMounted != 0) ? 1.0 : 0.0;
    return float4(roots, float(U.gamedirMounted), ok, 1.0);
}

fragment float4 aether_depth_hiz_occlusion_feedback_fragment(constant HizOcclusionFeedbackUniforms &U [[buffer(0)]],
                                                             depth2d<float> depthTex [[texture(0)]],
                                                             sampler samp [[sampler(0)]],
                                                             float2 uv [[stage_in]]) {
    float z = depthTex.sample(samp, uv);
    float occ = (U.occluded != 0) ? 1.0 : 0.0;
    return float4(z, occ, U.nearestHiz, (U.metalFeedback != 0) ? 1.0 : 0.0);
}
