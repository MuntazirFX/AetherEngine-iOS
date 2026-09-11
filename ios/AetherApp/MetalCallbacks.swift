// MetalCallbacks.swift
// C-side Metal hooks. Renderer handles drawing directly now.
// AetherEngine-iOS · Clean-room.

import Foundation

@_cdecl("aether_metal_init_swift")
public func aether_metal_init_swift(_ user: UnsafeMutableRawPointer?,
                                    _ w: UInt32, _ h: UInt32) -> Int32 {
    print("[MetalCallbacks] init \(w)x\(h)")
    return 0
}

@_cdecl("aether_metal_resize_swift")
public func aether_metal_resize_swift(_ user: UnsafeMutableRawPointer?,
                                      _ w: UInt32, _ h: UInt32) -> Int32 {
    return 0
}

@_cdecl("aether_metal_submit_swift")
public func aether_metal_submit_swift(_ user: UnsafeMutableRawPointer?,
                                      _ cmd: UnsafeRawPointer?) -> Int32 {
    return 0
}

@_cdecl("aether_metal_shutdown_swift")
public func aether_metal_shutdown_swift(_ user: UnsafeMutableRawPointer?) -> Int32 {
    print("[MetalCallbacks] shutdown")
    return 0
}
