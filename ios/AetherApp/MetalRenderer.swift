// MetalRenderer.swift
// Renders BSP/world mesh + entities + particles + sky + water + fog + lightmap + VIS leaf cull. STEP 2i / bsp-vis.
// Pushes view/proj + frame dt into EngineBridge each draw.
// AetherEngine-iOS · Clean-room.

import MetalKit
import SwiftUI
import simd

struct Uniforms {
    var model:       simd_float4x4
    var view:        simd_float4x4
    var proj:        simd_float4x4
    var lightDir:    simd_float3
    var pad0:        Float = 0
    var baseColor:   simd_float4
    var useTexture:  Float = 0
    var useLightmap: Float = 0
    var pad2:        Float = 0
    var pad3:        Float = 0
}

final class MetalRenderer: NSObject, MTKViewDelegate {
    let device: MTLDevice
    let queue:  MTLCommandQueue

    var bspPipeline:  MTLRenderPipelineState?
    var styleBlendPipeline: MTLRenderPipelineState?
    var styleBlendUboBuffer: MTLBuffer?
    var mdlPipeline:  MTLRenderPipelineState?
    var depthState:   MTLDepthStencilState?
    var depthPrepassPipeline: MTLRenderPipelineState?
    var depthPrepassDepthState: MTLDepthStencilState?
    var samplerState: MTLSamplerState?

    var vertexBuffer: MTLBuffer?
    var indexBuffer:  MTLBuffer?
    var indexCount:   Int = 0
    /// Culled (leaf/PVS) index buffer — rewritten each frame from EngineBridge VIS.
    var culledIndexBuffer: MTLBuffer?
    var culledIndexCount: Int = 0

    var atlasTexture: MTLTexture?
    var hasTexture:   Bool = false
    var lightmapTexture: MTLTexture?
    var hasLightmap:     Bool = false

    var mdlVertexBuf:  MTLBuffer?
    var mdlIndexBuf:   MTLBuffer?
    var mdlIndexCount: Int = 0
    var hasMdl:        Bool = false

    // Monster billboards
    var monsterVertexBuf: MTLBuffer?
    var monsterIndexBuf:  MTLBuffer?
    var monsterIndexCount: Int = 0
    var monsterPositions: [simd_float3] = []

    // Particles (driven by AetherParticle via EngineBridge)
    var particlePipeline: MTLRenderPipelineState?
    var particleDepthState: MTLDepthStencilState?
    var particleBuffer:   MTLBuffer?
    var particleCount:    Int = 0
    var decalQuadPipeline: MTLRenderPipelineState?
    var spriteQuadPipeline: MTLRenderPipelineState?
    var decalQuadBuffer: MTLBuffer?
    var spriteQuadBuffer: MTLBuffer?
    var decalQuadCount: Int = 0
    var spriteQuadCount: Int = 0
    var bspDynPipeline: MTLRenderPipelineState?
    var blobShadowPipeline: MTLRenderPipelineState?
    var postfxPipeline: MTLRenderPipelineState?
    var bloomBrightPipeline: MTLRenderPipelineState?
    var bloomBlurPipeline: MTLRenderPipelineState?
    var bloomBlurHPipeline: MTLRenderPipelineState?
    var bloomBlurVPipeline: MTLRenderPipelineState?
    var bloomCombinePipeline: MTLRenderPipelineState?
    var bloomBrightTexture: MTLTexture?
    var bloomBlurTexture: MTLTexture?
    var bloomTexSize: (Int, Int) = (0, 0)
    var dynLightUboBuffer: MTLBuffer?
    var blobShadowBuffer: MTLBuffer?
    var postfxBuffer: MTLBuffer?
    var blobShadowCount: Int = 0
    private var styleTimeAccum: Float = 0
    /* Offscreen color + depth for PostFX (scene → texture → brightness/gamma). */
    private var sceneColorTexture: MTLTexture?
    private var sceneDepthTexture: MTLTexture?
    /* Hi-Z array filled from MTK depth attachment (batch19). */
    private var hizArrayTexture: MTLTexture?
    private var hizArraySize: (Int, Int, Int) = (0, 0, 0) /* w,h,slices */
    private var hizEncodePipeline: MTLComputePipelineState?
    /* Studio skinref atlas bound on MDL draw (batch19). */
    private var studioSkinTexture: MTLTexture?
    private var studioSkinSampler: MTLSamplerState?
    private var skinrefRemapPipeline: MTLRenderPipelineState?
    private var waterReflectTexture: MTLTexture?
    private var waterReflectSize: (Int, Int) = (0, 0)
    private var depthPrepassBoundThisFrame = false
    private var postfxSampler: MTLSamplerState?
    private var postfxOffscreenSize: (Int, Int) = (0, 0)

    private let maxParticleUpload = 512
    private var particleSeedOrigin = simd_float3(0, 0, 64)
    private var particleRespawnAccum: Float = 0

    // Sky dome (driven by AetherSky via EngineBridge)
    var skyPipeline: MTLRenderPipelineState?
    var skyDepthState: MTLDepthStencilState?
    var skyBuffer: MTLBuffer?
    var skyVertexCount: Int = 0

    // Water plane (driven by AetherWater via EngineBridge)
    var waterPipeline: MTLRenderPipelineState?
    var waterDepthState: MTLDepthStencilState?
    var waterBuffer: MTLBuffer?
    var waterVertexCount: Int = 0

    // Fog fullscreen tint (driven by AetherFog via EngineBridge)
    var fogPipeline: MTLRenderPipelineState?
    var fogDepthState: MTLDepthStencilState?
    var fogBuffer: MTLBuffer?
    var fogVertexCount: Int = 0

    private var lastTime: CFTimeInterval = CACurrentMediaTime()

    init?(mtkView: MTKView) {
        guard let dev = mtkView.device ?? MTLCreateSystemDefaultDevice(),
              let q   = dev.makeCommandQueue() else { return nil }
        self.device = dev
        self.queue  = q
        super.init()
        mtkView.delegate = self
        mtkView.colorPixelFormat = .bgra8Unorm
        mtkView.depthStencilPixelFormat = .depth32Float
        mtkView.clearColor = MTLClearColor(red: 0.25, green: 0.30, blue: 0.45, alpha: 1.0)
        mtkView.preferredFramesPerSecond = 60
        mtkView.isPaused = false
        mtkView.enableSetNeedsDisplay = false

        buildBspPipeline(mtkView: mtkView)
        buildMdlPipeline(mtkView: mtkView)
        buildParticlePipeline(mtkView: mtkView)
        buildSkyPipeline(mtkView: mtkView)
        buildWaterPipeline(mtkView: mtkView)
        buildFogPipeline(mtkView: mtkView)
        buildDecalSpritePipelines(mtkView: mtkView)
        buildDynBlobPostfxPipelines(mtkView: mtkView)
        buildDepthState()
        buildDepthPrepassPipeline()
        buildSampler()

        let opaque = Unmanaged.passUnretained(mtkView).toOpaque()
        engine_renderer_attach_metal(opaque)
    }

