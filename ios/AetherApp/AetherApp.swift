// AetherApp.swift
// Main entry point. Uses NSHomeDirectory() for a clean Documents path.
// AetherEngine-iOS · Clean-room.

import SwiftUI

@main
struct AetherApp: App {

    init() {
        // NSHomeDirectory() always returns the app's container root.
        // Concatenating "/Documents" gives us a clean, non-nested path.
        let home          = NSHomeDirectory()
        let documentsPath = "\(home)/Documents"
        let bundlePath    = Bundle.main.bundlePath

        print("[AetherApp] home = \(home)")
        print("[AetherApp] documents = \(documentsPath)")
        print("[AetherApp] bundle = \(bundlePath)")

        // 1. Bring the C engine up
        engine_init(documentsPath, bundlePath)

        // 2. Audio backend
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
                .onAppear {
                    AetherAudioiOS.shared.start()

                    let vol = UserDefaults.standard.double(forKey: "s_master_volume")
                    AetherAudioiOS.shared.setMasterVolume(Float(vol > 0 ? vol : 1.0))
                    let muted = UserDefaults.standard.bool(forKey: "s_mute")
                    AetherAudioiOS.shared.setMuted(muted)
                }
        }
    }
}
