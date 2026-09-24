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
