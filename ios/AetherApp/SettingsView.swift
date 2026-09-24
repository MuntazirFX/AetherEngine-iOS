// SettingsView.swift
// Pushes UI values into C settings/cvars and persists aether.cfg.
// AetherEngine-iOS · Clean-room.

import SwiftUI

struct SettingsView: View {
    @AppStorage("r_fps_limit")        var fpsLimit: Int = 120
    @AppStorage("r_vsync")            var vsync: Bool = true
    @AppStorage("s_master_volume")    var masterVolume: Double = 1.0
    @AppStorage("s_music_volume")     var musicVolume: Double = 0.7
    @AppStorage("s_effects_volume")   var effectsVolume: Double = 1.0
    @AppStorage("s_mute")             var muted: Bool = false
    @AppStorage("touch_layout")       var touchLayout: Int = 0  // 0 = RH, 1 = LH
    @AppStorage("touch_opacity")      var touchOpacity: Double = 0.75
    @AppStorage("in_look_sensitivity") var lookSensitivity: Double = 1.0
    @AppStorage("in_invert_y")        var invertY: Bool = false
    @AppStorage("perf_show_fps")      var showFPS: Bool = false

    var body: some View {
        NavigationView {
            Form {
                Section(header: Text("Graphics")) {
                    Picker("FPS Limit", selection: $fpsLimit) {
                        Text("30").tag(30)
                        Text("60").tag(60)
                        Text("120").tag(120)
                    }
                    Toggle("V-Sync", isOn: $vsync)
                    Toggle("Show FPS Counter", isOn: $showFPS)
                }

                Section(header: Text("Audio")) {
                    Toggle("Mute All", isOn: $muted)
                    Slider(value: $masterVolume, in: 0...1) { Text("Master") }
                    Slider(value: $musicVolume,  in: 0...1) { Text("Music") }
                    Slider(value: $effectsVolume, in: 0...1) { Text("Effects") }
                }

                Section(header: Text("Touch Controls")) {
                    Picker("Layout", selection: $touchLayout) {
                        Text("Right-handed").tag(0)
                        Text("Left-handed").tag(1)
                    }
                    Slider(value: $touchOpacity, in: 0.2...1.0) { Text("Opacity") }
                }

                Section(header: Text("Input")) {
                    Slider(value: $lookSensitivity, in: 0.1...5.0) { Text("Look Sensitivity") }
                    Toggle("Invert Y-Axis", isOn: $invertY)
                }

                Section(header: Text("About")) {
                    HStack { Text("Engine"); Spacer(); Text(String(cString: engine_version())) }
                    HStack { Text("Target"); Spacer(); Text("ARM64 · iOS") }
                    HStack { Text("Running"); Spacer(); Text(engine_is_running() != 0 ? "yes" : "no") }
                }
            }
            .navigationTitle("Settings")
            .onAppear { pushAll() }
            .onChange(of: fpsLimit) { _ in pushAll() }
            .onChange(of: vsync) { _ in pushAll() }
            .onChange(of: masterVolume) { _ in pushAll() }
            .onChange(of: musicVolume) { _ in pushAll() }
            .onChange(of: effectsVolume) { _ in pushAll() }
            .onChange(of: muted) { _ in pushAll() }
            .onChange(of: touchLayout) { _ in pushAll() }
            .onChange(of: touchOpacity) { _ in pushAll() }
            .onChange(of: lookSensitivity) { _ in pushAll() }
            .onChange(of: invertY) { _ in pushAll() }
            .onChange(of: showFPS) { _ in pushAll() }
            .onDisappear { _ = engine_settings_save_default() }
        }
    }

    private func pushAll() {
        "r_fps_limit".withCString { _ = engine_settings_set_int($0, Int32(fpsLimit)) }
        "r_vsync".withCString { _ = engine_settings_set_bool($0, vsync) }
        "s_master_volume".withCString { _ = engine_settings_set_float($0, Float(masterVolume)) }
        "s_music_volume".withCString { _ = engine_settings_set_float($0, Float(musicVolume)) }
        "s_effects_volume".withCString { _ = engine_settings_set_float($0, Float(effectsVolume)) }
        "s_mute".withCString { _ = engine_settings_set_bool($0, muted) }
        "touch_layout".withCString { _ = engine_settings_set_int($0, Int32(touchLayout)) }
        "touch_opacity".withCString { _ = engine_settings_set_float($0, Float(touchOpacity)) }
        "in_look_sensitivity".withCString { _ = engine_settings_set_float($0, Float(lookSensitivity)) }
        "in_invert_y".withCString { _ = engine_settings_set_bool($0, invertY) }
        "perf_show_fps".withCString { _ = engine_settings_set_bool($0, showFPS) }
        AetherAudioiOS.shared.setMasterVolume(Float(masterVolume))
        AetherAudioiOS.shared.setMuted(muted)
        engine_settings_apply()
        _ = engine_settings_save_default()
    }
}

struct SettingsView_Previews: PreviewProvider {
    static var previews: some View { SettingsView() }
}
