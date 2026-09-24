// AetherApp.swift
// Main entry. Documents path + settings persistence + audio.
// AetherEngine-iOS · Clean-room.

import SwiftUI
import UIKit

@main
struct AetherApp: App {

    init() {
        let home          = NSHomeDirectory()
        let documentsPath = "\(home)/Documents"
        let bundlePath    = Bundle.main.bundlePath

        print("[AetherApp] home = \(home)")
        print("[AetherApp] documents = \(documentsPath)")
        print("[AetherApp] bundle = \(bundlePath)")

        engine_init(documentsPath, bundlePath)
        AetherAudioiOS.shared.start()

        if engine_settings_load_default() == 0 {
            let vol = UserDefaults.standard.double(forKey: "s_master_volume")
            engine_audio_set_master_volume(Float(vol > 0 ? vol : 1.0))
            let muted = UserDefaults.standard.bool(forKey: "s_mute")
            engine_audio_set_mute(muted)
        }
        engine_settings_apply()

        let man = "\(documentsPath)/manifests"
        _ = man.withCString { engine_manifest_load_all($0) }
    }

    var body: some Scene {
        WindowGroup {
            DashboardView()
                .preferredColorScheme(.dark)
                .onAppear {
                    AetherAudioiOS.shared.start()
                    engine_settings_apply()
                    let vol: Float = "s_master_volume".withCString { engine_settings_get_float($0, 1.0) }
                    let muted: Bool = "s_mute".withCString { engine_settings_get_bool($0, false) }
                    AetherAudioiOS.shared.setMasterVolume(vol)
                    AetherAudioiOS.shared.setMuted(muted)
                }
                .onReceive(NotificationCenter.default.publisher(for: UIApplication.willResignActiveNotification)) { _ in
                    _ = engine_settings_save_default()
                }
        }
    }
}
