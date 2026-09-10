// TouchControlsView.swift
// Renders virtual joystick + action buttons on top of the game.
// Publishes input to the C engine via EngineBridge.

import SwiftUI

struct TouchControlsView: View {
    @AppStorage("touch_layout")  var touchLayout: Int = 0     // 0 = RH, 1 = LH
    @AppStorage("touch_opacity") var touchOpacity: Double = 0.75

    @State private var joystickOffset: CGSize = .zero
    @State private var joystickActive: Bool = false

    var body: some View {
        GeometryReader { geo in
            ZStack {
                // Left-side joystick (RH) or right-side (LH)
                JoystickView(offset: $joystickOffset, active: $joystickActive)
                    .frame(width: 140, height: 140)
                    .position(x: (touchLayout == 0 ? 0.22 : 0.78) * geo.size.width,
                              y: 0.75 * geo.size.height)
                    .opacity(touchOpacity)

                // Action button cluster
                ActionCluster(isLeftHanded: touchLayout == 1,
                              opacity: touchOpacity,
                              size: geo.size)
            }
            .background(Color.clear)
            .onChange(of: joystickOffset) { newValue in
                let nx = Float(newValue.width  / 70.0)
                let ny = Float(newValue.height / 70.0)
                engine_input_set_move(nx, ny)
            }
        }
    }
}

// MARK: - Virtual Joystick
struct JoystickView: View {
    @Binding var offset: CGSize
    @Binding var active: Bool

    var body: some View {
        ZStack {
            Circle()
                .stroke(Color.white.opacity(0.35), lineWidth: 3)
            Circle()
                .fill(Color.white.opacity(0.10))
            Circle()
                .fill(Color.blue.opacity(0.85))
                .frame(width: 60, height: 60)
                .offset(offset)
        }
        .contentShape(Circle())
        .gesture(
            DragGesture(minimumDistance: 0)
                .onChanged { value in
                    active = true
                    let maxR: CGFloat = 70
                    var d = value.translation
                    let mag = sqrt(d.width*d.width + d.height*d.height)
                    if mag > maxR {
                        d.width  *= maxR / mag
                        d.height *= maxR / mag
                    }
                    offset = d
                }
                .onEnded { _ in
                    offset = .zero
                    active = false
                }
        )
    }
}

// MARK: - Action Button Cluster
struct ActionCluster: View {
    let isLeftHanded: Bool
    let opacity: Double
    let size: CGSize

    var body: some View {
        let cx = (isLeftHanded ? 0.18 : 0.82) * size.width
        let cy = 0.78 * size.height

        ZStack {
            actionButton(symbol: "flame.fill",    action: "fire",   x: cx,          y: cy - 50)
            actionButton(symbol: "arrow.up",      action: "jump",   x: cx - 50,     y: cy)
            actionButton(symbol: "arrow.down",    action: "duck",   x: cx + 45,     y: cy + 10)
            actionButton(symbol: "hand.tap",      action: "use",    x: cx - 45,     y: cy - 50)
            actionButton(symbol: "arrow.triangle.2.circlepath", action: "reload",
                                                              x: cx + 10,     y: cy - 85)
        }
        .opacity(opacity)
    }

    @ViewBuilder
    private func actionButton(symbol: String, action: String,
                              x: CGFloat, y: CGFloat) -> some View {
        Button(action: {
            let pressed = !isHeld(action: action)
            engine_input_set_action(action, pressed)
        }) {
            Image(systemName: symbol)
                .font(.system(size: 22, weight: .bold))
                .foregroundColor(.white)
                .frame(width: 55, height: 55)
                .background(Color.black.opacity(0.45))
                .clipShape(Circle())
                .overlay(Circle().stroke(Color.white.opacity(0.35), lineWidth: 1.5))
        }
        .position(x: x, y: y)
    }

    private func isHeld(action: String) -> Bool { return false }
}

struct TouchControlsView_Previews: PreviewProvider {
    static var previews: some View {
        TouchControlsView()
            .background(Color.black)
    }
}
