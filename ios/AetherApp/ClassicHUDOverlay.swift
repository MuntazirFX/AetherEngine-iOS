// ClassicHUDOverlay.swift — GoldSrc-style HUD presentation bridge.
// Engine state remains authoritative in EngineBridge/C HUD runtime.

import SwiftUI

struct ClassicHUDOverlay: View {
    @State private var health: Int = 100
    @State private var armor: Int = 0
    @State private var battery: Int = 0
    @State private var clip: Int = 0
    @State private var clipMax: Int = 0
    @State private var reserve: Int = 0
    @State private var weapon: Int = 0
    @State private var alive: Bool = true
    @State private var air: Int = 100
    @State private var airMax: Int = 100
    @State private var drowning: Bool = false
    @State private var crosshairSpread: CGFloat = 0
    @State private var specLabel: String = ""
    @State private var specVisible: Bool = false

    private let poll = Timer.publish(every: 1.0 / 15.0, on: .main, in: .common).autoconnect()

    var body: some View {
        GeometryReader { geo in
            ZStack {
                // Classic center crosshair. Kept deliberately simple like GoldSrc.
                CrosshairView(spread: crosshairSpread)
                    .frame(width: min(geo.size.width, geo.size.height) * 0.12,
                           height: min(geo.size.width, geo.size.height) * 0.12)
                    .allowsHitTesting(false)

                VStack {
                    Spacer()
                    HStack(alignment: .bottom) {
                        statusPanel
                        Spacer()
                        ammoPanel
                    }
                    .padding(.horizontal, 18)
                    .padding(.bottom, 18)
                }

                if specVisible && !specLabel.isEmpty {
                    VStack {
                        Text(specLabel)
                            .font(.system(size: 16, weight: .bold, design: .serif))
                            .foregroundColor(.cyan)
                            .padding(.horizontal, 14)
                            .padding(.vertical, 6)
                            .background(Color.black.opacity(0.55))
                            .overlay(Rectangle().stroke(Color.cyan.opacity(0.35), lineWidth: 1))
                        Spacer()
                    }
                    .padding(.top, 28)
                }

                if !alive {
                    Text("YOU DIED")
                        .font(.system(size: 32, weight: .bold, design: .serif))
                        .foregroundColor(.white)
                        .padding(.horizontal, 24)
                        .padding(.vertical, 12)
                        .background(Color.black.opacity(0.70))
                        .overlay(Rectangle().stroke(Color.white.opacity(0.25), lineWidth: 1))
                }
            }
            .onAppear { refresh() }
            .onReceive(poll) { _ in refresh() }
        }
        .allowsHitTesting(false)
    }

    private var statusPanel: some View {
        HStack(alignment: .bottom, spacing: 12) {
            hudLabeledValue(label: "HEALTH", value: "\(health)", valueColor: healthColor, valueSize: 30)
            if armor > 0 {
                hudLabeledValue(label: "ARMOR", value: "\(armor)", valueColor: .white, valueSize: 24)
            }
            if battery > 0 {
                hudLabeledValue(label: "HEV", value: "\(battery)", valueColor: .white.opacity(0.85), valueSize: 18)
            }
            if air < airMax || drowning {
                hudLabeledValue(label: "AIR", value: "\(air)", valueColor: airColor, valueSize: 24)
            }
        }
        .hudPanelChrome()
    }

    @ViewBuilder
    private var ammoPanel: some View {
        if weapon != 0 {
            VStack(alignment: .trailing, spacing: 1) {
                Text(weaponName)
                    .font(.system(size: 11, weight: .bold, design: .serif))
                    .foregroundColor(.white.opacity(0.62))
                HStack(alignment: .lastTextBaseline, spacing: 5) {
                    Text("\(clip)")
                        .font(.system(size: 31, weight: .bold, design: .serif))
                        .foregroundColor(clip == 0 ? .red : .white)
                    Text("/ \(reserve)")
                        .font(.system(size: 19, weight: .bold, design: .serif))
                        .foregroundColor(.white.opacity(0.80))
                }
            }
            .hudPanelChrome()
        }
    }

