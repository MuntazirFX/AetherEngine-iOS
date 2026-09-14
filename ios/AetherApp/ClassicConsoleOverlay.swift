// ClassicConsoleOverlay.swift
// iOS 14-compatible console overlay.
// AetherEngine-iOS · Clean-room.

import SwiftUI

struct ClassicConsoleOverlay: View {
    @State private var consoleText: String = ""
    @State private var lines: [String] = [
        "AetherEngine Console v0.1.0",
        "Type 'help' for commands",
    ]

    var body: some View {
        ZStack {
            Color.black.opacity(0.85)
                .ignoresSafeArea()

            VStack(alignment: .leading, spacing: 4) {
                // Log lines
                ScrollView {
                    VStack(alignment: .leading, spacing: 2) {
                        ForEach(0..<lines.count, id: \.self) { i in
                            Text(lines[i])
                                .font(.system(.caption, design: .monospaced))
                                .foregroundColor(.green)
                                .frame(maxWidth: .infinity, alignment: .leading)
                        }
                    }
                }
                .frame(maxHeight: 300)

                Divider()
                    .background(Color.green)

                // iOS 14-compatible TextField with onCommit
                HStack(spacing: 4) {
                    Text("]")
                        .font(.system(.body, design: .monospaced))
                        .foregroundColor(.green)
                    TextField("command", text: $consoleText, onCommit: {
                        executeCommand()
                    })
                    .font(.system(.body, design: .monospaced))
                    .foregroundColor(.green)
                    .autocapitalization(.none)
                    .disableAutocorrection(true)
                }
            }
            .padding()
        }
    }

    private func executeCommand() {
        guard !consoleText.isEmpty else { return }
        lines.append("] \(consoleText)")
        lines.append("Unknown command: \(consoleText)")
        consoleText = ""
    }
}

struct ClassicConsoleOverlay_Previews: PreviewProvider {
    static var previews: some View {
        ClassicConsoleOverlay()
    }
}
