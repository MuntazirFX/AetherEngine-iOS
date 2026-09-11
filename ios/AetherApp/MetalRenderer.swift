// MetalRenderer.swift
// Renders the BSP mesh (STEP 12). iOS 14 compatible.
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
    let queue: MTLCommandQueue
    var pipelineState: MTLRenderPipelineState?
    var depthState:    MTLDepthStencilState?

    var vertexBuffer: MTLBuffer?
    var indexBuffer:  MTLBuffer?
    var indexCount:   Int = 0
    var vertexCount:  Int = 0

    var camPos    = simd_float3(0, 0, 500)
    var camTarget = simd_float3(0, 0, 0)
    var camUp     = simd_float3(0, 1, 0)

    init?(mtkView: MTKView) {
        guard let dev = mtkView.device ?? MTLCreateSystemDefaultDevice(),
              let q   = dev.makeCommandQueue() else { return nil }
        self.device = dev
        self.queue  = q
        super.init()
        mtkView.delegate = self
        mtkView.colorPixelFormat = .bgra8Unorm
        mtkView.depthStencilPixelFormat = .depth32Float
        mtkView.clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
        buildPipeline(mtkView: mtkView)
        buildDepthState()

        let opaque = Unmanaged.passUnretained(mtkView).toOpaque()
        engine_renderer_attach_metal(opaque)
    }

    private func buildPipeline(mtkView: MTKView) {
        guard let lib = device.makeDefaultLibrary() else {
            print("[MetalRenderer] No default library"); return
        }
        guard let vfn = lib.makeFunction(name: "aether_vertex_main"),
              let ffn = lib.makeFunction(name: "aether_fragment_main") else {
            print("[MetalRenderer] Missing shader funcs"); return
        }

        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float3; vd.attributes[0].offset = 0
        vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float3; vd.attributes[1].offset = MemoryLayout<Float>.size * 3
        vd.attributes[1].bufferIndex = 0
        vd.attributes[2].format = .float2; vd.attributes[2].offset = MemoryLayout<Float>.size * 6
        vd.attributes[2].bufferIndex = 0
        vd.layouts[0].stride = MemoryLayout<Float>.size * 8
        vd.layouts[0].stepFunction = .perVertex

        let desc = MTLRenderPipelineDescriptor()
        desc.vertexFunction   = vfn
        desc.fragmentFunction = ffn
        desc.vertexDescriptor = vd
        desc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        desc.depthAttachmentPixelFormat      = mtkView.depthStencilPixelFormat

        do { pipelineState = try device.makeRenderPipelineState(descriptor: desc) }
        catch { print("[MetalRenderer] pipeline error: \(error)") }
    }

    private func buildDepthState() {
        let d = MTLDepthStencilDescriptor()
        d.depthCompareFunction = .less
        d.isDepthWriteEnabled  = true
        depthState = device.makeDepthStencilState(descriptor: d)
    }

    func uploadMeshFromEngine() {
        let vCount = Int(engine_bsp_mesh_vertex_count())
        let iCount = Int(engine_bsp_mesh_index_count())
        guard vCount > 0, iCount > 0 else {
            print("[MetalRenderer] No mesh to upload"); return
        }

        let vBytes = vCount * 8 * MemoryLayout<Float>.size
        var vData = [Float](repeating: 0, count: vCount * 8)
        let gotV = vData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_vertices(buf.baseAddress, Int32(vCount)))
        }
        guard gotV > 0 else { print("[MetalRenderer] vertex copy failed"); return }
        vertexBuffer = device.makeBuffer(bytes: vData, length: vBytes, options: .storageModeShared)

        let iBytes = iCount * MemoryLayout<UInt32>.size
        var iData = [UInt32](repeating: 0, count: iCount)
        let gotI = iData.withUnsafeMutableBufferPointer { buf -> Int32 in
            Int32(engine_bsp_mesh_copy_indices(buf.baseAddress, Int32(iCount)))
        }
        guard gotI > 0 else { print("[MetalRenderer] index copy failed"); return }
        indexBuffer = device.makeBuffer(bytes: iData, length: iBytes, options: .storageModeShared)

        indexCount  = iCount
        vertexCount = vCount

        var mn = [Float](repeating: 0, count: 3)
        var mx = [Float](repeating: 0, count: 3)
        var ct = [Float](repeating: 0, count: 3)
        mn.withUnsafeMutableBufferPointer { a in
            mx.withUnsafeMutableBufferPointer { b in
                ct.withUnsafeMutableBufferPointer { c in
                    engine_bsp_mesh_get_bounds(a.baseAddress, b.baseAddress, c.baseAddress)
                }
            }
        }
        let center = simd_float3(ct[0], ct[1], ct[2])
        let size   = simd_float3(mx[0]-mn[0], mx[1]-mn[1], mx[2]-mn[2])
        let radius = max(size.x, max(size.y, size.z))
        camTarget = center
        camPos    = center + simd_float3(radius * 0.6, radius * 0.9, radius * 0.6)

        print("[MetalRenderer] Mesh uploaded: \(vCount) verts, \(iCount) indices")
    }

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        engine_renderer_resize(UInt32(size.width), UInt32(size.height))
    }

    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let rpd      = view.currentRenderPassDescriptor,
              let cmd      = queue.makeCommandBuffer() else { return }

        rpd.colorAttachments[0].clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
        rpd.colorAttachments[0].loadAction = .clear

        engine_renderer_begin_frame()

        guard let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) else {
            cmd.commit(); return
        }
        if let ps = pipelineState { enc.setRenderPipelineState(ps) }
        if let ds = depthState    { enc.setDepthStencilState(ds) }

        if let vb = vertexBuffer, let ib = indexBuffer, indexCount > 0 {
            let viewMat = lookAt(eye: camPos, center: camTarget, up: camUp)
            let projMat = perspective(fovY: 60 * .pi / 180,
                                       aspect: Float(view.drawableSize.width / view.drawableSize.height),
                                       near: 1.0, far: 20000.0)

            var U = Uniforms(model: matrix_identity_float4x4,
                             view: viewMat,
                             proj: projMat,
                             lightDir: simd_normalize(simd_float3(0.3, 0.8, 0.5)),
                             pad0: 0,
                             baseColor: simd_float4(0.8, 0.85, 0.95, 1.0))

            enc.setVertexBuffer(vb, offset: 0, index: 0)
            enc.setVertexBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.setFragmentBytes(&U, length: MemoryLayout<Uniforms>.stride, index: 1)
            enc.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indexCount,
                                      indexType: .uint32,
                                      indexBuffer: ib,
                                      indexBufferOffset: 0)
        }

        enc.endEncoding()
        engine_renderer_end_frame()
        cmd.present(drawable)
        cmd.commit()
    }

    // MARK: - Matrix helpers
    private func lookAt(eye: simd_float3, center: simd_float3, up: simd_float3) -> simd_float4x4 {
        let f = simd_normalize(center - eye)
        let s = simd_normalize(simd_cross(f, up))
        let u = simd_cross(s, f)
        var m = matrix_identity_float4x4
        m.columns.0 = simd_float4(s.x, u.x, -f.x, 0)
        m.columns.1 = simd_float4(s.y, u.y, -f.y, 0)
        m.columns.2 = simd_float4(s.z, u.z, -f.z, 0)
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

// MARK: - SwiftUI wrapper
struct MetalView: UIViewRepresentable {
    func makeCoordinator() -> Coordinator { Coordinator() }

    func makeUIView(context: Context) -> MTKView {
        let view = MTKView()
        view.device = MTLCreateSystemDefaultDevice()
        view.colorPixelFormat = .bgra8Unorm
        view.depthStencilPixelFormat = .depth32Float
        view.clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
        view.preferredFramesPerSecond = 60
        view.enableSetNeedsDisplay = false
        view.isPaused = false
        let r = MetalRenderer(mtkView: view)
        context.coordinator.renderer = r
        DispatchQueue.main.async {
            r?.uploadMeshFromEngine()
        }
        return view
    }

    func updateUIView(_ uiView: MTKView, context: Context) {}

    final class Coordinator {
        var renderer: MetalRenderer?
    }
}
