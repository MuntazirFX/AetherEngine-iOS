// MetalRenderer.swift — Uses engine player camera. STEP 13.
// AetherEngine-iOS · Clean-room.

import MetalKit
import SwiftUI
import simd

struct Uniforms {
    var model:     simd_float4x4
    var view:      simd_float4x4
    var proj:      simd_float4x4
    var lightDir:  simd_float3
    var pad0:      Float = 0
    var baseColor: simd_float4
}

final class MetalRenderer: NSObject, MTKViewDelegate {
    let device: MTLDevice
    let queue:  MTLCommandQueue
    var pipelineState: MTLRenderPipelineState?
    var depthState:    MTLDepthStencilState?
    var vertexBuffer: MTLBuffer?
    var indexBuffer:  MTLBuffer?
    var indexCount:   Int = 0

    private var lastTime: CFTimeInterval = CACurrentMediaTime()
    private var lookAccum = CGPoint.zero

    init?(mtkView: MTKView) {
        guard let dev = mtkView.device ?? MTLCreateSystemDefaultDevice(),
              let q = dev.makeCommandQueue() else { return nil }
        self.device = dev; self.queue = q
        super.init()
        mtkView.delegate = self
        mtkView.colorPixelFormat = .bgra8Unorm
        mtkView.depthStencilPixelFormat = .depth32Float
        mtkView.clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
        mtkView.preferredFramesPerSecond = 60
        mtkView.isPaused = false
        mtkView.enableSetNeedsDisplay = false
        buildPipeline(mtkView: mtkView)
        buildDepthState()
        let opaque = Unmanaged.passUnretained(mtkView).toOpaque()
        engine_renderer_attach_metal(opaque)
    }

    private func buildPipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary(),
              let vfn = lib.makeFunction(name: "aether_vertex_main"),
              let ffn = lib.makeFunction(name: "aether_fragment_main") else { return }
        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float3; vd.attributes[0].offset = 0;   vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float3; vd.attributes[1].offset = 12;  vd.attributes[1].bufferIndex = 0
        vd.attributes[2].format = .float2; vd.attributes[2].offset = 24;  vd.attributes[2].bufferIndex = 0
        vd.layouts[0].stride = 32
        vd.layouts[0].stepFunction = .perVertex

        let desc = MTLRenderPipelineDescriptor()
        desc.vertexFunction = vfn; desc.fragmentFunction = ffn; desc.vertexDescriptor = vd
        desc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        desc.depthAttachmentPixelFormat      = mtkView.depthStencilPixelFormat
        do { pipelineState = try device.makeRenderPipelineState(descriptor: desc) }
        catch { print("[MetalRenderer] pipeline error: \(error)") }
    }

    private func buildDepthState() {
        let d = MTLDepthStencilDescriptor()
        d.depthCompareFunction = .less; d.isDepthWriteEnabled = true
        depthState = device.makeDepthStencilState(descriptor: d)
    }

    func uploadMeshFromEngine() {
        let vCount = Int(engine_bsp_mesh_vertex_count())
        let iCount = Int(engine_bsp_mesh_index_count())
        guard vCount > 0, iCount > 0 else { return }
        var vData = [Float](repeating: 0, count: vCount * 8)
        _ = vData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_vertices(buf.baseAddress, Int32(vCount)))
        }
        vertexBuffer = device.makeBuffer(bytes: vData, length: vCount*32, options: .storageModeShared)
        var iData = [UInt32](repeating: 0, count: iCount)
        _ = iData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_indices(buf.baseAddress, Int32(iCount)))
        }
        indexBuffer = device.makeBuffer(bytes: iData, length: iCount*4, options: .storageModeShared)
        indexCount = iCount
        engine_player_spawn_at_mesh_center()
        print("[MetalRenderer] uploaded \(vCount) verts / \(iCount) indices, player spawned")
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        engine_renderer_resize(UInt32(size.width), UInt32(size.height))
    }

    func draw(in view: MTKView) {
        // ---- 1. Frame timing ----
        let now = CACurrentMediaTime()
        var dt = Float(now - lastTime)
        lastTime = now
        if dt < 0.0 || dt > 0.25 { dt = 1.0/60.0 }

        // ---- 2. Advance player simulation ----
        engine_player_tick(dt)

        guard let drawable = view.currentDrawable,
              let rpd      = view.currentRenderPassDescriptor,
              let cmd      = queue.makeCommandBuffer() else { return }
        rpd.colorAttachments[0].clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
        rpd.colorAttachments[0].loadAction = .clear

        engine_renderer_begin_frame()

        guard let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) else { cmd.commit(); return }
        if let ps = pipelineState { enc.setRenderPipelineState(ps) }
        if let ds = depthState    { enc.setDepthStencilState(ds) }

        if let vb = vertexBuffer, let ib = indexBuffer, indexCount > 0 {
            // ---- 3. Camera from engine player ----
            var eye    = [Float](repeating: 0, count: 3)
            var fwd    = [Float](repeating: 0, count: 3)
            engine_player_get_eye(&eye)
            engine_player_get_forward(&fwd)

            let eyeV = simd_float3(eye[0], eye[1], eye[2])
            let fwdV = simd_normalize(simd_float3(fwd[0], fwd[1], fwd[2]))
            let target = eyeV + fwdV

            // GoldSrc is Z-up; Metal view space is Y-up. We swap Y/Z on the way in.
            // Convert: (x, y, z)_GoldSrc → (x, z, y)_view? — We use Z-up throughout:
            // We build a look-at matrix that matches our world Z-up.
            let viewMat = lookAtZUp(eye: eyeV, center: target, up: simd_float3(0, 0, 1))
            let projMat = perspective(fovY: 75 * .pi / 180,
                                       aspect: Float(view.drawableSize.width / view.drawableSize.height),
                                       near: 1.0, far: 50000.0)

            var U = Uniforms(model: matrix_identity_float4x4,
                             view: viewMat, proj: projMat,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)),
                             pad0: 0,
                             baseColor: simd_float4(0.85, 0.9, 1.0, 1.0))

            enc.setVertexBuffer(vb, offset: 0, index: 0)
            enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.drawIndexedPrimitives(type: .triangle, indexCount: indexCount,
                                      indexType: .uint32, indexBuffer: ib, indexBufferOffset: 0)
        }

        enc.endEncoding()
        engine_renderer_end_frame()
        cmd.present(drawable); cmd.commit()
    }

    // Look-at for Z-up world (GoldSrc).
    private func lookAtZUp(eye: simd_float3, center: simd_float3, up: simd_float3) -> simd_float4x4 {
        let f = simd_normalize(center - eye)
        let s = simd_normalize(simd_cross(f, up))
        let u = simd_cross(s, f)
        // Standard RH look-at; Z-up world needs care but this works visually.
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
        v.clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
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
