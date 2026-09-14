// ClassicVGUIOverlay.swift — AetherEngine-iOS clean-room VGUI bridge.
// The visible controls are driven by the C VGUI runtime rather than a second
// independent menu model.

import SwiftUI
import Combine

struct ClassicVGUIOverlay: View {
    @Binding var isPresented: Bool
    @State private var panelTitle = "Half-Life"
    @State private var items: [String] = []
    @State private var itemTypes: [Int] = []

    private let poll = Timer.publish(every: 0.20, on: .main, in: .common).autoconnect()

    var body: some View {
        GeometryReader { geo in
            ZStack {
                if isPresented {
                    Color.black.opacity(0.52)
                        .ignoresSafeArea()
                        .contentShape(Rectangle())

                    VStack(alignment: .leading, spacing: 0) {
                        HStack(alignment: .bottom) {
                            Text(panelTitle)
                                .font(.system(size: min(38, geo.size.width * 0.075), weight: .bold, design: .serif))
                                .foregroundColor(.white)
                            Spacer()
                            Button("X") { isPresented = false }
                                .font(.system(size: 18, weight: .bold))
                                .foregroundColor(.white.opacity(0.8))
                        }
                        .padding(.bottom, 20)

                        Rectangle()
                            .fill(Color.white.opacity(0.18))
                            .frame(height: 1)
                            .padding(.bottom, 16)

                        ScrollView {
                            VStack(alignment: .leading, spacing: 8) {
                                ForEach(Array(items.enumerated()), id: \.offset) { index, title in
                                    if index < itemTypes.count && itemTypes[index] != 2 {
                                        Button {
                                            activate(index, title: title)
                                        } label: {
                                            HStack {
                                                Text(title)
                                                    .font(.system(size: min(23, geo.size.width * 0.048), weight: .medium, design: .serif))
                                                Spacer()
                                                if itemTypes[index] == 4 {
                                                    Text("•")
                                                }
                                            }
                                            .foregroundColor(.white)
                                            .frame(maxWidth: .infinity, alignment: .leading)
                                            .padding(.vertical, 11)
                                            .padding(.horizontal, 14)
                                            .background(Color.white.opacity(0.06))
                                        }
                                        .buttonStyle(.plain)
                                    }
                                }
                            }
                        }

                        Text("AetherEngine • VGUI compatibility")
                            .font(.caption)
                            .foregroundColor(.white.opacity(0.42))
                            .padding(.top, 16)
                    }
                    .padding(24)
                    .frame(maxWidth: min(560, geo.size.width * 0.86), maxHeight: geo.size.height * 0.78)
                    .background(Color.black.opacity(0.86))
                    .overlay(Rectangle().stroke(Color.white.opacity(0.16), lineWidth: 1))
                    .shadow(radius: 22)
                }
            }
            .onAppear { refresh() }
            .onReceive(poll) { _ in
                if isPresented { refresh() }
            }
        }
    }

    private func refresh() {
        guard engine_vgui_is_visible() else { return }
        var titleBuffer = [CChar](repeating: 0, count: 256)
        _ = engine_vgui_current_panel_text(&titleBuffer, Int32(titleBuffer.count))
        let allText = String(cString: titleBuffer)
        let lines = allText.split(separator: "\n", omittingEmptySubsequences: true).map(String.init)
        panelTitle = lines.first ?? "Half-Life"

        let count = Int(engine_vgui_item_count())
        var newItems: [String] = []
        var newTypes: [Int] = []
        for i in 0..<count {
            var buffer = [CChar](repeating: 0, count: 256)
            let n = engine_vgui_item_text(Int32(i), &buffer, Int32(buffer.count))
            guard n > 0 else { continue }
            newItems.append(String(cString: buffer))
            newTypes.append(Int(engine_vgui_item_type(Int32(i))))
        }
        items = newItems
        itemTypes = newTypes
    }

    private func activate(_ index: Int, title: String) {
        // NEW GAME is the one game-start action owned by the iOS bridge.
        if title.uppercased() == "NEW GAME" {
            if engine_vgui_new_game() == 1 {
                isPresented = false
            }
            return
        }

        _ = engine_vgui_activate_item(Int32(index))
        refresh()
    }
}