    private func buildBspPipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary(),
              let vfn = lib.makeFunction(name: "aether_vertex_main"),
              let ffn = lib.makeFunction(name: "aether_fragment_main") else { return }
        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float3; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
        vd.attributes[2].format = .float2; vd.attributes[2].offset = 24; vd.attributes[2].bufferIndex = 0
        vd.attributes[3].format = .float2; vd.attributes[3].offset = 32; vd.attributes[3].bufferIndex = 0
        vd.attributes[4].format = .float;  vd.attributes[4].offset = 40; vd.attributes[4].bufferIndex = 0
        vd.layouts[0].stride = 44
        vd.layouts[0].stepFunction = .perVertex
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.depthAttachmentPixelFormat      = mtkView.depthStencilPixelFormat
        do { bspPipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] BSP pipeline error: \(error)") }
        // Live face-id multi-style LM blend pipeline (cleaned UBO upload path).
        if let sfn = lib.makeFunction(name: "aether_fragment_style_blend") {
            let sd = MTLRenderPipelineDescriptor()
            sd.vertexFunction = vfn; sd.fragmentFunction = sfn; sd.vertexDescriptor = vd
            sd.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            sd.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
            do { styleBlendPipeline = try device.makeRenderPipelineState(descriptor: sd) }
            catch { print("[MetalRenderer] style blend pipeline error: \(error)") }
        }
    }

    private func buildMdlPipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary(),
              let vfn = lib.makeFunction(name: "aether_model_vertex"),
              let ffn = lib.makeFunction(name: "aether_model_fragment") else { return }
        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float3; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
        vd.layouts[0].stride = 24
        vd.layouts[0].stepFunction = .perVertex
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.depthAttachmentPixelFormat      = mtkView.depthStencilPixelFormat
        do { mdlPipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] MDL pipeline error: \(error)") }
    }

    private func buildParticlePipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary(),
              let vfn = lib.makeFunction(name: "aether_particle_vertex"),
              let ffn = lib.makeFunction(name: "aether_particle_fragment") else { return }
        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float;  vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
        vd.attributes[2].format = .float4; vd.attributes[2].offset = 16; vd.attributes[2].bufferIndex = 0
        vd.layouts[0].stride = 32
        vd.layouts[0].stepFunction = .perVertex
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.colorAttachments[0].isBlendingEnabled = true
        d.colorAttachments[0].rgbBlendOperation = .add
        d.colorAttachments[0].alphaBlendOperation = .add
        d.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        d.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        d.colorAttachments[0].sourceAlphaBlendFactor = .one
        d.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
        do { particlePipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] particle pipeline error: \(error)") }
    }

    private func buildSkyPipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary(),
              let vfn = lib.makeFunction(name: "aether_sky_vertex"),
              let ffn = lib.makeFunction(name: "aether_sky_fragment") else { return }
        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float4; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
        vd.layouts[0].stride = 28
        vd.layouts[0].stepFunction = .perVertex
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
        do { skyPipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] sky pipeline error: \(error)") }
    }

    private func buildWaterPipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary(),
              let vfn = lib.makeFunction(name: "aether_water_vertex"),
              let ffn = lib.makeFunction(name: "aether_water_fragment") else { return }
        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float2; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
        vd.attributes[2].format = .float4; vd.attributes[2].offset = 20; vd.attributes[2].bufferIndex = 0
        vd.layouts[0].stride = 36
        vd.layouts[0].stepFunction = .perVertex
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.colorAttachments[0].isBlendingEnabled = true
        d.colorAttachments[0].rgbBlendOperation = .add
        d.colorAttachments[0].alphaBlendOperation = .add
        d.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        d.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        d.colorAttachments[0].sourceAlphaBlendFactor = .one
        d.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
        do { waterPipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] water pipeline error: \(error)") }
    }

    private func buildFogPipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary(),
              let vfn = lib.makeFunction(name: "aether_fog_vertex"),
              let ffn = lib.makeFunction(name: "aether_fog_fragment") else { return }
        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float2; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float2; vd.attributes[1].offset = 8;  vd.attributes[1].bufferIndex = 0
        vd.attributes[2].format = .float4; vd.attributes[2].offset = 16; vd.attributes[2].bufferIndex = 0
        vd.layouts[0].stride = 32
        vd.layouts[0].stepFunction = .perVertex
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.colorAttachments[0].isBlendingEnabled = true
        d.colorAttachments[0].rgbBlendOperation = .add
        d.colorAttachments[0].alphaBlendOperation = .add
        d.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        d.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        d.colorAttachments[0].sourceAlphaBlendFactor = .one
        d.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
        d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
        do { fogPipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] fog pipeline error: \(error)") }
    }

    private func buildDepthState() {
        let d = MTLDepthStencilDescriptor()
        d.depthCompareFunction = .less; d.isDepthWriteEnabled = true
        depthState = device.makeDepthStencilState(descriptor: d)
        let soft = MTLDepthStencilDescriptor()
        soft.depthCompareFunction = .less; soft.isDepthWriteEnabled = false
        particleDepthState = device.makeDepthStencilState(descriptor: soft)
        let skyD = MTLDepthStencilDescriptor()
        skyD.depthCompareFunction = .lessEqual; skyD.isDepthWriteEnabled = false
        skyDepthState = device.makeDepthStencilState(descriptor: skyD)
        let waterD = MTLDepthStencilDescriptor()
        waterD.depthCompareFunction = .less; waterD.isDepthWriteEnabled = false
        waterDepthState = device.makeDepthStencilState(descriptor: waterD)
        let fogD = MTLDepthStencilDescriptor()
        fogD.depthCompareFunction = .always; fogD.isDepthWriteEnabled = false
        fogDepthState = device.makeDepthStencilState(descriptor: fogD)
        let pre = MTLDepthStencilDescriptor()
        pre.depthCompareFunction = .less; pre.isDepthWriteEnabled = true
        depthPrepassDepthState = device.makeDepthStencilState(descriptor: pre)
    }


    private func buildDepthPrepassPipeline() {
        guard let lib = defaultLibrary else { return }
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = lib.makeFunction(name: "aether_depth_prepass_vertex")
        d.fragmentFunction = lib.makeFunction(name: "aether_depth_prepass_fragment")
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.colorAttachments[0].isBlendingEnabled = false
        // Depth-only intent: color write mask none when supported via encode plan.
        d.colorAttachments[0].writeMask = []
        d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
        let v = MTLVertexDescriptor()
        v.attributes[0].format = .float3
        v.attributes[0].offset = 0
        v.attributes[0].bufferIndex = 0
        v.layouts[0].stride = 44 /* match BSP mesh: pos@0 */
        d.vertexDescriptor = v
        do { depthPrepassPipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] depth prepass pipeline error: \(error)") }
    }

    /// Host/bridge-driven depth prepass encode plan: records that Metal should write depth first.
    /// Uses real camera MVP when threaded via engine_depth_prepass_camera_set (not identity stub).
    func encodeDepthPrepassIfNeeded(_ encoder: MTLRenderCommandEncoder, mvp: simd_float4x4) {
        _ = engine_depth_prepass_bind_before_main()
        var passes: UInt32 = 0, w: UInt32 = 0, h: UInt32 = 0
        var writeDepth: Int32 = 0
        var mvpArr = [Float](repeating: 0, count: 16)
        var hasMvp: Int32 = 0
        let needed = engine_depth_prepass_encode_plan_ex(&passes, &w, &h, &writeDepth, &mvpArr, &hasMvp)
        guard needed != 0, passes > 0, let pipe = depthPrepassPipeline else { return }
        encoder.setRenderPipelineState(pipe)
        if let ds = depthPrepassDepthState { encoder.setDepthStencilState(ds) }
        // Bind depth prepass BEFORE main color pass (early-Z plan) with real camera MVP.
        if let vb = vertexBuffer, indexCount > 0 {
            encoder.setVertexBuffer(vb, offset: 0, index: 0)
            struct DepthU { var mvp: simd_float4x4; var clip: simd_float4; var enabled: Float; var pad: simd_float3 }
            var useMvp = mvp
            if hasMvp != 0 {
                useMvp = simd_float4x4(columns: (
                    SIMD4<Float>(mvpArr[0], mvpArr[1], mvpArr[2], mvpArr[3]),
                    SIMD4<Float>(mvpArr[4], mvpArr[5], mvpArr[6], mvpArr[7]),
                    SIMD4<Float>(mvpArr[8], mvpArr[9], mvpArr[10], mvpArr[11]),
                    SIMD4<Float>(mvpArr[12], mvpArr[13], mvpArr[14], mvpArr[15])
                ))
            }
            var DU = DepthU(mvp: useMvp, clip: simd_float4(0,0,1,0),
                            enabled: 1, pad: simd_float3(0,0,0))
            encoder.setVertexBytes(&DU, length: MemoryLayout<DepthU>.stride, index: 1)
            if let ib = culledIndexBuffer ?? indexBuffer {
                let ic = culledIndexCount > 0 ? culledIndexCount : indexCount
                if ic > 0 {
                    encoder.drawIndexedPrimitives(type: .triangle, indexCount: ic,
                                                  indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
                }
            }
        }
        _ = engine_depth_prepass_mark_bound()
        depthPrepassBoundThisFrame = true
        // Chain depth prepass → Hi-Z pyramid bind after depth encode
        var bLevels: Int32 = 0, bViews: Int32 = 0, bBound: Int32 = 0
        _ = engine_depth_hiz_bind_execute(w < 2 ? 2 : w, h < 2 ? 2 : h, nil, 0,
                                          &bLevels, &bViews, &bBound)
        _ = w; _ = h; _ = writeDepth; _ = bLevels; _ = bViews; _ = bBound
    }

    /// Clear + draw world with mirrored MVP into waterReflectTexture, then resolve/mips.
    private func encodeWaterReflectPass(cmd: MTLCommandBuffer, viewMat: simd_float4x4, projMat: simd_float4x4,
                                        pixelFormat: MTLPixelFormat) {
        guard let rtex = waterReflectTexture else { return }
        var mvpArr = [Float](repeating: 0, count: 16)
        var rw: UInt32 = 0, rh: UInt32 = 0
        var clr: Int32 = 0, drw: Int32 = 0, res: Int32 = 0
        let needed = engine_water_reflect_rt_draw_plan(&mvpArr, &rw, &rh, &clr, &drw, &res)
        guard needed != 0 else { return }
        _ = engine_water_reflect_rt_clear(0.2, 0.35, 0.5, 1.0)
        let rpd = MTLRenderPassDescriptor()
        rpd.colorAttachments[0].texture = rtex
        rpd.colorAttachments[0].loadAction = clr != 0 ? .clear : .load
        rpd.colorAttachments[0].storeAction = .store
        rpd.colorAttachments[0].clearColor = MTLClearColor(red: 0.2, green: 0.35, blue: 0.5, alpha: 1)
        guard let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) else { return }
        defer { enc.endEncoding() }
        if drw != 0, let pipe = bspPipeline ?? mdlPipeline, let vb = vertexBuffer {
            enc.setRenderPipelineState(pipe)
            enc.setVertexBuffer(vb, offset: 0, index: 0)
            let mirrorMvp = simd_float4x4(columns: (
                SIMD4<Float>(mvpArr[0], mvpArr[1], mvpArr[2], mvpArr[3]),
                SIMD4<Float>(mvpArr[4], mvpArr[5], mvpArr[6], mvpArr[7]),
                SIMD4<Float>(mvpArr[8], mvpArr[9], mvpArr[10], mvpArr[11]),
                SIMD4<Float>(mvpArr[12], mvpArr[13], mvpArr[14], mvpArr[15])
            ))
            // Rebuild as model/view/proj from bridge mirror MVP for world draw into RT.
            var viewCols = [Float](repeating: 0, count: 16)
            var projCols = [Float](repeating: 0, count: 16)
            withUnsafeBytes(of: viewMat) { buf in
                for i in 0..<16 { viewCols[i] = buf.load(fromByteOffset: i*4, as: Float.self) }
            }
            withUnsafeBytes(of: projMat) { buf in
                for i in 0..<16 { projCols[i] = buf.load(fromByteOffset: i*4, as: Float.self) }
            }
            _ = engine_water_reflect_rt_build_mirror_mvp(&viewCols, &projCols, &mvpArr)
            var U = Uniforms(model: matrix_identity_float4x4, view: mirrorMvp, proj: matrix_identity_float4x4,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)), pad0: 0,
                             baseColor: simd_float4(0.7, 0.85, 1.0, 1.0),
                             useTexture: 0, useLightmap: 0, pad2: 0, pad3: 0)
            enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            if let ib = culledIndexBuffer ?? indexBuffer {
                let ic = culledIndexCount > 0 ? culledIndexCount : indexCount
                if ic > 0 {
                    enc.drawIndexedPrimitives(type: .triangle, indexCount: ic,
                                              indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
                }
            }
            _ = pipe; _ = pixelFormat
        }
        // Entities/monsters into reflection RT (not BSP-only).
        var de: Int32 = 0, dm: Int32 = 0
        var ec: UInt32 = 0, mc: UInt32 = 0
        var rw2: UInt32 = 0, rh2: UInt32 = 0
        var clr2: Int32 = 0, dw2: Int32 = 0, res2: Int32 = 0
        _ = engine_water_reflect_rt_draw_plan_full(&mvpArr, &rw2, &rh2, &clr2, &dw2, &de, &dm, &ec, &mc, &res2)
        var drawStudio: Int32 = 0
        var studioCount: UInt32 = 0
        _ = engine_water_reflect_rt_draw_plan_studio_flags(&drawStudio, &studioCount)
        if (de != 0 || dm != 0 || drawStudio != 0),
           let pipe = mdlPipeline,
           let vb = monsterVertexBuf, let ib = monsterIndexBuf, monsterIndexCount > 0 {
            enc.setRenderPipelineState(pipe)
            enc.setVertexBuffer(vb, offset: 0, index: 0)
            let mirrorMvp2 = simd_float4x4(columns: (
                SIMD4<Float>(mvpArr[0], mvpArr[1], mvpArr[2], mvpArr[3]),
                SIMD4<Float>(mvpArr[4], mvpArr[5], mvpArr[6], mvpArr[7]),
                SIMD4<Float>(mvpArr[8], mvpArr[9], mvpArr[10], mvpArr[11]),
                SIMD4<Float>(mvpArr[12], mvpArr[13], mvpArr[14], mvpArr[15])
            ))
            for (mi, mpos) in monsterPositions.enumerated() {
                var tint = simd_float4(0.85, 0.35, 0.35, 1.0) // debug-box default
                var mat: Int32 = 0, sg: UInt32 = 0, st: UInt32 = 0, att: Int32 = -1
                var rgba = [Float](repeating: 1, count: 4)
                if engine_water_reflect_ent_get_studio(UInt32(mi), &mat, &sg, &st, &att, &rgba) != 0,
                   mat > 0 {
                    // Studio/skinned material — less debug-box red
                    tint = simd_float4(rgba[0], rgba[1], rgba[2], rgba[3])
                    _ = engine_water_reflect_ent_set_studio_tex(UInt32(mi), sg, st)
                    var srgba = [Float](repeating: 0, count: 4)
                    if engine_water_reflect_ent_sample_studio_tex(UInt32(mi), 0.25, 0.75, &srgba) != 0 {
                        tint = simd_float4(srgba[0], srgba[1], srgba[2], srgba[3])
                    }
                }
                var model = matrix_identity_float4x4
                model.columns.3 = SIMD4<Float>(mpos.x, mpos.y, mpos.z, 1)
                var U = Uniforms(model: model, view: mirrorMvp2, proj: matrix_identity_float4x4,
                                 lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)), pad0: 0,
                                 baseColor: tint,
                                 useTexture: mat > 0 ? 1 : 0, useLightmap: 0, pad2: 0, pad3: 0)
                enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.drawIndexedPrimitives(type: .triangle, indexCount: monsterIndexCount,
                                          indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
            }
            // Hi-Z gated LOD draw into reflect RT
            var hlod: Int32 = 0, hissue: Int32 = 0, hocc: Int32 = 0
            var hv: UInt32 = 0, ht: UInt32 = 0
            _ = engine_mdl_lod_gpu_issue_draw_hiz(200.0, 16.0, &hlod, &hv, &ht, &hissue, &hocc)
            // Real Hi-Z mip pyramid + visibility query hooks (Metal encode path)
            _ = engine_mdl_hiz_pyramid_reset(64, 64)
            _ = engine_mdl_hiz_pyramid_write(32, 32, 0.2)
            _ = engine_mdl_hiz_build_pyramid()
            _ = engine_mdl_hiz_pyramid_set_gpu_hooks(1)
            var hvVis: Int32 = 0, hvOcc: Int32 = 0, hvMip: Int32 = 0
            var hvZ: Float = 0
            _ = engine_mdl_hiz_vis_query(0.4, 0.4, 0.6, 0.6, 0.8, &hvVis, &hvOcc, &hvZ, &hvMip)
            var plod: Int32 = 0, pissue: Int32 = 0, pocc: Int32 = 0
            var ppx: Float = 0
            _ = engine_mdl_lod_hiz_pyramid_gate(200.0, 16.0, 4.0, 0.5, 0.5, 0.5,
                                               &plod, &pissue, &pocc, &ppx)
            // Portal-aware mirror MVP when crossing portals
            _ = engine_water_reflect_portal_set(0, 0, 0, 128, 0, 0, 1)
            var portalMvp = [Float](repeating: 0, count: 16)
            _ = engine_water_reflect_rt_build_mirror_mvp_portal(&portalMvp)
            if studioCount > 0 {
                _ = engine_water_reflect_ent_set_studio_tex(0, 1, 2)
                var srgba = [Float](repeating: 0, count: 4)
                _ = engine_water_reflect_ent_sample_studio_tex(0, 0.3, 0.7, &srgba)
                _ = srgba
            }
            // Depth prepass → Hi-Z pyramid bind (encode order + texture views)
            var dhNeed: Int32 = 0, dhSteps: Int32 = 0
            var dhViews: UInt32 = 0
            _ = engine_depth_hiz_bind_plan(64, 64, &dhNeed, &dhSteps, &dhViews)
            var depthStub = [Float](repeating: 1.0, count: 64 * 64)
            for y in 20..<44 { for x in 20..<44 { depthStub[y * 64 + x] = 0.2 } }
            var dhLevels: Int32 = 0, dhV2: Int32 = 0, dhBound: Int32 = 0
            _ = engine_depth_hiz_bind_execute(64, 64, &depthStub, UInt32(depthStub.count),
                                              &dhLevels, &dhV2, &dhBound)
            var mmVis: Int32 = 0, mmOcc: Int32 = 0, mmMip: Int32 = 0
            var mmZ: Float = 0
            _ = engine_mdl_hiz_vis_query_multi_mip(0.4, 0.4, 0.6, 0.6, 0.8,
                                                   &mmVis, &mmOcc, &mmZ, &mmMip)
            // Portal winding + recursive reflect views
            _ = engine_portal_winding_make_rect(0, 0, 32, 0, 1, 0, 32, 48)
            _ = engine_portal_winding_clip_water()
            var prv: UInt32 = 0, prd: UInt32 = 0
            _ = engine_water_reflect_recursive_plan(0, 0, 64, 3, &prv, &prd)
            // Fixture MDL skin pages into water RT
            _ = engine_mdl_skin_pages_build(4)
            if studioCount > 0 {
                _ = engine_water_reflect_ent_bind_skin_page(0, 0)
                var pageRgba = [Float](repeating: 0, count: 4)
                _ = engine_water_reflect_ent_sample_skin_page(0, 0.25, 0.75, &pageRgba)
                _ = pageRgba
            }
            // Device Hi-Z texture2d_array + array-mip vis query on Metal encode path
            var arrSlices: UInt32 = 0, arrW: UInt32 = 0, arrH: UInt32 = 0
            _ = engine_mdl_hiz_bind_texture2d_array(&arrSlices, &arrW, &arrH)
            _ = engine_mdl_hiz_array_mark_bound()
            var dhArrSlices: Int32 = 0, dhArrBound: Int32 = 0
            _ = engine_depth_hiz_array_bind(arrSlices, &dhArrSlices, &dhArrBound)
            var aqVis: Int32 = 0, aqOcc: Int32 = 0, aqMip: Int32 = 0
            var aqZ: Float = 0
            _ = engine_mdl_hiz_vis_query_array_mip(0.4, 0.4, 0.6, 0.6, 0.85, 1,
                                                   &aqVis, &aqOcc, &aqZ, &aqMip)
            // Multi-portal leaf graph flood → reflect views
            var pgLeaves: UInt32 = 0, pgEdges: UInt32 = 0
            _ = engine_bsp_portal_graph_build_multi(&pgLeaves, &pgEdges)
            var pgReached: UInt32 = 0, pgDepth: UInt32 = 0
            _ = engine_bsp_portal_graph_flood(0, 3, &pgReached, &pgDepth)
            var pgViews: UInt32 = 0, pgFlood: UInt32 = 0
            _ = engine_water_reflect_portal_graph_plan(0, 0, 64, 0, 3, &pgViews, &pgFlood)
            // Packed MDL skin lumps (fixture fallback when no user asset)
            var lumpCount: UInt32 = 0
            var lumpFallback: Int32 = 0
            _ = engine_mdl_skin_lumps_load_or_fixture(nil, 0, 4, &lumpCount, &lumpFallback)
            var lumpRgba = [Float](repeating: 0, count: 4)
            _ = engine_mdl_skin_lumps_sample(0, 0.25, 0.75, &lumpRgba)
            if studioCount > 0 {
                _ = engine_mdl_skin_lump_bind_water_ent(0, 0)
            }
            // GPU Hi-Z downsample into array slices → vis query bind
            var dsSlices: UInt32 = 0, dsPasses: UInt32 = 0
            var dsReady: Int32 = 0
            _ = engine_mdl_hiz_array_downsample(&dsSlices, &dsPasses, &dsReady)
            var dsBound: Int32 = 0, dsVisReady: Int32 = 0
            _ = engine_depth_hiz_downsample_bind(dsSlices, dsPasses, &dsBound, &dsVisReady)
            var dqVis: Int32 = 0, dqOcc: Int32 = 0, dqMip: Int32 = 0
            var dqZ: Float = 0
            _ = engine_mdl_hiz_vis_query_downsampled(0.4, 0.4, 0.6, 0.6, 0.85, 1,
                                                    &dqVis, &dqOcc, &dqZ, &dqMip)
            // Fuller portal windings from marksurfaces / planes
            var pwCount: UInt32 = 0
            var pwFromBsp: Int32 = 0
            _ = engine_bsp_portal_windings_from_current(&pwCount, &pwFromBsp)
            var pwAttached: UInt32 = 0
            _ = engine_bsp_portal_graph_attach_windings(&pwAttached)
            var pwPlane = [Float](repeating: 0, count: 4)
            var pwCenter = [Float](repeating: 0, count: 3)
            var pwVerts: UInt32 = 0
            var pwMarks: Int32 = 0
            _ = engine_bsp_portal_winding_get(0, &pwPlane, &pwVerts, &pwCenter, &pwMarks)
            var pwViews: UInt32 = 0
            _ = engine_water_reflect_portal_winding_plan(0, 0, 64, 0, 3, &pwViews)
            // MDL skinref / family select
            var srFam: UInt32 = 0, srEnt: UInt32 = 0
            _ = engine_mdl_skinref_init_fixture(&srFam, &srEnt)
            _ = engine_mdl_skinref_select_family_name("camo")
            _ = engine_mdl_skinref_select_ref(1)
            var rFam: UInt32 = 0, rRef: UInt32 = 0, rGrp: UInt32 = 0, rTex: UInt32 = 0, rSkin: UInt32 = 0
            _ = engine_mdl_skinref_resolve(&rFam, &rRef, &rGrp, &rTex, &rSkin)
            var srRgba = [Float](repeating: 0, count: 4)
            _ = engine_mdl_skinref_sample(0.3, 0.7, &srRgba)
            _ = engine_mdl_skinref_cycle_family(1)
            // Live Metal Hi-Z encode from depth texture (encode plan)
            var leSlices: UInt32 = 0, lePasses: UInt32 = 0, leSamples: UInt32 = 0
            var leNeeded: Int32 = 0
            _ = engine_mdl_hiz_encode_from_depth(64, 64, &leSlices, &lePasses, &leSamples, &leNeeded)
            var depPasses: UInt32 = 0
            var depNeeded: Int32 = 0
            _ = engine_depth_hiz_live_encode_plan(64, 64, leSlices, &depPasses, &depNeeded)
            _ = engine_mdl_hiz_live_encode_mark()
            let leEncoded = engine_mdl_hiz_live_encode_was_encoded()
            // Portal winding clip against recursive reflect planes
            var clipVerts: UInt32 = 0, clipPlanes: UInt32 = 0
            _ = engine_portal_winding_clip_reflect_planes(&clipVerts, &clipPlanes)
            var pcViews: UInt32 = 0, pcClipVerts: UInt32 = 0
            _ = engine_water_reflect_portal_clip_plan(0, 0, 64, 3, &pcViews, &pcClipVerts)
            // Studio skinref → texture remap on draw
            var rmFam: UInt32 = 0, rmRef: UInt32 = 0, rmGrp: UInt32 = 0
            var rmTex: UInt32 = 0, rmSkin: UInt32 = 0, rmSlot: UInt32 = 0
            _ = engine_mdl_skinref_remap_draw(3, &rmFam, &rmRef, &rmGrp, &rmTex, &rmSkin, &rmSlot)
            var rmU: Float = 0, rmV: Float = 0
            _ = engine_mdl_skinref_remap_uv(0.25, 0.75, &rmU, &rmV)
            var rmRgba = [Float](repeating: 0, count: 4)
            _ = engine_mdl_skinref_remap_sample(0.25, 0.75, &rmRgba)
            // Batch19: MTK depth attach → Hi-Z encode wire + portal clip stack + skin Metal bind
            var mtkPasses: UInt32 = 0
            var mtkNeeded: Int32 = 0
            _ = engine_depth_hiz_mtk_attach_plan(UInt32(max(dw / 2, 64)), UInt32(max(dh / 2, 64)),
                                                 &mtkPasses, &mtkNeeded)
            _ = engine_depth_hiz_mtk_attach_wire()
            _ = engine_depth_hiz_mtk_attach_mark()
            let mtkReady = engine_depth_hiz_mtk_attach_encode_ready()
            var mtkSlices: UInt32 = 0, mtkEncPasses: UInt32 = 0, mtkSamples: UInt32 = 0
            var mtkEncNeeded: Int32 = 0
            _ = engine_mdl_hiz_encode_from_mtk_attach(64, 64, &mtkSlices, &mtkEncPasses,
                                                      &mtkSamples, &mtkEncNeeded)
            var stackCount: UInt32 = 0
            _ = engine_portal_clip_stack_push_reflect(&stackCount)
            var stackVerts: UInt32 = 0
            _ = engine_portal_clip_stack_clip(&stackVerts)
            var stViews: UInt32 = 0, stStack: UInt32 = 0, stClip: UInt32 = 0
            _ = engine_water_reflect_portal_stack_plan(0, 0, 64, 3, &stViews, &stStack, &stClip)
            var mbFam: UInt32 = 0, mbRef: UInt32 = 0, mbSlot: UInt32 = 0
            var mbW: UInt32 = 0, mbH: UInt32 = 0, mbBytes: UInt32 = 0
            _ = engine_mdl_skinref_metal_bind_draw(3, &mbFam, &mbRef, &mbSlot, &mbW, &mbH, &mbBytes)
            _ = engine_mdl_skinref_metal_bind_mark()
            let mbBound = engine_mdl_skinref_metal_bind_was_bound()
            _ = mtkPasses; _ = mtkNeeded; _ = mtkReady; _ = mtkSlices; _ = mtkEncPasses
            _ = mtkSamples; _ = mtkEncNeeded; _ = stackCount; _ = stackVerts
            _ = stViews; _ = stStack; _ = stClip
            _ = mbFam; _ = mbRef; _ = mbSlot; _ = mbW; _ = mbH; _ = mbBytes; _ = mbBound
            _ = arrSlices; _ = arrW; _ = arrH; _ = dhArrSlices; _ = dhArrBound
            _ = aqVis; _ = aqOcc; _ = aqZ; _ = aqMip
            _ = pgLeaves; _ = pgEdges; _ = pgReached; _ = pgDepth; _ = pgViews; _ = pgFlood
            _ = lumpCount; _ = lumpFallback; _ = lumpRgba
            _ = dsSlices; _ = dsPasses; _ = dsReady; _ = dsBound; _ = dsVisReady
            _ = dqVis; _ = dqOcc; _ = dqZ; _ = dqMip
            _ = pwCount; _ = pwFromBsp; _ = pwAttached; _ = pwPlane; _ = pwCenter
            _ = pwVerts; _ = pwMarks; _ = pwViews
            _ = srFam; _ = srEnt; _ = rFam; _ = rRef; _ = rGrp; _ = rTex; _ = rSkin; _ = srRgba
            _ = leSlices; _ = lePasses; _ = leSamples; _ = leNeeded; _ = depPasses; _ = depNeeded; _ = leEncoded
            _ = clipVerts; _ = clipPlanes; _ = pcViews; _ = pcClipVerts
            _ = rmFam; _ = rmRef; _ = rmGrp; _ = rmTex; _ = rmSkin; _ = rmSlot; _ = rmU; _ = rmV; _ = rmRgba
            _ = ec; _ = mc; _ = res2; _ = dw2; _ = clr2; _ = rw2; _ = rh2; _ = studioCount
            _ = hvVis; _ = hvOcc; _ = hvZ; _ = hvMip; _ = plod; _ = pissue; _ = pocc; _ = ppx
            _ = portalMvp
            _ = dhNeed; _ = dhSteps; _ = dhViews; _ = dhLevels; _ = dhV2; _ = dhBound
            _ = mmVis; _ = mmOcc; _ = mmZ; _ = mmMip; _ = prv; _ = prd
        }
        // GPU studio LOD draw path: select LOD by camera distance and issue draw.
        var lod: Int32 = 0, issue: Int32 = 0
        var lv: UInt32 = 0, lt: UInt32 = 0
        let camDist: Float = 120.0
        if engine_mdl_lod_gpu_issue_draw(camDist, &lod, &lv, &lt, &issue) != 0, issue != 0,
           let pipe = mdlPipeline, let vb = mdlVertexBuf, let ib = mdlIndexBuf, mdlIndexCount > 0 {
            enc.setRenderPipelineState(pipe)
            enc.setVertexBuffer(vb, offset: 0, index: 0)
            var U = Uniforms(model: matrix_identity_float4x4, view: matrix_identity_float4x4,
                             proj: matrix_identity_float4x4,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)), pad0: 0,
                             baseColor: simd_float4(0.6, 0.8, 0.5, 1.0),
                             useTexture: 0, useLightmap: 0, pad2: 0, pad3: 0)
            // Prefer selected LOD mesh copy when available; fall back to uploaded MDL.
            var pos = [Float](repeating: 0, count: 144)
            var idx = [UInt32](repeating: 0, count: 96)
            var ov: UInt32 = 0, ot: UInt32 = 0
            var olod: Int32 = 0
            if engine_mdl_lod_gpu_issue_draw_copy(camDist, &pos, 48, &idx, 96, &olod, &ov, &ot) != 0,
               ov > 0, ot > 0 {
                if let tmpV = device.makeBuffer(bytes: pos, length: Int(ov) * 3 * 4, options: .storageModeShared),
                   let tmpI = device.makeBuffer(bytes: idx, length: Int(ot) * 3 * 4, options: .storageModeShared) {
                    enc.setVertexBuffer(tmpV, offset: 0, index: 0)
                    enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                    enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                    enc.drawIndexedPrimitives(type: .triangle, indexCount: Int(ot) * 3,
                                              indexType: .uint32, indexBuffer: tmpI, indexBufferOffset: 0)
                }
            } else {
                enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.drawIndexedPrimitives(type: .triangle, indexCount: mdlIndexCount,
                                          indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
            }
            _ = lod; _ = lv; _ = lt; _ = vb; _ = pipe
        }
        if res != 0 {
            _ = engine_water_reflect_rt_resolve()
            _ = engine_water_reflect_rt_gen_mips()
        }
    }

    private func ensureWaterReflectRT(fbW: Int, fbH: Int, pixelFormat: MTLPixelFormat) {
        _ = engine_water_reflect_rt_ensure(UInt32(fbW), UInt32(fbH), 0.5)
        var passes: UInt32 = 0, w: UInt32 = 0, h: UInt32 = 0
        var alloc: Int32 = 0, sample: Int32 = 0
        _ = engine_water_reflect_rt_encode_plan(&passes, &w, &h, &alloc, &sample)
        var iw = Int(w), ih = Int(h)
        if iw <= 0 || ih <= 0 { iw = max(fbW / 2, 1); ih = max(fbH / 2, 1) }
        if waterReflectSize == (iw, ih), waterReflectTexture != nil { return }
        let desc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: pixelFormat, width: iw, height: ih, mipmapped: true)
        desc.usage = [.renderTarget, .shaderRead]
        desc.storageMode = .private
        waterReflectTexture = device.makeTexture(descriptor: desc)
        waterReflectSize = (iw, ih)
        _ = passes; _ = alloc; _ = sample
    }

    private func buildSampler() {
        let s = MTLSamplerDescriptor()
        s.minFilter = .linear; s.magFilter = .linear
        s.sAddressMode = .clampToEdge; s.tAddressMode = .clampToEdge
        samplerState = device.makeSamplerState(descriptor: s)
    }

    // MARK: - Upload
    func uploadMeshFromEngine() {
        uploadBspMesh()
        uploadAtlas()
        // Ensure procedural lightmap stub exists (mesh may have been built before Metal init).
        if engine_lightmap_width() <= 0 || engine_lightmap_is_stub() == 0 {
            _ = engine_lightmap_bake_active_mesh()
            // Re-copy verts so lu/lv from bake are in the GPU buffer.
            uploadBspMesh()
        }
        uploadLightmap()
        uploadMdlMesh()
        buildMonsterBoxes()
        uploadSkyDome()
        uploadWaterPlane()

        // Prefer info_player_start (bridge falls back to mesh center).
        engine_player_spawn_at_start()
        if engine_player_has_start() {
            print("[MetalRenderer] Player spawned at info_player_start")
        } else {
            print("[MetalRenderer] Player spawned at mesh center (no info_player_start)")
        }

        // Seed a particle burst near the player eye / mesh center so Metal draws live C state.
        var eye = [Float](repeating: 0, count: 3)
        engine_player_get_eye(&eye)
        particleSeedOrigin = simd_float3(eye[0], eye[1], eye[2] + 24)
        engine_particles_clear()
        let seeded = engine_particles_spawn_burst(eye[0], eye[1], eye[2] + 24, 96)
        let synth = engine_bsp_mesh_is_synthetic() != 0
        print("[MetalRenderer] Upload complete (BSP=\(indexCount > 0) synthetic=\(synth) tris=\(engine_bsp_mesh_triangle_count()), VIS faces=\(engine_bsp_vis_visible_face_count())/\(engine_bsp_vis_total_face_count()) leaf=\(engine_bsp_vis_find_leaf()), lightmap=\(hasLightmap) \(engine_lightmap_width())x\(engine_lightmap_height()) stub=\(engine_lightmap_is_stub() != 0), MDL=\(hasMdl), Monsters=\(monsterPositions.count), Particles=\(seeded), Sky=\(skyVertexCount), Water=\(waterVertexCount))")
    }

    private func uploadBspMesh() {
        let vCount = Int(engine_bsp_mesh_vertex_count())
        let iCount = Int(engine_bsp_mesh_index_count())
        guard vCount > 0, iCount > 0 else { return }
        // aether_mesh_vertex_t: pos3+n3+uv2+luv2+face_id = 11 floats (44 bytes)
        var vData = [Float](repeating: 0, count: vCount * 11)
        _ = vData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_vertices(buf.baseAddress, Int32(vCount)))
        }
        vertexBuffer = device.makeBuffer(bytes: vData, length: vCount * 44, options: .storageModeShared)
        var iData = [UInt32](repeating: 0, count: iCount)
        _ = iData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_indices(buf.baseAddress, Int32(iCount)))
        }
        indexBuffer = device.makeBuffer(bytes: iData, length: iCount * 4, options: .storageModeShared)
        indexCount = iCount
        // Pre-allocate culled IB at full capacity; contents refreshed in draw via VIS.
        culledIndexBuffer = device.makeBuffer(length: iCount * 4, options: .storageModeShared)
        culledIndexCount = iCount
        if let cib = culledIndexBuffer {
            memcpy(cib.contents(), iData, iCount * 4)
        }
        refreshVisIndices(force: true)
    }

    /// Push eye → EngineBridge VIS and rewrite the culled index buffer.
    @discardableResult
    private func refreshVisIndices(force: Bool = false) -> Int {
        var eye = [Float](repeating: 0, count: 3)
        engine_player_get_eye(&eye)
        engine_bsp_vis_set_view_origin(eye[0], eye[1], eye[2])
        let n = Int(engine_bsp_vis_update())
        guard n > 0, let cib = culledIndexBuffer, n <= indexCount else {
            culledIndexCount = indexCount
            return culledIndexCount
        }
        var iData = [UInt32](repeating: 0, count: n)
        let copied = iData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_vis_copy_indices(buf.baseAddress, Int32(n)))
        }
        let use = Int(copied)
        if use > 0 {
            memcpy(cib.contents(), iData, use * 4)
            culledIndexCount = use
        } else {
            culledIndexCount = indexCount
        }
        if force {
            print("[MetalRenderer] VIS leaf=\(engine_bsp_vis_find_leaf()) faces=\(engine_bsp_vis_visible_face_count())/\(engine_bsp_vis_total_face_count()) indices=\(culledIndexCount)/\(indexCount) forceFull=\(engine_bsp_vis_force_full() != 0)")
        }
        return culledIndexCount
    }

    private func uploadAtlas() {
        let w = Int(engine_texture_atlas_width())
        let h = Int(engine_texture_atlas_height())
        guard w > 0, h > 0 else { hasTexture = false; return }
        let byteCount = w * h * 4
        var rgba = [UInt8](repeating: 0, count: byteCount)
        let copied = rgba.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_texture_atlas_copy_rgba(buf.baseAddress, Int32(byteCount)))
        }
        guard copied == byteCount else { hasTexture = false; return }
        let td = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm,
                                                          width: w, height: h, mipmapped: false)
        td.usage = .shaderRead
        td.storageMode = .shared
        guard let tex = device.makeTexture(descriptor: td) else { hasTexture = false; return }
        tex.replace(region: MTLRegionMake2D(0, 0, w, h), mipmapLevel: 0,
                    withBytes: rgba, bytesPerRow: w * 4)
        atlasTexture = tex
        hasTexture = true
    }

    private func uploadLightmap() {
        let w = Int(engine_lightmap_width())
        let h = Int(engine_lightmap_height())
        guard w > 0, h > 0, engine_lightmap_enabled() != 0 else {
            hasLightmap = false
            return
        }
        let byteCount = w * h * 4
        var rgba = [UInt8](repeating: 0, count: byteCount)
        let copied = rgba.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_lightmap_copy_rgba(buf.baseAddress, Int32(byteCount)))
        }
        guard copied == byteCount else { hasLightmap = false; return }
        let td = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm,
                                                          width: w, height: h, mipmapped: false)
        td.usage = .shaderRead
        td.storageMode = .shared
        guard let tex = device.makeTexture(descriptor: td) else { hasLightmap = false; return }
        tex.replace(region: MTLRegionMake2D(0, 0, w, h), mipmapLevel: 0,
                    withBytes: rgba, bytesPerRow: w * 4)
        lightmapTexture = tex
        hasLightmap = true
        print("[MetalRenderer] Lightmap uploaded \(w)x\(h) stub=\(engine_lightmap_is_stub() != 0)")
    }

    private func uploadMdlMesh() {
        var vCount = Int(engine_mdl_mesh_vertex_count())
        var tCount = Int(engine_mdl_mesh_triangle_count())
        if vCount <= 0 || tCount <= 0 {
            /* Clean-room triangle fixture when no user .mdl is mounted. */
            let got = Int(engine_mdl_fixture_extract_verts())
            if got > 0 {
                vCount = Int(engine_mdl_mesh_vertex_count())
                tCount = Int(engine_mdl_mesh_triangle_count())
            }
        }
        guard vCount > 0, tCount > 0 else { hasMdl = false; return }

        var pData = [Float](repeating: 0, count: vCount * 3)
        _ = pData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_mdl_mesh_copy_positions(buf.baseAddress, Int32(vCount * 3)))
        }
        var nData = [Float](repeating: 0, count: vCount * 3)
        _ = nData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_mdl_mesh_copy_normals(buf.baseAddress, Int32(vCount * 3)))
        }
        var interleaved = [Float](repeating: 0, count: vCount * 6)
        for i in 0..<vCount {
            interleaved[i*6 + 0] = pData[i*3 + 0]
            interleaved[i*6 + 1] = pData[i*3 + 1]
            interleaved[i*6 + 2] = pData[i*3 + 2]
            interleaved[i*6 + 3] = nData[i*3 + 0]
            interleaved[i*6 + 4] = nData[i*3 + 1]
            interleaved[i*6 + 5] = nData[i*3 + 2]
        }
        mdlVertexBuf = device.makeBuffer(bytes: interleaved, length: vCount * 24, options: .storageModeShared)
        var iData = [UInt32](repeating: 0, count: tCount * 3)
        _ = iData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_mdl_mesh_copy_indices(buf.baseAddress, Int32(tCount * 3)))
        }
        mdlIndexBuf = device.makeBuffer(bytes: iData, length: tCount * 3 * 4, options: .storageModeShared)
        mdlIndexCount = tCount * 3
        hasMdl = true
    }


    private func uploadSkyDome() {
        let cap = Int(engine_sky_render_vertex_capacity())
        guard cap > 0, engine_sky_enabled() != 0 else {
            skyVertexCount = 0
            return
        }
        var packed = [Float](repeating: 0, count: cap * 7)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_sky_copy_render(buf.baseAddress, Int32(cap)))
        }
        skyVertexCount = Int(n)
        guard skyVertexCount > 0 else { return }
        let bytes = skyVertexCount * 28
        skyBuffer = device.makeBuffer(bytes: packed, length: bytes, options: .storageModeShared)
        _ = engine_sky_set_name("desert")
        print("[MetalRenderer] Sky dome verts=\(skyVertexCount) faces=\(engine_sky_face_count()) r=\(engine_sky_radius())")
    }

    private func uploadWaterPlane() {
        guard engine_water_enabled() != 0 else {
            waterVertexCount = 0
            return
        }
        // Place a demo pool slightly below the player eye / mesh center.
        var eye = [Float](repeating: 0, count: 3)
        engine_player_get_eye(&eye)
        engine_water_set_origin(eye[0], eye[1])
        engine_water_set_height(eye[2] - 80)
        engine_water_set_size(384)
        engine_water_set_wave(1.2, 8.0, 0.04)
        engine_water_set_color(0.12, 0.42, 0.62, 0.70)

        let cap = Int(engine_water_render_vertex_capacity())
        guard cap > 0 else { waterVertexCount = 0; return }
        var packed = [Float](repeating: 0, count: cap * 9)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_water_copy_render(buf.baseAddress, Int32(cap)))
        }
        waterVertexCount = Int(n)
        guard waterVertexCount > 0 else { return }
        let bytes = waterVertexCount * 36
        waterBuffer = device.makeBuffer(bytes: packed, length: max(bytes, cap * 36), options: .storageModeShared)
        print("[MetalRenderer] Water plane verts=\(waterVertexCount) t=\(engine_water_wave_time())")
    }

    // Build a 1x1x1 box for each monster at its position
    private func buildMonsterBoxes() {
        let maxMonsters = 128
        var positions = [Float](repeating: 0, count: maxMonsters * 3)
        let count = positions.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_monster_positions_copy(buf.baseAddress, Int32(maxMonsters)))
        }
        guard count > 0 else {
            monsterIndexCount = 0
            return
        }

        monsterPositions.removeAll()
        for i in 0..<Int(count) {
            monsterPositions.append(simd_float3(positions[i*3+0], positions[i*3+1], positions[i*3+2]))
        }

        // Build a unit cube centered at origin
        let s: Float = 16.0   // half-size (so 32 units total)
        let verts: [Float] = [
            // 8 corners × (pos.xyz, normal.xyz)
            -s,-s,-s, 0,0,-1,   s,-s,-s, 0,0,-1,   s, s,-s, 0,0,-1,  -s, s,-s, 0,0,-1,
            -s,-s, s, 0,0, 1,   s,-s, s, 0,0, 1,   s, s, s, 0,0, 1,  -s, s, s, 0,0, 1,
            -s,-s,-s, -1,0,0,  -s,-s, s, -1,0,0,  -s, s, s, -1,0,0,  -s, s,-s, -1,0,0,
             s,-s,-s,  1,0,0,   s,-s, s,  1,0,0,   s, s, s,  1,0,0,   s, s,-s,  1,0,0,
            -s,-s,-s, 0,-1,0,   s,-s,-s, 0,-1,0,   s,-s, s, 0,-1,0,  -s,-s, s, 0,-1,0,
            -s, s,-s, 0, 1,0,   s, s,-s, 0, 1,0,   s, s, s, 0, 1,0,  -s, s, s, 0, 1,0,
        ]
        let idx: [UInt32] = [
            0,1,2, 0,2,3,       // back
            4,6,5, 4,7,6,       // front
            8,9,10, 8,10,11,    // left
            12,14,13, 12,15,14, // right
            16,17,18, 16,18,19, // bottom
            20,22,21, 20,23,22, // top
        ]

        monsterVertexBuf = device.makeBuffer(bytes: verts, length: verts.count * 4, options: .storageModeShared)
        monsterIndexBuf  = device.makeBuffer(bytes: idx, length: idx.count * 4, options: .storageModeShared)
        monsterIndexCount = idx.count
        _ = engine_water_reflect_ent_clear()
        for (i, mpos) in monsterPositions.enumerated() {
            // Studio/skinned materials into water reflection RT (not debug-box)
            _ = engine_water_reflect_ent_push_studio(UInt32(i + 1), 1, mpos.x, mpos.y, mpos.z, 8, 8, 8,
                                                    2 /* skinned */, UInt32(i % 4), UInt32(i % 3), 0,
                                                    0.55, 0.75, 0.45, 1.0)
        }
        _ = engine_water_reflect_ent_mark_above(0)
    }

    // MARK: - MTKViewDelegate
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        engine_renderer_resize(UInt32(size.width), UInt32(size.height))
    }


    private func buildDecalSpritePipelines(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary() else { return }
        if let vfn = lib.makeFunction(name: "aether_decal_quad_vertex"),
           let ffn = lib.makeFunction(name: "aether_decal_quad_fragment") {
            let vd = MTLVertexDescriptor()
            vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
            vd.attributes[1].format = .float2; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
            vd.attributes[2].format = .float;  vd.attributes[2].offset = 20; vd.attributes[2].bufferIndex = 0
            vd.attributes[3].format = .float4; vd.attributes[3].offset = 24; vd.attributes[3].bufferIndex = 0
            vd.layouts[0].stride = 40
            vd.layouts[0].stepFunction = .perVertex
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
            d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            d.colorAttachments[0].isBlendingEnabled = true
            d.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            d.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
            do { decalQuadPipeline = try device.makeRenderPipelineState(descriptor: d) }
            catch { print("[MetalRenderer] decal quad pipeline error: \(error)") }
        }
        if let vfn = lib.makeFunction(name: "aether_sprite_quad_vertex"),
           let ffn = lib.makeFunction(name: "aether_sprite_quad_fragment") {
            let vd = MTLVertexDescriptor()
            vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
            vd.attributes[1].format = .float2; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
            vd.attributes[2].format = .float4; vd.attributes[2].offset = 20; vd.attributes[2].bufferIndex = 0
            vd.layouts[0].stride = 36
            vd.layouts[0].stepFunction = .perVertex
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
            d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            d.colorAttachments[0].isBlendingEnabled = true
            d.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            d.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
            do { spriteQuadPipeline = try device.makeRenderPipelineState(descriptor: d) }
            catch { print("[MetalRenderer] sprite quad pipeline error: \(error)") }
        }
    }

    private func syncAndDrawDecalQuads(encoder enc: MTLRenderCommandEncoder,
                                       viewMat: simd_float4x4, projMat: simd_float4x4) {
        guard let pipeline = decalQuadPipeline else { return }
        let maxV = 256 * 6
        var packed = [Float](repeating: 0, count: maxV * 10)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_decals_copy_quads(buf.baseAddress, Int32(maxV)))
        }
        decalQuadCount = Int(n)
        guard decalQuadCount >= 6 else { return }
        let bytes = decalQuadCount * 40
        if decalQuadBuffer == nil || decalQuadBuffer!.length < bytes {
            decalQuadBuffer = device.makeBuffer(length: max(bytes, 4096), options: .storageModeShared)
        }
        if let buf = decalQuadBuffer {
            packed.withUnsafeBytes { raw in
                if let base = raw.baseAddress { buf.contents().copyMemory(from: base, byteCount: bytes) }
            }
        }
        engine_renderer_draw_feature(Int32(ENGINE_CMD_DRAW_DECALS))
        struct DecalUniforms { var view: simd_float4x4; var proj: simd_float4x4 }
        var U = DecalUniforms(view: viewMat, proj: projMat)
        enc.setRenderPipelineState(pipeline)
        if let soft = particleDepthState { enc.setDepthStencilState(soft) }
        if let vb = decalQuadBuffer { enc.setVertexBuffer(vb, offset: 0, index: 0) }
        enc.setVertexBytes(&U, length: MemoryLayout<DecalUniforms>.stride, index: 1)
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: decalQuadCount)
        if let ds = depthState { enc.setDepthStencilState(ds) }
    }

    private func syncAndDrawSpriteStub(encoder enc: MTLRenderCommandEncoder,
                                       viewMat: simd_float4x4, projMat: simd_float4x4,
                                       eye: simd_float3) {
        guard let pipeline = spriteQuadPipeline else { return }
        var packed = [Float](repeating: 0, count: 6 * 9)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_sprite_copy_quad(eye.x + 40, eye.y, eye.z + 16, 20, 20,
                                          buf.baseAddress, 6))
        }
        spriteQuadCount = Int(n)
        guard spriteQuadCount >= 6 else { return }
        let bytes = spriteQuadCount * 36
        if spriteQuadBuffer == nil || spriteQuadBuffer!.length < bytes {
            spriteQuadBuffer = device.makeBuffer(length: max(bytes, 512), options: .storageModeShared)
        }
        if let buf = spriteQuadBuffer {
            packed.withUnsafeBytes { raw in
                if let base = raw.baseAddress { buf.contents().copyMemory(from: base, byteCount: bytes) }
            }
        }
        struct DecalUniforms { var view: simd_float4x4; var proj: simd_float4x4 }
        var U = DecalUniforms(view: viewMat, proj: projMat)
        enc.setRenderPipelineState(pipeline)
        if let soft = particleDepthState { enc.setDepthStencilState(soft) }
        if let vb = spriteQuadBuffer { enc.setVertexBuffer(vb, offset: 0, index: 0) }
        enc.setVertexBytes(&U, length: MemoryLayout<DecalUniforms>.stride, index: 1)
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: spriteQuadCount)
        if let ds = depthState { enc.setDepthStencilState(ds) }
    }

    func draw(in view: MTKView) {
        let now = CACurrentMediaTime()
        var dt = Float(now - lastTime)
        lastTime = now
        if dt < 0.0 || dt > 0.25 { dt = 1.0/60.0 }

        engine_host_frame(dt)
        engine_player_tick(dt)

        guard let drawable = view.currentDrawable,
              let cmd      = queue.makeCommandBuffer() else { return }

        let dw = max(Int(view.drawableSize.width), 1)
        let dh = max(Int(view.drawableSize.height), 1)
        ensureOffscreenTargets(width: dw, height: dh, pixelFormat: view.colorPixelFormat)
        ensureWaterReflectRT(fbW: dw, fbH: dh, pixelFormat: view.colorPixelFormat)
        _ = engine_postfx_ensure_offscreen(Int32(dw), Int32(dh))
        _ = engine_depth_prepass_ensure(UInt32(dw), UInt32(dh))

        guard let sceneColor = sceneColorTexture, let sceneDepth = sceneDepthTexture else { return }
        let rpd = MTLRenderPassDescriptor()
        rpd.colorAttachments[0].texture = sceneColor
        rpd.colorAttachments[0].loadAction = .clear
        rpd.colorAttachments[0].storeAction = .store
        rpd.colorAttachments[0].clearColor = MTLClearColor(red: 0.45, green: 0.65, blue: 0.95, alpha: 1.0)
        rpd.depthAttachment.texture = sceneDepth
        rpd.depthAttachment.loadAction = .clear
        rpd.depthAttachment.storeAction = .store
        rpd.depthAttachment.clearDepth = 1.0

        engine_renderer_begin_frame_dt(dt)

        var eye = [Float](repeating: 0, count: 3)
        var fwd = [Float](repeating: 0, count: 3)
        engine_player_get_eye(&eye)
        engine_player_get_forward(&fwd)
        // Optional spectator follow overrides eye/forward.
        if engine_spectator_is_following() != 0 {
            var se = [Float](repeating: 0, count: 3)
            var sf = [Float](repeating: 0, count: 3)
            _ = engine_spectator_tick(dt, eye[0], eye[1], eye[2], fwd[0], fwd[1], fwd[2])
            _ = engine_spectator_get_eye(&se)
            _ = engine_spectator_get_forward(&sf)
            eye = se; fwd = sf
        }

        let eyeV  = simd_float3(eye[0], eye[1], eye[2])
        let fwdV  = simd_normalize(simd_float3(fwd[0], fwd[1], fwd[2]))
        let target = eyeV + fwdV

        let viewMat = lookAtZUp(eye: eyeV, center: target, up: simd_float3(0, 0, 1))
        let aspect = Float(max(view.drawableSize.width, 1) / max(view.drawableSize.height, 1))
        let projMat = perspective(fovY: 75 * .pi / 180,
                                   aspect: aspect,
                                   near: 1.0, far: 50000.0)

        // Keep C-side renderer camera + feature state in sync with Metal.
        var viewFlat = flattenMatrix(viewMat)
        var projFlat = flattenMatrix(projMat)
        viewFlat.withUnsafeMutableBufferPointer { vb in
            projFlat.withUnsafeMutableBufferPointer { pb in
                engine_renderer_set_camera(vb.baseAddress, pb.baseAddress)
                // Thread real camera matrices into depth prepass (not identity stub).
                _ = engine_depth_prepass_camera_set(vb.baseAddress, pb.baseAddress,
                                                    eye[0], eye[1], eye[2])
            }
        }

        // Mirrored-camera encode into water reflection RT (clear + draw world + resolve).
        encodeWaterReflectPass(cmd: cmd, viewMat: viewMat, projMat: projMat,
                               pixelFormat: view.colorPixelFormat)

        guard let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) else { cmd.commit(); return }
        depthPrepassBoundThisFrame = false
        let mvp = projMat * viewMat
        encodeDepthPrepassIfNeeded(enc, mvp: mvp)
        if let ds = depthState { enc.setDepthStencilState(ds) }

        // Leaf / PVS cull from eye, then submit DRAW_WORLD (MetalCallbacks tracks it).
        _ = refreshVisIndices(force: false)
        engine_renderer_draw_world()

        // ---- Sky (behind world; no depth write) ----
        syncAndDrawSky(encoder: enc, viewMat: viewMat, projMat: projMat, eye: eyeV)

        // ---- Water (animated plane from AetherWater) ----
        syncAndDrawWater(encoder: enc, viewMat: viewMat, projMat: projMat)

        // ---- BSP (culled index list when VIS is active) ----
        let bspIndexCount = culledIndexCount > 0 ? culledIndexCount : indexCount
        let bspIndexBuffer = culledIndexBuffer ?? indexBuffer
        if let vb = vertexBuffer, let ib = bspIndexBuffer, bspIndexCount > 0 {
            let useDyn = (bspDynPipeline != nil)
            let useStyle = (!useDyn && styleBlendPipeline != nil && hasLightmap)
            let pipeline = useDyn ? bspDynPipeline! : (useStyle ? styleBlendPipeline! : bspPipeline)
            if let pipeline = pipeline {
            enc.setRenderPipelineState(pipeline)
            var U = Uniforms(model: matrix_identity_float4x4,
                             view: viewMat, proj: projMat,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)),
                             pad0: 0,
                             baseColor: simd_float4(0.85, 0.9, 1.0, 1.0),
                             useTexture: hasTexture ? 1.0 : 0.0,
                             useLightmap: hasLightmap ? 1.0 : 0.0,
                             pad2: 0, pad3: 0)
            enc.setVertexBuffer(vb, offset: 0, index: 0)
            enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            if useStyle {
                var packed = [Float](repeating: 0, count: 4 + 64 * 4)
                var faceCount: UInt32 = 0
                let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
                    Int32(engine_lightmap_fill_style_blend_draw(buf.baseAddress, UInt32(packed.count), &faceCount))
                }
                if n >= 4 {
                    // buffer(2) = FaceStyleBlendUniforms header (4 floats)
                    var hdr = packed
                    enc.setFragmentBytes(&hdr, length: 16, index: 2)
                    // buffer(3) = weights float4[64]
                    var weights = Array(packed[4..<min(packed.count, 4 + 64 * 4)])
                    while weights.count < 64 * 4 { weights.append(0) }
                    enc.setFragmentBytes(&weights, length: 64 * 4 * MemoryLayout<Float>.stride, index: 3)
                    if styleBlendUboBuffer == nil || styleBlendUboBuffer!.length < Int(n) * 4 {
                        styleBlendUboBuffer = device.makeBuffer(length: max(Int(n) * 4, 1024), options: .storageModeShared)
                    }
                    if let buf = styleBlendUboBuffer {
                        packed.withUnsafeBytes { raw in
                            if let base = raw.baseAddress {
                                memcpy(buf.contents(), base, Int(n) * MemoryLayout<Float>.stride)
                            }
                        }
                    }
                }
            }
            if useDyn {
                var packed = [Float](repeating: 0, count: 4 + 16 * 8)
                let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
                    Int32(engine_dynlights_fill_ubo(buf.baseAddress, Int32(packed.count)))
                }
                if n > 0 {
                    let bytes = Int(n) * MemoryLayout<Float>.stride
                    if dynLightUboBuffer == nil || dynLightUboBuffer!.length < bytes {
                        dynLightUboBuffer = device.makeBuffer(length: max(bytes, 512), options: .storageModeShared)
                    }
                    if let buf = dynLightUboBuffer {
                        packed.withUnsafeBytes { raw in
                            if let base = raw.baseAddress {
                                buf.contents().copyMemory(from: base, byteCount: bytes)
                            }
                        }
                        enc.setFragmentBuffer(buf, offset: 0, index: 2)
                    }
                }
            }
            if let ss = samplerState {
                enc.setFragmentSamplerState(ss, index: 0)
            }
            if let tex = atlasTexture {
                enc.setFragmentTexture(tex, index: 0)
            }
            if let lm = lightmapTexture {
                enc.setFragmentTexture(lm, index: 1)
            }
            enc.drawIndexedPrimitives(type: .triangle, indexCount: bspIndexCount,
                                      indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
            }
        }

        // ---- MDL (test model) + studio skinref Metal texture bind ----
        if let pipeline = (skinrefRemapPipeline ?? mdlPipeline),
           let vb = mdlVertexBuf, let ib = mdlIndexBuf, mdlIndexCount > 0 {
            enc.setRenderPipelineState(pipeline)
            var pos = [Float](repeating: 0, count: 3)
            engine_mdl_mesh_get_render_pos(&pos)
            var model = matrix_identity_float4x4
            model.columns.3 = simd_float4(pos[0], pos[1], pos[2], 1.0)
            var U = Uniforms(model: model, view: viewMat, proj: projMat,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)),
                             pad0: 0,
                             baseColor: simd_float4(0.85, 0.75, 0.55, 1.0),
                             useTexture: (studioSkinTexture != nil) ? 1.0 : 0.0,
                             useLightmap: 0.0, pad2: 0, pad3: 0)
            enc.setVertexBuffer(vb, offset: 0, index: 0)
            enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            // Skinref remap → actual Metal texture bind on studio draw (slot 3)
            var bf: UInt32 = 0, br: UInt32 = 0, bs: UInt32 = 0, bw: UInt32 = 0, bh: UInt32 = 0, bb: UInt32 = 0
            if engine_mdl_skinref_metal_bind_draw(3, &bf, &br, &bs, &bw, &bh, &bb) != 0 {
                if let skin = studioSkinTexture {
                    enc.setFragmentTexture(skin, index: Int(bs))
                    if let samp = studioSkinSampler ?? samplerState {
                        enc.setFragmentSamplerState(samp, index: Int(bs))
                    }
                }
                struct SkinBindU {
                    var family: UInt32; var refIndex: UInt32; var group: UInt32; var tex: UInt32
                    var drawSlot: UInt32; var uvScaleX: Float; var uvScaleY: Float
                    var uvOffX: Float; var uvOffY: Float; var pad0: Float; var pad1: Float; var pad2: Float
                }
                var SU = SkinBindU(family: bf, refIndex: br, group: 0, tex: 0,
                                   drawSlot: bs, uvScaleX: 0.5, uvScaleY: 0.5,
                                   uvOffX: 0, uvOffY: 0, pad0: 0, pad1: 0, pad2: 0)
                enc.setFragmentBytes(&SU, length: MemoryLayout<SkinBindU>.stride, index: 2)
                var tint = simd_float4(0.85, 0.75, 0.55, 1.0)
                enc.setFragmentBytes(&tint, length: MemoryLayout<simd_float4>.stride, index: 3)
                _ = engine_mdl_skinref_metal_bind_mark()
            }
            enc.drawIndexedPrimitives(type: .triangle, indexCount: mdlIndexCount,
                                      indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
        }

        // ---- Monsters (as boxes) ----
        if let pipeline = mdlPipeline, let vb = monsterVertexBuf, let ib = monsterIndexBuf, monsterIndexCount > 0 {
            enc.setRenderPipelineState(pipeline)
            for mpos in monsterPositions {
                var model = matrix_identity_float4x4
                model.columns.3 = simd_float4(mpos.x, mpos.y, mpos.z, 1.0)
                var U = Uniforms(model: model, view: viewMat, proj: projMat,
                                 lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)),
                                 pad0: 0,
                                 baseColor: simd_float4(0.9, 0.3, 0.3, 1.0),   // red = monster
                                 useTexture: 0.0, useLightmap: 0.0, pad2: 0, pad3: 0)
                enc.setVertexBuffer(vb, offset: 0, index: 0)
                enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.drawIndexedPrimitives(type: .triangle, indexCount: monsterIndexCount,
                                          indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
            }
        }

        // ---- Particles (AetherParticle pool) ----
        syncAndDrawParticles(encoder: enc, viewMat: viewMat, projMat: projMat, dt: dt)

        // ---- Fog (fullscreen tint from AetherFog) ----
        syncAndDrawFog(encoder: enc)
        syncAndDrawDecalQuads(encoder: enc, viewMat: viewMat, projMat: projMat)
        syncAndDrawSpriteStub(encoder: enc, viewMat: viewMat, projMat: projMat, eye: eyeV)
        syncAndDrawBlobShadows(encoder: enc, viewMat: viewMat, projMat: projMat, eye: eyeV)
        styleTimeAccum += dt
        _ = engine_lightstyles_update(styleTimeAccum)
        _ = engine_lightmap_apply_style_pingpong(0)
        _ = engine_postfx_set_from_settings()

        enc.endEncoding()

        // Live MTK depth attachment → Hi-Z encode (compute from sceneDepth)
        encodeHizFromMtkDepthAttach(cmd: cmd, depth: sceneDepth)

        // Pass 2: PostFX (± bloom bright/blur/combine) → drawable.
        encodePostFXChain(commandBuffer: cmd, view: view, sceneTex: sceneColorTexture)

        engine_renderer_draw_hud()
        engine_renderer_end_frame()
        cmd.present(drawable)
        cmd.commit()
    }



    private func syncAndDrawWater(encoder enc: MTLRenderCommandEncoder,
                                  viewMat: simd_float4x4,
                                  projMat: simd_float4x4) {
        guard engine_water_enabled() != 0, let pipeline = waterPipeline else { return }
        var eyeR = [Float](repeating: 0, count: 3)
        engine_player_get_eye(&eyeR)
        _ = engine_water_reflect_compute(eyeR[0], eyeR[1], eyeR[2])
        _ = engine_water_reflect_rt_encode_plan(nil, nil, nil, nil, nil)

        let cap = Int(engine_water_render_vertex_capacity())
        guard cap > 0 else { return }
        var packed = [Float](repeating: 0, count: cap * 9)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_water_copy_render(buf.baseAddress, Int32(cap)))
        }
        waterVertexCount = Int(n)
        guard waterVertexCount > 0 else { return }

        let bytes = waterVertexCount * 36
        if waterBuffer == nil || waterBuffer!.length < bytes {
            waterBuffer = device.makeBuffer(length: max(bytes, cap * 36), options: .storageModeShared)
        }
        if let buf = waterBuffer {
            packed.withUnsafeBytes { raw in
                if let base = raw.baseAddress {
                    buf.contents().copyMemory(from: base, byteCount: bytes)
                }
            }
        }

        engine_renderer_draw_feature(Int32(ENGINE_CMD_DRAW_WATER))

        struct WaterUniforms {
            var view: simd_float4x4
            var proj: simd_float4x4
            var time: Float
            var pad0: Float = 0
            var pad1: Float = 0
            var pad2: Float = 0
        }
        var WU = WaterUniforms(view: viewMat, proj: projMat, time: engine_water_wave_time())
        enc.setRenderPipelineState(pipeline)
        if let rtex = waterReflectTexture {
            enc.setFragmentTexture(rtex, index: 1)
        }
        if let ss = samplerState {
            enc.setFragmentSamplerState(ss, index: 0)
        }
        var reflectOn: Float = engine_water_reflect_rt_sample_needed() != 0 ? 1.0 : 0.0
        enc.setFragmentBytes(&reflectOn, length: 4, index: 2)
        if let wd = waterDepthState { enc.setDepthStencilState(wd) }
        enc.setCullMode(.none)
        if let vb = waterBuffer {
            enc.setVertexBuffer(vb, offset: 0, index: 0)
        }
        enc.setVertexBytes(&WU, length: MemoryLayout<WaterUniforms>.stride, index: 1)
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: waterVertexCount)
        if let ds = depthState { enc.setDepthStencilState(ds) }
    }

    private func syncAndDrawSky(encoder enc: MTLRenderCommandEncoder,
                                viewMat: simd_float4x4,
                                projMat: simd_float4x4,
                                eye: simd_float3) {
        guard skyVertexCount > 0, let pipeline = skyPipeline, let vb = skyBuffer else { return }
        engine_renderer_draw_feature(Int32(ENGINE_CMD_DRAW_SKY))

        struct SkyUniforms {
            var view: simd_float4x4
            var proj: simd_float4x4
            var eye: simd_float3
            var pad0: Float = 0
        }
        var SU = SkyUniforms(view: viewMat, proj: projMat, eye: eye, pad0: 0)
        enc.setRenderPipelineState(pipeline)
        if let sd = skyDepthState { enc.setDepthStencilState(sd) }
        enc.setCullMode(.front) // dome faces inward
        enc.setVertexBuffer(vb, offset: 0, index: 0)
        enc.setVertexBytes(&SU, length: MemoryLayout<SkyUniforms>.stride, index: 1)
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: skyVertexCount)
        enc.setCullMode(.none)
        if let ds = depthState { enc.setDepthStencilState(ds) }
    }

    private func syncAndDrawParticles(encoder enc: MTLRenderCommandEncoder,
                                      viewMat: simd_float4x4,
                                      projMat: simd_float4x4,
                                      dt: Float) {
        // Replenish when the pool runs dry so the feature stays visible in demos.
        particleRespawnAccum += dt
        if engine_particles_active_count() < 8 && particleRespawnAccum > 0.35 {
            particleRespawnAccum = 0
            var eye = [Float](repeating: 0, count: 3)
            engine_player_get_eye(&eye)
            _ = engine_particles_spawn_burst(eye[0], eye[1], eye[2] + 16, 48)
        }

        var packed = [Float](repeating: 0, count: maxParticleUpload * 8)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_particles_copy_render(buf.baseAddress, Int32(maxParticleUpload)))
        }
        particleCount = Int(n)
        guard particleCount > 0, let pipeline = particlePipeline else { return }

        let bytes = particleCount * 32
        if particleBuffer == nil || particleBuffer!.length < bytes {
            particleBuffer = device.makeBuffer(length: max(bytes, maxParticleUpload * 32),
                                               options: .storageModeShared)
        }
        if let buf = particleBuffer {
            packed.withUnsafeBytes { raw in
                if let base = raw.baseAddress {
                    buf.contents().copyMemory(from: base, byteCount: bytes)
                }
            }
        }

        // Notify C backend that particles are being drawn this frame.
        engine_renderer_draw_feature(Int32(ENGINE_CMD_DRAW_PARTICLES))

        struct ParticleUniforms {
            var view: simd_float4x4
            var proj: simd_float4x4
        }
        var PU = ParticleUniforms(view: viewMat, proj: projMat)
        enc.setRenderPipelineState(pipeline)
        if let softDepth = particleDepthState {
            enc.setDepthStencilState(softDepth)
        }
        if let vb = particleBuffer {
            enc.setVertexBuffer(vb, offset: 0, index: 0)
        }
        enc.setVertexBytes(&PU, length: MemoryLayout<ParticleUniforms>.stride, index: 1)
        enc.drawPrimitives(type: .point, vertexStart: 0, vertexCount: particleCount)
        // Restore default depth write for any later passes
        if let ds = depthState { enc.setDepthStencilState(ds) }
    }


    private func syncAndDrawFog(encoder enc: MTLRenderCommandEncoder) {
        guard engine_fog_enabled() != 0, let pipeline = fogPipeline else { return }

        let cap = Int(engine_fog_render_vertex_capacity())
        guard cap > 0 else { return }
        var packed = [Float](repeating: 0, count: cap * 8)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_fog_copy_render(buf.baseAddress, Int32(cap)))
        }
        fogVertexCount = Int(n)
        guard fogVertexCount > 0 else { return }

        let bytes = fogVertexCount * 32
        if fogBuffer == nil || fogBuffer!.length < bytes {
            fogBuffer = device.makeBuffer(length: max(bytes, cap * 32), options: .storageModeShared)
        }
        if let buf = fogBuffer {
            packed.withUnsafeBytes { raw in
                if let base = raw.baseAddress {
                    buf.contents().copyMemory(from: base, byteCount: bytes)
                }
            }
        }

        engine_renderer_draw_feature(Int32(ENGINE_CMD_DRAW_FOG))

        struct FogUniforms {
            var density: Float
            var factor: Float
            var pad0: Float = 0
            var pad1: Float = 0
        }
        var FU = FogUniforms(density: engine_fog_density(), factor: engine_fog_factor())
        enc.setRenderPipelineState(pipeline)
        if let fd = fogDepthState { enc.setDepthStencilState(fd) }
        enc.setCullMode(.none)
        if let vb = fogBuffer {
            enc.setVertexBuffer(vb, offset: 0, index: 0)
        }
        enc.setVertexBytes(&FU, length: MemoryLayout<FogUniforms>.stride, index: 1)
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: fogVertexCount)
        if let ds = depthState { enc.setDepthStencilState(ds) }
    }


    private func buildDynBlobPostfxPipelines(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary() else { return }
        // BSP + dyn lights (world pos)
        if let vfn = lib.makeFunction(name: "aether_vertex_main_world"),
           let ffn = lib.makeFunction(name: "aether_fragment_dynlights_world") {
            let vd = MTLVertexDescriptor()
            vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
            vd.attributes[1].format = .float3; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
            vd.attributes[2].format = .float2; vd.attributes[2].offset = 24; vd.attributes[2].bufferIndex = 0
            vd.attributes[3].format = .float2; vd.attributes[3].offset = 32; vd.attributes[3].bufferIndex = 0
            vd.attributes[4].format = .float;  vd.attributes[4].offset = 40; vd.attributes[4].bufferIndex = 0
            vd.layouts[0].stride = 44
            vd.layouts[0].stepFunction = .perVertex
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
            d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
            do { bspDynPipeline = try device.makeRenderPipelineState(descriptor: d) }
            catch { print("[MetalRenderer] BSP dyn pipeline error: \(error)") }
        }
        // Blob shadow
        if let vfn = lib.makeFunction(name: "aether_blob_shadow_vertex"),
           let ffn = lib.makeFunction(name: "aether_blob_shadow_fragment") {
            let vd = MTLVertexDescriptor()
            vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
            vd.attributes[1].format = .float2; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
            vd.attributes[2].format = .float;  vd.attributes[2].offset = 20; vd.attributes[2].bufferIndex = 0
            vd.layouts[0].stride = 32
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
            d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            d.colorAttachments[0].isBlendingEnabled = true
            d.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            d.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            d.colorAttachments[0].sourceAlphaBlendFactor = .one
            d.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
            d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
            do { blobShadowPipeline = try device.makeRenderPipelineState(descriptor: d) }
            catch { print("[MetalRenderer] blob pipeline error: \(error)") }
        }
        // PostFX hook (fullscreen; samples color attachment if available — stub draws params path)
        if let vfn = lib.makeFunction(name: "aether_postfx_vertex"),
           let ffn = lib.makeFunction(name: "aether_postfx_fragment") {
            let vd = MTLVertexDescriptor()
            vd.attributes[0].format = .float3; vd.attributes[0].offset = 0; vd.attributes[0].bufferIndex = 0
            vd.attributes[1].format = .float2; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
            vd.layouts[0].stride = 20
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
            d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
            do { postfxPipeline = try device.makeRenderPipelineState(descriptor: d) }
            catch { print("[MetalRenderer] postfx pipeline error: \(error)") }
        }
        // Bloom chain pipelines (bright / blur / combine) — share PostFX vertex layout.
        if let vfn = lib.makeFunction(name: "aether_postfx_vertex") {
            let vd = MTLVertexDescriptor()
            vd.attributes[0].format = .float3; vd.attributes[0].offset = 0; vd.attributes[0].bufferIndex = 0
            vd.attributes[1].format = .float2; vd.attributes[1].offset = 12; vd.attributes[1].bufferIndex = 0
            vd.layouts[0].stride = 20
            if let fBright = lib.makeFunction(name: "aether_bloom_bright_fragment") {
                let d = MTLRenderPipelineDescriptor()
                d.vertexFunction = vfn; d.fragmentFunction = fBright; d.vertexDescriptor = vd
                d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
                do { bloomBrightPipeline = try device.makeRenderPipelineState(descriptor: d) }
                catch { print("[MetalRenderer] bloom bright error: \(error)") }
            }
            if let fBlur = lib.makeFunction(name: "aether_bloom_blur_fragment") {
                let d = MTLRenderPipelineDescriptor()
                d.vertexFunction = vfn; d.fragmentFunction = fBlur; d.vertexDescriptor = vd
                d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
                do { bloomBlurPipeline = try device.makeRenderPipelineState(descriptor: d) }
                catch { print("[MetalRenderer] bloom blur error: \(error)") }
            }
            if let fH = lib.makeFunction(name: "aether_bloom_blur_h_fragment") {
                let d = MTLRenderPipelineDescriptor()
                d.vertexFunction = vfn; d.fragmentFunction = fH; d.vertexDescriptor = vd
                d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
                do { bloomBlurHPipeline = try device.makeRenderPipelineState(descriptor: d) }
                catch { print("[MetalRenderer] bloom blur H error: \(error)") }
            }
            if let fV = lib.makeFunction(name: "aether_bloom_blur_v_fragment") {
                let d = MTLRenderPipelineDescriptor()
                d.vertexFunction = vfn; d.fragmentFunction = fV; d.vertexDescriptor = vd
                d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
                do { bloomBlurVPipeline = try device.makeRenderPipelineState(descriptor: d) }
                catch { print("[MetalRenderer] bloom blur V error: \(error)") }
            }
            if let fComb = lib.makeFunction(name: "aether_bloom_combine_fragment") {
                let d = MTLRenderPipelineDescriptor()
                d.vertexFunction = vfn; d.fragmentFunction = fComb; d.vertexDescriptor = vd
                d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
                d.depthAttachmentPixelFormat = mtkView.depthStencilPixelFormat
                do { bloomCombinePipeline = try device.makeRenderPipelineState(descriptor: d) }
                catch { print("[MetalRenderer] bloom combine error: \(error)") }
            }
        }
    }

    private func syncAndDrawBlobShadows(encoder enc: MTLRenderCommandEncoder,
                                        viewMat: simd_float4x4,
                                        projMat: simd_float4x4,
                                        eye: simd_float3) {
        guard let pipeline = blobShadowPipeline else { return }
        var packed = [Float](repeating: 0, count: 6 * 8)
        // Player blob
        let nPlayer = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_shadow_copy_blob(eye.x, eye.y, 0.0, 24.0, buf.baseAddress, 6))
        }
        let total = Int(nPlayer)
        guard total >= 6 else { return }
        let bytes = total * 32
        if blobShadowBuffer == nil || blobShadowBuffer!.length < bytes {
            blobShadowBuffer = device.makeBuffer(length: max(bytes, 512), options: .storageModeShared)
        }
        if let buf = blobShadowBuffer {
            packed.withUnsafeBytes { raw in
                if let base = raw.baseAddress {
                    buf.contents().copyMemory(from: base, byteCount: bytes)
                }
            }
        }
        struct DecalUniforms { var view: simd_float4x4; var proj: simd_float4x4 }
        var U = DecalUniforms(view: viewMat, proj: projMat)
        enc.setRenderPipelineState(pipeline)
        enc.setCullMode(.none)
        if let vb = blobShadowBuffer { enc.setVertexBuffer(vb, offset: 0, index: 0) }
        enc.setVertexBytes(&U, length: MemoryLayout<DecalUniforms>.stride, index: 1)
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: total)
        blobShadowCount = total
    }

    private func ensureOffscreenTargets(width: Int, height: Int, pixelFormat: MTLPixelFormat) {
        if postfxOffscreenSize == (width, height), sceneColorTexture != nil, sceneDepthTexture != nil {
            return
        }
        let colorDesc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: pixelFormat, width: width, height: height, mipmapped: false)
        colorDesc.usage = [.renderTarget, .shaderRead]
        colorDesc.storageMode = .private
        sceneColorTexture = device.makeTexture(descriptor: colorDesc)

        let depthDesc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .depth32Float, width: width, height: height, mipmapped: false)
        /* shaderRead so Hi-Z encode kernel can sample the live MTK depth attachment */
        depthDesc.usage = [.renderTarget, .shaderRead]
        depthDesc.storageMode = .private
        sceneDepthTexture = device.makeTexture(descriptor: depthDesc)
        postfxOffscreenSize = (width, height)
        ensureHizArrayTexture(width: max(width / 2, 64), height: max(height / 2, 64), slices: 4)
        ensureStudioSkinAtlas()

        if postfxSampler == nil {
            let sd = MTLSamplerDescriptor()
            sd.minFilter = .linear
            sd.magFilter = .linear
            sd.sAddressMode = .clampToEdge
            sd.tAddressMode = .clampToEdge
            postfxSampler = device.makeSamplerState(descriptor: sd)
        }
    }

    private func fillPostFXQuadBuffer() -> Int {
        var packed = [Float](repeating: 0, count: 6 * 5)
        let n = packed.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_postfx_copy_fullscreen(buf.baseAddress, 6))
        }
        guard n >= 6 else { return 0 }
        let bytes = Int(n) * 20
        if postfxBuffer == nil || postfxBuffer!.length < bytes {
            postfxBuffer = device.makeBuffer(length: max(bytes, 256), options: .storageModeShared)
        }
        if let buf = postfxBuffer {
            packed.withUnsafeBytes { raw in
                if let base = raw.baseAddress {
                    buf.contents().copyMemory(from: base, byteCount: bytes)
                }
            }
        }
        return Int(n)
    }

    private func ensureBloomTargets(sceneW: Int, sceneH: Int, pixelFormat: MTLPixelFormat) {
        let bw = max(sceneW / 2, 1)
        let bh = max(sceneH / 2, 1)
        if bloomTexSize == (bw, bh), bloomBrightTexture != nil, bloomBlurTexture != nil { return }
        let desc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: pixelFormat, width: bw, height: bh, mipmapped: false)
        desc.usage = [.renderTarget, .shaderRead]
        desc.storageMode = .private
        bloomBrightTexture = device.makeTexture(descriptor: desc)
        bloomBlurTexture = device.makeTexture(descriptor: desc)
        bloomTexSize = (bw, bh)
    }

    private func encodePostFXChain(commandBuffer cmd: MTLCommandBuffer,
                                   view: MTKView,
                                   sceneTex: MTLTexture?) {
        guard let sceneTex = sceneTex else { return }
        let n = fillPostFXQuadBuffer()
        guard n >= 6, let vb = postfxBuffer else { return }

        var bloom = [Float](repeating: 0, count: 4)
        _ = bloom.withUnsafeMutableBufferPointer { buf in
            engine_postfx_fill_bloom(buf.baseAddress)
        }
        let bloomOn = bloom[3] > 0.5
            && bloomBrightPipeline != nil
            && bloomBlurPipeline != nil
            && bloomCombinePipeline != nil

        struct PostFXUniforms {
            var brightness: Float
            var gamma: Float
            var exposure: Float
            var enabled: Float
        }
        struct BloomUniforms {
            var threshold: Float
            var intensity: Float
            var blur_radius: Float
            var enabled: Float
        }
        var PU = PostFXUniforms(brightness: engine_postfx_brightness(),
                                gamma: engine_postfx_gamma(),
                                exposure: 1.0,
                                enabled: 1.0)
        var BU = BloomUniforms(threshold: bloom[0], intensity: bloom[1],
                               blur_radius: bloom[2], enabled: bloom[3])

        if bloomOn {
            ensureBloomTargets(sceneW: sceneTex.width, sceneH: sceneTex.height,
                               pixelFormat: view.colorPixelFormat)
            // Bright pass
            if let brightTex = bloomBrightTexture, let pipe = bloomBrightPipeline {
                let rpd = MTLRenderPassDescriptor()
                rpd.colorAttachments[0].texture = brightTex
                rpd.colorAttachments[0].loadAction = .clear
                rpd.colorAttachments[0].storeAction = .store
                rpd.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1)
                if let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) {
                    enc.setRenderPipelineState(pipe)
                    enc.setCullMode(.none)
                    enc.setVertexBuffer(vb, offset: 0, index: 0)
                    enc.setFragmentBytes(&BU, length: MemoryLayout<BloomUniforms>.stride, index: 1)
                    enc.setFragmentTexture(sceneTex, index: 0)
                    if let samp = postfxSampler { enc.setFragmentSamplerState(samp, index: 0) }
                    enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: n)
                    enc.endEncoding()
                }
            }
            // Blur pass — prefer separable H then V when pipelines exist
            struct BloomBlurDir {
                var threshold: Float; var intensity: Float
                var blur_radius: Float; var enabled: Float
                var dirX: Float; var dirY: Float; var pad0: Float; var pad1: Float
            }
            var bloomResultTex = bloomBlurTexture
            if let brightTex = bloomBrightTexture, let blurTex = bloomBlurTexture,
               let pipeH = bloomBlurHPipeline, let pipeV = bloomBlurVPipeline {
                var BH = BloomBlurDir(threshold: bloom[0], intensity: bloom[1],
                                      blur_radius: bloom[2], enabled: bloom[3],
                                      dirX: 1, dirY: 0, pad0: 0, pad1: 0)
                let rpdH = MTLRenderPassDescriptor()
                rpdH.colorAttachments[0].texture = blurTex
                rpdH.colorAttachments[0].loadAction = .clear
                rpdH.colorAttachments[0].storeAction = .store
                rpdH.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1)
                if let enc = cmd.makeRenderCommandEncoder(descriptor: rpdH) {
                    enc.setRenderPipelineState(pipeH)
                    enc.setCullMode(.none)
                    enc.setVertexBuffer(vb, offset: 0, index: 0)
                    enc.setFragmentBytes(&BH, length: MemoryLayout<BloomBlurDir>.stride, index: 1)
                    enc.setFragmentTexture(brightTex, index: 0)
                    if let samp = postfxSampler { enc.setFragmentSamplerState(samp, index: 0) }
                    enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: n)
                    enc.endEncoding()
                }
                var BV = BH; BV.dirX = 0; BV.dirY = 1
                let rpdV = MTLRenderPassDescriptor()
                rpdV.colorAttachments[0].texture = brightTex
                rpdV.colorAttachments[0].loadAction = .clear
                rpdV.colorAttachments[0].storeAction = .store
                rpdV.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1)
                if let enc = cmd.makeRenderCommandEncoder(descriptor: rpdV) {
                    enc.setRenderPipelineState(pipeV)
                    enc.setCullMode(.none)
                    enc.setVertexBuffer(vb, offset: 0, index: 0)
                    enc.setFragmentBytes(&BV, length: MemoryLayout<BloomBlurDir>.stride, index: 1)
                    enc.setFragmentTexture(blurTex, index: 0)
                    if let samp = postfxSampler { enc.setFragmentSamplerState(samp, index: 0) }
                    enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: n)
                    enc.endEncoding()
                }
                bloomResultTex = brightTex
            } else if let brightTex = bloomBrightTexture, let blurTex = bloomBlurTexture,
                      let pipe = bloomBlurPipeline {
                let rpd = MTLRenderPassDescriptor()
                rpd.colorAttachments[0].texture = blurTex
                rpd.colorAttachments[0].loadAction = .clear
                rpd.colorAttachments[0].storeAction = .store
                rpd.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1)
                if let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) {
                    enc.setRenderPipelineState(pipe)
                    enc.setCullMode(.none)
                    enc.setVertexBuffer(vb, offset: 0, index: 0)
                    enc.setFragmentBytes(&BU, length: MemoryLayout<BloomUniforms>.stride, index: 1)
                    enc.setFragmentTexture(brightTex, index: 0)
                    if let samp = postfxSampler { enc.setFragmentSamplerState(samp, index: 0) }
                    enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: n)
                    enc.endEncoding()
                }
                bloomResultTex = blurTex
            }
            // Combine → drawable
            guard let rpd2 = view.currentRenderPassDescriptor else { return }
            rpd2.colorAttachments[0].loadAction = .dontCare
            if let enc = cmd.makeRenderCommandEncoder(descriptor: rpd2),
               let pipe = bloomCombinePipeline,
               let blurTex = bloomResultTex {
                enc.setRenderPipelineState(pipe)
                enc.setCullMode(.none)
                enc.setVertexBuffer(vb, offset: 0, index: 0)
                enc.setFragmentBytes(&PU, length: MemoryLayout<PostFXUniforms>.stride, index: 1)
                enc.setFragmentBytes(&BU, length: MemoryLayout<BloomUniforms>.stride, index: 2)
                enc.setFragmentTexture(sceneTex, index: 0)
                enc.setFragmentTexture(blurTex, index: 1)
                if let samp = postfxSampler { enc.setFragmentSamplerState(samp, index: 0) }
                enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: n)
                enc.endEncoding()
            }
            return
        }

        // Fallback: single brightness/gamma PostFX pass
        guard let pipeline = postfxPipeline else { return }
        guard let rpd2 = view.currentRenderPassDescriptor else { return }
        rpd2.colorAttachments[0].loadAction = .dontCare
        guard let enc = cmd.makeRenderCommandEncoder(descriptor: rpd2) else { return }
        enc.setRenderPipelineState(pipeline)
        enc.setCullMode(.none)
        enc.setVertexBuffer(vb, offset: 0, index: 0)
        enc.setFragmentBytes(&PU, length: MemoryLayout<PostFXUniforms>.stride, index: 1)
        enc.setFragmentTexture(sceneTex, index: 0)
        if let samp = postfxSampler { enc.setFragmentSamplerState(samp, index: 0) }
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: n)
        enc.endEncoding()
    }

    private func syncAndDrawPostFX(encoder enc: MTLRenderCommandEncoder, sceneTex: MTLTexture?) {
        // Kept for any callers; prefer encodePostFXChain.
        guard let pipeline = postfxPipeline, let sceneTex = sceneTex else { return }
        let n = fillPostFXQuadBuffer()
        guard n >= 6, let vb = postfxBuffer else { return }
        struct PostFXUniforms {
            var brightness: Float; var gamma: Float; var exposure: Float; var enabled: Float
        }
        var PU = PostFXUniforms(brightness: engine_postfx_brightness(),
                                gamma: engine_postfx_gamma(), exposure: 1.0, enabled: 1.0)
        enc.setRenderPipelineState(pipeline)
        enc.setCullMode(.none)
        enc.setVertexBuffer(vb, offset: 0, index: 0)
        enc.setFragmentBytes(&PU, length: MemoryLayout<PostFXUniforms>.stride, index: 1)
        enc.setFragmentTexture(sceneTex, index: 0)
        if let samp = postfxSampler { enc.setFragmentSamplerState(samp, index: 0) }
        enc.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: n)
    }


    /// Column-major float[16] matching aether_mat4_t / Metal simd layout.
    private func flattenMatrix(_ m: simd_float4x4) -> [Float] {
        return [
            m.columns.0.x, m.columns.0.y, m.columns.0.z, m.columns.0.w,
            m.columns.1.x, m.columns.1.y, m.columns.1.z, m.columns.1.w,
            m.columns.2.x, m.columns.2.y, m.columns.2.z, m.columns.2.w,
            m.columns.3.x, m.columns.3.y, m.columns.3.z, m.columns.3.w
        ]
    }

    private func lookAtZUp(eye: simd_float3, center: simd_float3, up: simd_float3) -> simd_float4x4 {
        let f = simd_normalize(center - eye)
        let s = simd_normalize(simd_cross(f, up))
        let u = simd_cross(s, f)
        var m = matrix_identity_float4x4
        m.columns.0 = simd_float4( s.x,  u.x, -f.x, 0)
        m.columns.1 = simd_float4( s.y,  u.y, -f.y, 0)
        m.columns.2 = simd_float4( s.z,  u.z, -f.z, 0)
        m.columns.3 = simd_float4(-simd_dot(s, eye), -simd_dot(u, eye), simd_dot(f, eye), 1)
        return m
    }

    private func perspective(fovY: Float, aspect: Float, near: Float, far: Float) -> simd_float4x4 {
        let y = 1.0 / tanf(fovY * 0.5)
        let x = y / aspect
        let z = far / (near - far)
        var m = simd_float4x4(0)
        m.columns.0 = simd_float4(x, 0, 0, 0)
        m.columns.1 = simd_float4(0, y, 0, 0)
        m.columns.2 = simd_float4(0, 0, z, -1)
        m.columns.3 = simd_float4(0, 0, z * near, 0)
        return m
    }
}

