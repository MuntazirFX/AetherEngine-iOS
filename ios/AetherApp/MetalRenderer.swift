// MetalRenderer.swift
// iOS Metal backend. Receives commands from the C renderer abstraction
// and translates them into Metal API calls.

import MetalKit
import SwiftUI

final class MetalRenderer: NSObject, MTKViewDelegate {

    let device: MTLDevice
    let queue: MTLCommandQueue
    var pipelineState: MTLRenderPipelineState?
    var clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)

    var viewSize: CGSize = .zero

    init?(mtkView: MTKView) {
        guard let dev = mtkView.device ?? MTLCreateSystemDefaultDevice(),
              let q   = dev.makeCommandQueue() else { return nil }
        self.device = dev
        self.queue  = q
        super.init()
        mtkView.delegate = self
        buildPipeline(mtkView: mtkView)
    }

    private func buildPipeline(mtkView: MTKView) {
        // Minimal placeholder pipeline. Real world/HUD pipelines added later.
        let lib = device.makeDefaultLibrary()
        let desc = MTLRenderPipelineDescriptor()
        desc.vertexFunction   = lib?.makeFunction(name: "vertex_main")
        desc.fragmentFunction = lib?.makeFunction(name: "fragment_main")
        desc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: desc)
        } catch {
            print("[MetalRenderer] pipeline error: \(error)")
        }
    }

    // MARK: - MTKViewDelegate
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        viewSize = size
        // engine_renderer_resize(UInt32(size.width), UInt32(size.height))
    }

    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let rpd = view.currentRenderPassDescriptor,
              let cmd = queue.makeCommandBuffer() else { return }

        rpd.colorAttachments[0].clearColor = clearColor
        rpd.colorAttachments[0].loadAction = .clear

        guard let enc = cmd.makeRenderCommandEncoder(descriptor: rpd) else {
            cmd.commit(); return
        }

        // --- Placeholder: real draw calls come from engine render commands.
        if let ps = pipelineState {
            enc.setRenderPipelineState(ps)
        }
        enc.endEncoding()

        cmd.present(drawable)
        cmd.commit()
    }
}

// MARK: - SwiftUI wrapper
struct MetalView: UIViewRepresentable {
    func makeCoordinator() -> Coordinator { Coordinator() }

    func makeUIView(context: Context) -> MTKView {
        let view = MTKView()
        view.device = MTLCreateSystemDefaultDevice()
        view.colorPixelFormat = .bgra8Unorm
        view.clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
        view.preferredFramesPerSecond = 120
        view.enableSetNeedsDisplay = false
        view.isPaused = false
        context.coordinator.renderer = MetalRenderer(mtkView: view)
        return view
    }

    func updateUIView(_ uiView: MTKView, context: Context) {}

    final class Coordinator {
        var renderer: MetalRenderer?
    }
}
