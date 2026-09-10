// AetherApp.swift
// Main entry point for the AetherEngine iOS application.

import SwiftUI

@main
struct AetherApp: App {

    init() {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].path
        let bundlePath = Bundle.main.bundlePath

        // 1. Bring the C engine up
        engine_init(documentsPath, bundlePath)

        // 2. Bring the iOS audio backend up
        AetherAudioiOS.shared.start()

        // 3. Apply saved settings
        let vol = UserDefaults.standard.double(forKey: "s_master_volume")
        engine_audio_set_master_volume(Float(vol > 0 ? vol : 1.0))

        let muted = UserDefaults.standard.bool(forKey: "s_mute")
        engine_audio_set_mute(muted)
    }

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .preferredColorScheme(.dark)
        }
    }
}
