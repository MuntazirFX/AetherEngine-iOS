// MetalRenderer.swift
// Renders BSP mesh + MDL model + entities + particles + sky. STEP 18B / metal-sky.
// Pushes view/proj + frame dt into EngineBridge each draw.
// AetherEngine-iOS · Clean-room.

import MetalKit
import SwiftUI
import simd

struct Uniforms {
    var model:      simd_float4x4
    var view:       simd_float4x4
    var proj:       simd_float4x4
    var lightDir:   simd_float3
    var pad0:       Float = 0
    var baseColor:  simd_float4
    var useTexture: Float = 0
    var pad1:       Float = 0
    var pad2:       Float = 0
    var pad3:       Float = 0
}

final class MetalRenderer: NSObject, MTKViewDelegate {
    let device: MTLDevice
    let queue:  MTLCommandQueue

    var bspPipeline:  MTLRenderPipelineState?
    var mdlPipeline:  MTLRenderPipelineState?
    var depthState:   MTLDepthStencilState?
    var samplerState: MTLSamplerState?

    var vertexBuffer: MTLBuffer?
    var indexBuffer:  MTLBuffer?
    var indexCount:   Int = 0

    var atlasTexture: MTLTexture?
    var hasTexture:   Bool = false

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
    private let maxParticleUpload = 512
    private var particleSeedOrigin = simd_float3(0, 0, 64)
    private var particleRespawnAccum: Float = 0

    // Sky dome (driven by AetherSky via EngineBridge)
    var skyPipeline: MTLRenderPipelineState?
    var skyDepthState: MTLDepthStencilState?
    var skyBuffer: MTLBuffer?
    var skyVertexCount: Int = 0

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
        buildDepthState()
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
        vd.layouts[0].stride = 32
        vd.layouts[0].stepFunction = .perVertex
        let d = MTLRenderPipelineDescriptor()
        d.vertexFunction = vfn; d.fragmentFunction = ffn; d.vertexDescriptor = vd
        d.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        d.depthAttachmentPixelFormat      = mtkView.depthStencilPixelFormat
        do { bspPipeline = try device.makeRenderPipelineState(descriptor: d) }
        catch { print("[MetalRenderer] BSP pipeline error: \(error)") }
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
        uploadMdlMesh()
        buildMonsterBoxes()
        uploadSkyDome()

        // Try spawning player at info_player_start; fallback to mesh center
        if engine_player_has_start() {
            engine_player_spawn_at_start()
            print("[MetalRenderer] Player spawned at info_player_start")
        } else {
            engine_player_spawn_at_mesh_center()
            print("[MetalRenderer] Player spawned at mesh center (no info_player_start)")
        }

