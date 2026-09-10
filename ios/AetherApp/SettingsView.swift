// SettingsView.swift
// UIKit + SwiftUI settings panel matching AetherEngine's design.

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
                    HStack { Text("Engine"); Spacer(); Text("AetherEngine 0.1.0") }
                    HStack { Text("Target"); Spacer(); Text("ARM64 · iOS") }
                }
            }
            .navigationTitle("Settings")
        }
    }
}

struct SettingsView_Previews: PreviewProvider {
    static var previews: some View { SettingsView() }
}
