// AetherApp.swift
// Main entry point for the AetherEngine iOS application.
// AetherEngine-iOS · Clean-room.

import SwiftUI

@main
struct AetherApp: App {

    init() {
        // Only initialize the C engine here. Audio is deferred to onAppear
        // so the app doesn't crash if the audio session isn't ready yet.
        let documentsPath = FileManager.default.urls(
            for: .documentDirectory, in: .userDomainMask
        )[0].path
        let bundlePath = Bundle.main.bundlePath

        engine_init(documentsPath, bundlePath)

        // Apply saved settings to the C engine (values only, no playback).
        let vol = UserDefaults.standard.double(forKey: "s_master_volume")
        engine_audio_set_master_volume(Float(vol > 0 ? vol : 1.0))

        let muted = UserDefaults.standard.bool(forKey: "s_mute")
        engine_audio_set_mute(muted)
    }

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .preferredColorScheme(.dark)
                .onAppear {
                    // Deferred audio startup — runs once the UI is visible.
                    AetherAudioiOS.shared.start()

                    // Re-apply volume now that audio is running.
                    let vol = UserDefaults.standard.double(forKey: "s_master_volume")
                    AetherAudioiOS.shared.setMasterVolume(Float(vol > 0 ? vol : 1.0))
                    let muted = UserDefaults.standard.bool(forKey: "s_mute")
                    AetherAudioiOS.shared.setMuted(muted)
                }
        }
    }
}