    private func hudLabeledValue(label: String, value: String, valueColor: Color, valueSize: CGFloat) -> some View {
        HStack(alignment: .lastTextBaseline, spacing: 6) {
            Text(label)
                .font(.system(size: 12, weight: .bold, design: .serif))
                .foregroundColor(.white.opacity(0.70))
            Text(value)
                .font(.system(size: valueSize, weight: .bold, design: .serif))
                .foregroundColor(valueColor)
                .monospacedDigit()
        }
    }

    private var healthColor: Color {
        if health > 50 { return .white }
        if health > 25 { return .yellow }
        return .red
    }

    private var airColor: Color {
        if drowning { return .red }
        if airMax > 0 && air * 100 / max(airMax, 1) <= 25 { return .orange }
        return .cyan
    }

    private var weaponName: String {
        switch weapon {
        case 1: return "CROWBAR"
        case 2: return "GLOCK"
        case 3: return "PYTHON"
        case 4: return "MP5"
        case 5: return "SHOTGUN"
        case 6: return "CROSSBOW"
        case 7: return "RPG"
        case 8: return "GAUSS"
        case 9: return "EGON"
        case 10: return "HIVEHAND"
        case 11: return "GRENADE"
        case 12: return "SATCHEL"
        case 13: return "TRIPMINE"
        case 14: return "SNARK"
        default: return "WEAPON"
        }
    }

    private func refresh() {
        health = Int(engine_hud_health().rounded())
        armor = Int(engine_hud_armor().rounded())
        battery = Int(engine_hud_battery().rounded())
        clip = Int(engine_hud_clip())
        clipMax = Int(engine_hud_clip_max())
        reserve = Int(engine_hud_reserve_ammo())
        weapon = Int(engine_hud_active_weapon())
        alive = engine_hud_alive()
        let amax = engine_hud_air_max()
        airMax = max(Int(amax.rounded()), 1)
        // Present air as 0..100 style meter relative to air_max.
        if amax > 0 {
            air = Int(((engine_hud_air() / amax) * 100.0).rounded())
        } else {
            air = 0
        }
        drowning = engine_hud_drowning() != 0
        crosshairSpread = CGFloat(engine_hud_crosshair_spread())
        specVisible = engine_spectator_hud_visible() != 0
        if specVisible {
            var buf = [CChar](repeating: 0, count: 64)
            let n = engine_spectator_hud_indicator(&buf, 64)
            if n > 0 {
                specLabel = String(cString: buf)
            } else {
                specLabel = ""
            }
        } else {
            specLabel = ""
        }
        _ = clipMax
    }
}

private struct CrosshairView: View {
    /// 0..1 engine spread; widens the classic crosshair gap when moving/firing.
    var spread: CGFloat = 0

    var body: some View {
        GeometryReader { g in
            let cx = g.size.width / 2
            let cy = g.size.height / 2
            let length = min(g.size.width, g.size.height) * 0.18
            let baseGap = length * 0.70
            let gap = baseGap + length * min(max(spread, 0), 1) * 0.55
            ZStack {
                Rectangle().frame(width: 1.5, height: length)
                    .position(x: cx, y: cy - gap / 2 - length / 2)
                Rectangle().frame(width: 1.5, height: length)
                    .position(x: cx, y: cy + gap / 2 + length / 2)
                Rectangle().frame(width: length, height: 1.5)
                    .position(x: cx - gap / 2 - length / 2, y: cy)
                Rectangle().frame(width: length, height: 1.5)
                    .position(x: cx + gap / 2 + length / 2, y: cy)
            }
            .foregroundColor(.green)
        }
    }
}

private extension View {
    func hudPanelChrome() -> some View {
        self
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color.black.opacity(0.45))
            .overlay(Rectangle().stroke(Color.white.opacity(0.18), lineWidth: 1))
    }
}
