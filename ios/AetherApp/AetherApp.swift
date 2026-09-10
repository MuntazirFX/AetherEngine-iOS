// AetherApp.swift
// Main entry point for the AetherEngine iOS application.

import SwiftUI

@main
struct AetherApp: App {
    // Initialize the C Engine Bridge when the app launches
    init() {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].path
        let bundlePath = Bundle.main.bundlePath
        
        // Call our C bridge function to start the engine
        engine_init(documentsPath, bundlePath)
    }
    
    var body: some Scene {
        WindowGroup {
            DashboardView()
                .preferredColorScheme(.dark) // Matches the dark UI mockup
        }
    }
}