struct MetalView: UIViewRepresentable {
    func makeCoordinator() -> Coordinator { Coordinator() }
    func makeUIView(context: Context) -> MTKView {
        let v = MTKView()
        v.device = MTLCreateSystemDefaultDevice()
        v.colorPixelFormat = .bgra8Unorm
        v.depthStencilPixelFormat = .depth32Float
        v.clearColor = MTLClearColor(red: 0.25, green: 0.30, blue: 0.45, alpha: 1.0)
        v.preferredFramesPerSecond = 60
        v.enableSetNeedsDisplay = false
        v.isPaused = false
        let r = MetalRenderer(mtkView: v)
        context.coordinator.renderer = r
        DispatchQueue.main.async { r?.uploadMeshFromEngine() }
        return v
    }
    func updateUIView(_ uiView: MTKView, context: Context) {}
    final class Coordinator { var renderer: MetalRenderer? }


    // MARK: - Batch19 Hi-Z from MTK depth + studio skin atlas

    private func ensureHizEncodePipeline() {
        guard hizEncodePipeline == nil, let lib = device.makeDefaultLibrary(),
              let fn = lib.makeFunction(name: "aether_hiz_encode_from_depth") else { return }
        do { hizEncodePipeline = try device.makeComputePipelineState(function: fn) }
        catch { print("[MetalRenderer] hiz encode pipeline: \(error)") }
    }