        // Seed a particle burst near the player eye / mesh center so Metal draws live C state.
        var eye = [Float](repeating: 0, count: 3)
        engine_player_get_eye(&eye)
        particleSeedOrigin = simd_float3(eye[0], eye[1], eye[2] + 24)
        engine_particles_clear()
        let seeded = engine_particles_spawn_burst(eye[0], eye[1], eye[2] + 24, 96)
        print("[MetalRenderer] Upload complete (BSP=\(indexCount > 0), MDL=\(hasMdl), Monsters=\(monsterPositions.count), Particles=\(seeded), Sky=\(skyVertexCount))")
    }

    private func uploadBspMesh() {
        let vCount = Int(engine_bsp_mesh_vertex_count())
        let iCount = Int(engine_bsp_mesh_index_count())
        guard vCount > 0, iCount > 0 else { return }
        var vData = [Float](repeating: 0, count: vCount * 8)
        _ = vData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_vertices(buf.baseAddress, Int32(vCount)))
        }
        vertexBuffer = device.makeBuffer(bytes: vData, length: vCount * 32, options: .storageModeShared)
        var iData = [UInt32](repeating: 0, count: iCount)
        _ = iData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_indices(buf.baseAddress, Int32(iCount)))
        }
        indexBuffer = device.makeBuffer(bytes: iData, length: iCount * 4, options: .storageModeShared)
        indexCount = iCount
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

    private func uploadMdlMesh() {
        let vCount = Int(engine_mdl_mesh_vertex_count())
        let tCount = Int(engine_mdl_mesh_triangle_count())
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
    }

    // MARK: - MTKViewDelegate
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        engine_renderer_resize(UInt32(size.width), UInt32(size.height))
    }

    func draw(in view: MTKView) {
        let now = CACurrentMediaTime()
        var dt = Float(now - lastTime)
        lastTime = now
        if dt < 0.0 || dt > 0.25 { dt = 1.0/60.0 }

        engine_player_tick(dt)

        guard let drawable = view.currentDrawable,
              let rpd      = view.currentRenderPassDescriptor,
              let cmd      = queue.makeCommandBuffer() else { return }

        rpd.colorAttachments[0].clearColor = MTLClearColor(red: 0.45, green: 0.65, blue: 0.95, alpha: 1.0)
        rpd.colorAttachments[0].loadAction = .clear

        engine_renderer_begin_frame_dt(dt)

        guard let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) else { cmd.commit(); return }
        if let ds = depthState { enc.setDepthStencilState(ds) }

        var eye = [Float](repeating: 0, count: 3)
        var fwd = [Float](repeating: 0, count: 3)
        engine_player_get_eye(&eye)
        engine_player_get_forward(&fwd)

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
            }
        }
        engine_renderer_draw_world()

        // ---- Sky (behind world; no depth write) ----
        syncAndDrawSky(encoder: enc, viewMat: viewMat, projMat: projMat, eye: eyeV)

        // ---- BSP ----
        if let pipeline = bspPipeline, let vb = vertexBuffer, let ib = indexBuffer, indexCount > 0 {
            enc.setRenderPipelineState(pipeline)
            var U = Uniforms(model: matrix_identity_float4x4,
                             view: viewMat, proj: projMat,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)),
                             pad0: 0,
                             baseColor: simd_float4(0.85, 0.9, 1.0, 1.0),
                             useTexture: hasTexture ? 1.0 : 0.0,
                             pad1: 0, pad2: 0, pad3: 0)
            enc.setVertexBuffer(vb, offset: 0, index: 0)
            enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            if let tex = atlasTexture, let ss = samplerState {
                enc.setFragmentTexture(tex, index: 0)
                enc.setFragmentSamplerState(ss, index: 0)
            }
            enc.drawIndexedPrimitives(type: .triangle, indexCount: indexCount,
                                      indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
        }

        // ---- MDL (test model) ----
        if let pipeline = mdlPipeline, let vb = mdlVertexBuf, let ib = mdlIndexBuf, mdlIndexCount > 0 {
            enc.setRenderPipelineState(pipeline)
            var pos = [Float](repeating: 0, count: 3)
            engine_mdl_mesh_get_render_pos(&pos)
            var model = matrix_identity_float4x4
            model.columns.3 = simd_float4(pos[0], pos[1], pos[2], 1.0)
            var U = Uniforms(model: model, view: viewMat, proj: projMat,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)),
                             pad0: 0,
                             baseColor: simd_float4(0.85, 0.75, 0.55, 1.0),
                             useTexture: 0.0, pad1: 0, pad2: 0, pad3: 0)
            enc.setVertexBuffer(vb, offset: 0, index: 0)
            enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
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
                                 useTexture: 0.0, pad1: 0, pad2: 0, pad3: 0)
                enc.setVertexBuffer(vb, offset: 0, index: 0)
                enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
                enc.drawIndexedPrimitives(type: .triangle, indexCount: monsterIndexCount,
                                          indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
            }
        }

        // ---- Particles (AetherParticle pool) ----
        syncAndDrawParticles(encoder: enc, viewMat: viewMat, projMat: projMat, dt: dt)

        enc.endEncoding()
        engine_renderer_draw_hud()
        engine_renderer_end_frame()
        cmd.present(drawable)
        cmd.commit()
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
}
