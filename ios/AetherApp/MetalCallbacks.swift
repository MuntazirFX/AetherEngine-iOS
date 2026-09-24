// MetalCallbacks.swift
// C-side Metal hooks. Receives render commands submitted by AetherRender.
// MetalRenderer still owns the actual GPU encode path; these callbacks keep
// C→Swift command traffic alive (init/resize/camera/feature submits).
// AetherEngine-iOS · Clean-room.

import Foundation
import simd

/// Last camera matrices delivered via aether_metal_submit (column-major).
public enum AetherMetalCommandState {
    public static var lastCmdType: Int32 = 0
    public static var view = matrix_identity_float4x4
    public static var proj = matrix_identity_float4x4
    public static var viewportW: UInt32 = 0
    public static var viewportH: UInt32 = 0
    public static var submitCount: UInt64 = 0
    public static var particleSubmitCount: UInt64 = 0
    public static var skySubmitCount: UInt64 = 0
    public static var waterSubmitCount: UInt64 = 0
    public static var fogSubmitCount: UInt64 = 0
    public static var lastFeatureCmd: Int32 = 0
}

// Offsets must match aether_render_cmd_t (verified host sizeof=156).
private let kCmdOffType: Int = 0
private let kCmdOffView: Int = 20
private let kCmdOffProj: Int = 84
private let kCmdOffViewportW: Int = 148
private let kCmdOffViewportH: Int = 152

private let kCmdBeginFrame: Int32 = 1
private let kCmdSetViewport: Int32 = 3
private let kCmdDrawWater: Int32 = 6
private let kCmdDrawSky: Int32 = 7
private let kCmdDrawFog: Int32 = 8
private let kCmdDrawParticles: Int32 = 10

private func loadMat4(from base: UnsafeRawPointer, offset: Int) -> simd_float4x4 {
    let fp = base.advanced(by: offset).assumingMemoryBound(to: Float.self)
    let c0 = simd_float4(fp[0], fp[1], fp[2], fp[3])
    let c1 = simd_float4(fp[4], fp[5], fp[6], fp[7])
    let c2 = simd_float4(fp[8], fp[9], fp[10], fp[11])
    let c3 = simd_float4(fp[12], fp[13], fp[14], fp[15])
    return simd_float4x4(columns: (c0, c1, c2, c3))
}

@_cdecl("aether_metal_init_swift")
public func aether_metal_init_swift(_ user: UnsafeMutableRawPointer?,
                                    _ w: UInt32, _ h: UInt32) -> Int32 {
    AetherMetalCommandState.viewportW = w
    AetherMetalCommandState.viewportH = h
    print("[MetalCallbacks] init \(w)x\(h)")
    return 0
}

@_cdecl("aether_metal_resize_swift")
public func aether_metal_resize_swift(_ user: UnsafeMutableRawPointer?,
                                      _ w: UInt32, _ h: UInt32) -> Int32 {
    AetherMetalCommandState.viewportW = w
    AetherMetalCommandState.viewportH = h
    return 0
}

@_cdecl("aether_metal_submit_swift")
public func aether_metal_submit_swift(_ user: UnsafeMutableRawPointer?,
                                      _ cmd: UnsafeRawPointer?) -> Int32 {
    guard let cmd else { return 0 }
    let type = cmd.load(fromByteOffset: kCmdOffType, as: Int32.self)
    AetherMetalCommandState.lastCmdType = type
    AetherMetalCommandState.submitCount &+= 1
    AetherMetalCommandState.viewportW = cmd.load(fromByteOffset: kCmdOffViewportW, as: UInt32.self)
    AetherMetalCommandState.viewportH = cmd.load(fromByteOffset: kCmdOffViewportH, as: UInt32.self)

    // BEGIN_FRAME and SET_VIEWPORT carry camera matrices from the C renderer.
    if type == kCmdBeginFrame || type == kCmdSetViewport {
        AetherMetalCommandState.view = loadMat4(from: cmd, offset: kCmdOffView)
        AetherMetalCommandState.proj = loadMat4(from: cmd, offset: kCmdOffProj)
    }
    if type == kCmdDrawWater {
        AetherMetalCommandState.lastFeatureCmd = type
        AetherMetalCommandState.waterSubmitCount &+= 1
    }
    if type == kCmdDrawSky {
        AetherMetalCommandState.lastFeatureCmd = type
        AetherMetalCommandState.skySubmitCount &+= 1
    }
    if type == kCmdDrawFog {
        AetherMetalCommandState.lastFeatureCmd = type
        AetherMetalCommandState.fogSubmitCount &+= 1
    }
    if type == kCmdDrawParticles {
        AetherMetalCommandState.lastFeatureCmd = type
        AetherMetalCommandState.particleSubmitCount &+= 1
    }
    return 0
}

@_cdecl("aether_metal_shutdown_swift")
public func aether_metal_shutdown_swift(_ user: UnsafeMutableRawPointer?) -> Int32 {
    print("[MetalCallbacks] shutdown (submits=\(AetherMetalCommandState.submitCount))")
    AetherMetalCommandState.submitCount = 0
    return 0
}
