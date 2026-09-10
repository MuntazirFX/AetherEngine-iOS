// AetherApp.swift
import SwiftUI

@main
struct AetherApp: App {
    init() {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0].path
        let bundlePath = Bundle.main.bundlePath

        // Engine up
        engine_init(documentsPath, bundlePath)

        // Audio backend up
        AetherAudioiOS.shared.start()

        // Wire master volume from settings
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
