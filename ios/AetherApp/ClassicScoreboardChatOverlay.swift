// ClassicScoreboardChatOverlay.swift
// iOS 14-compatible scoreboard + chat overlay.
// AetherEngine-iOS · Clean-room.

import SwiftUI

struct ClassicScoreboardChatOverlay: View {
    @State private var chatText: String = ""
    @State private var showScoreboard: Bool = false

    var body: some View {
        ZStack {
            // Background overlay
            Color.black.opacity(0.65)
                .ignoresSafeArea()

            VStack(spacing: 20) {
                if showScoreboard {
                    scoreboardPanel
                }
                chatPanel
            }
            .padding()
        }
        .onTapGesture {
            showScoreboard.toggle()
        }
    }

    private var scoreboardPanel: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("SCOREBOARD")
                .font(.headline)
                .foregroundColor(.white)
            Text("Player 1    0")
                .foregroundColor(.gray)
            Text("Player 2    0")
                .foregroundColor(.gray)
        }
        .padding()
        .background(Color.black.opacity(0.7))
        .cornerRadius(10)
    }

    private var chatPanel: some View {
        VStack(spacing: 8) {
            Text("CHAT")
                .font(.headline)
                .foregroundColor(.white)

            // iOS 14-compatible TextField with onCommit
            TextField("Type message...", text: $chatText, onCommit: {
                sendChat()
            })
            .textFieldStyle(RoundedBorderTextFieldStyle())
            .foregroundColor(.black)
            .padding(.horizontal)

            Button(action: sendChat) {
                Text("Send")
                    .fontWeight(.semibold)
                    .foregroundColor(.white)
                    .padding(.horizontal, 24)
                    .padding(.vertical, 8)
                    .background(Color.blue)
                    .cornerRadius(8)
            }
        }
        .padding()
        .background(Color.black.opacity(0.7))
        .cornerRadius(10)
    }

    private func sendChat() {
        guard !chatText.isEmpty else { return }
        print("[Chat] Sending: \(chatText)")
        chatText = ""
    }
}

struct ClassicScoreboardChatOverlay_Previews: PreviewProvider {
    static var previews: some View {
        ClassicScoreboardChatOverlay()
    }
}
