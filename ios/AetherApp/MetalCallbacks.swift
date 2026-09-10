// MetalCallbacks.swift
// Swift-side implementations of C callbacks for the Metal renderer.
// AetherEngine-iOS · Clean-room.

import Foundation
import Metal
import MetalKit

private weak var g_mtkView: MTKView?

@_cdecl("aether_metal_init_swift")
public func aether_metal_init_swift(_ user: UnsafeMutableRawPointer?, _ w: UInt32, _ h: UInt32) -> Int32 {
    g_mtkView = user?.assumingMemoryBound(to: MTKView.self).pointee
    print("[MetalCallbacks] init \(w)x\(h)")
    return 0  // AETHER_OK
}

@_cdecl("aether_metal_resize_swift")
public func aether_metal_resize_swift(_ user: UnsafeMutableRawPointer?, _ w: UInt32, _ h: UInt32) -> Int32 {
    print("[MetalCallbacks] resize \(w)x\(h)")
    return 0  // AETHER_OK
}

@_cdecl("aether_metal_submit_swift")
public func aether_metal_submit_swift(_ user: UnsafeMutableRawPointer?, _ cmd: UnsafeRawPointer?) -> Int32 {
    guard let cmd = cmd else { return 0 }
    // Read command type (first 4 bytes)
    let type = cmd.assumingMemoryBound(to: UInt32.self).pointee
    switch type {
    case 1:  /* BEGIN_FRAME */  print("[MetalCallbacks] begin_frame")
    case 2:  /* CLEAR */        print("[MetalCallbacks] clear")
    case 3:  /* SET_VIEWPORT */ print("[MetalCallbacks] set_viewport")
    case 4:  /* DRAW_WORLD */   print("[MetalCallbacks] draw_world")
    case 5:  /* DRAW_HUD */     print("[MetalCallbacks] draw_hud")
    case 6:  /* END_FRAME */    print("[MetalCallbacks] end_frame")
    default: break
    }
    return 0  // AETHER_OK
}

@_cdecl("aether_metal_shutdown_swift")
public func aether_metal_shutdown_swift(_ user: UnsafeMutableRawPointer?) -> Int32 {
    print("[MetalCallbacks] shutdown")
    return 0  // AETHER_OK
}