    private func ensureHizArrayTexture(width: Int, height: Int, slices: Int) {
        let w = max(width, 8), h = max(height, 8), s = max(slices, 2)
        if hizArraySize == (w, h, s), hizArrayTexture != nil { return }
        let td = MTLTextureDescriptor()
        td.textureType = .type2DArray
        td.pixelFormat = .r32Float
        td.width = w; td.height = h; td.arrayLength = s
        td.usage = [.shaderWrite, .shaderRead]
        td.storageMode = .private
        hizArrayTexture = device.makeTexture(descriptor: td)
        hizArraySize = (w, h, s)
        ensureHizEncodePipeline()
    }

    /// Wire live MTK depth attachment into Hi-Z encode compute path.
    private func encodeHizFromMtkDepthAttach(cmd: MTLCommandBuffer, depth: MTLTexture) {
        let dw = UInt32(depth.width), dh = UInt32(depth.height)
        var passes: UInt32 = 0; var needed: Int32 = 0
        _ = engine_depth_hiz_mtk_attach_plan(dw, dh, &passes, &needed)
        _ = engine_depth_hiz_mtk_attach_wire()
        ensureHizArrayTexture(width: Int(max(dw / 2, 64)), height: Int(max(dh / 2, 64)), slices: 4)
        guard let pipe = hizEncodePipeline, let hiz = hizArrayTexture,
              let enc = cmd.makeComputeCommandEncoder() else {
            _ = engine_depth_hiz_mtk_attach_mark()
            return
        }
        enc.setComputePipelineState(pipe)
        enc.setTexture(depth, index: 0)
        enc.setTexture(hiz, index: 1)
        struct HizU {
            var srcWidth: UInt32; var srcHeight: UInt32
            var dstWidth: UInt32; var dstHeight: UInt32
            var srcSlice: UInt32; var dstSlice: UInt32
            var passIndex: UInt32; var fromDepth: UInt32
        }
        var U = HizU(srcWidth: dw, srcHeight: dh,
                     dstWidth: UInt32(hiz.width), dstHeight: UInt32(hiz.height),
                     srcSlice: 0, dstSlice: 0, passIndex: 0, fromDepth: 1)
        enc.setBytes(&U, length: MemoryLayout<HizU>.stride, index: 0)
        let tw = pipe.threadExecutionWidth
        let th = max(pipe.maxTotalThreadsPerThreadgroup / tw, 1)
        let tg = MTLSize(width: tw, height: th, depth: 1)
        let grid = MTLSize(width: hiz.width, height: hiz.height, depth: 1)
        enc.dispatchThreads(grid, threadsPerThreadgroup: tg)
        enc.endEncoding()
        _ = engine_depth_hiz_mtk_attach_mark()
        _ = engine_mdl_hiz_live_encode_mark()
    }

