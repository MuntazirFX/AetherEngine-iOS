// MetalCallbacks.swift
// Swift-side implementations of C callbacks for the Metal renderer.
// AetherEngine-iOS · Clean-room.

import Foundation
import Metal
import MetalKit

@_cdecl("aether_metal_init_swift")
public func aether_metal_init_swift(_ user: UnsafeMutableRawPointer?, _ w: UInt32, _ h: UInt32) -> Int32 {
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
    // Read command type (first 4 bytes = aether_render_cmd_type_t enum)
    let type = cmd.assumingMemoryBound(to: UInt32.self).pointee
    switch type {
    case 1:  /* BEGIN_FRAME */  break
    case 2:  /* CLEAR */        break
    case 3:  /* SET_VIEWPORT */ break
    case 4:  /* DRAW_WORLD */   break
    case 5:  /* DRAW_HUD */     break
    case 6:  /* END_FRAME */    break
    default: break
    }
    return 0  // AETHER_OK
}

@_cdecl("aether_metal_shutdown_swift")
public func aether_metal_shutdown_swift(_ user: UnsafeMutableRawPointer?) -> Int32 {
    print("[MetalCallbacks] shutdown")
    return 0  // AETHER_OK
}