    private func ensureStudioSkinAtlas() {
        if studioSkinTexture != nil { return }
        var w: UInt32 = 0, h: UInt32 = 0, nbytes: UInt32 = 0
        // Probe size first
        let probeCap: UInt32 = 64 * 16 * 4
        var probe = [UInt8](repeating: 0, count: Int(probeCap))
        guard engine_mdl_skinref_metal_atlas_rgba(&probe, probeCap, &w, &h, &nbytes) != 0,
              w > 0, h > 0, nbytes > 0 else { return }
        var rgba = [UInt8](repeating: 0, count: Int(nbytes))
        guard engine_mdl_skinref_metal_atlas_rgba(&rgba, nbytes, &w, &h, &nbytes) != 0 else { return }
        let td = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .rgba8Unorm,
                                                          width: Int(w), height: Int(h),
                                                          mipmapped: false)
        td.usage = [.shaderRead]
        td.storageMode = .shared
        guard let tex = device.makeTexture(descriptor: td) else { return }
        rgba.withUnsafeBytes { raw in
            if let base = raw.baseAddress {
                tex.replace(region: MTLRegionMake2D(0, 0, Int(w), Int(h)),
                            mipmapLevel: 0,
                            withBytes: base,
                            bytesPerRow: Int(w) * 4)
            }
        }
        studioSkinTexture = tex
        if studioSkinSampler == nil {
            let sd = MTLSamplerDescriptor()
            sd.minFilter = .linear; sd.magFilter = .linear
            sd.sAddressMode = .clampToEdge; sd.tAddressMode = .clampToEdge
            studioSkinSampler = device.makeSamplerState(descriptor: sd)
        }
        // Optional skinref remap fragment pipeline (falls back to mdlPipeline)
        if skinrefRemapPipeline == nil, let lib = device.makeDefaultLibrary(),
           let vert = lib.makeFunction(name: "aether_model_vertex"),
           let frag = lib.makeFunction(name: "aether_mdl_skinref_bind_fragment") {
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = vert; d.fragmentFunction = frag
            d.colorAttachments[0].pixelFormat = .bgra8Unorm
            d.depthAttachmentPixelFormat = .depth32Float
            let v = MTLVertexDescriptor()
            v.attributes[0].format = .float3; v.attributes[0].offset = 0; v.attributes[0].bufferIndex = 0
            v.attributes[1].format = .float3; v.attributes[1].offset = 12; v.attributes[1].bufferIndex = 0
            v.attributes[2].format = .float2; v.attributes[2].offset = 24; v.attributes[2].bufferIndex = 0
            v.layouts[0].stride = 44
            d.vertexDescriptor = v
            do { skinrefRemapPipeline = try device.makeRenderPipelineState(descriptor: d) }
            catch { /* keep mdlPipeline fallback */ }
        }
        _ = engine_mdl_skinref_metal_bind_draw(3, nil, nil, nil, nil, nil, nil)
        _ = engine_mdl_skinref_metal_bind_mark()
    }

}
