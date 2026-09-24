import SwiftUI

// GoldSrc-style scoreboard/chat presentation bridge.
// Network packets remain authoritative in the C net layer; this view only presents runtime state.
struct ClassicScoreboardChatOverlay: View {
    @Binding var scoreboardShown: Bool
    @Binding var chatShown: Bool
    @State private var rows: [ScoreRow] = []
    @State private var events: [JoinLeaveRow] = []
    @State private var chat: [ChatRow] = []
    @State private var chatText = ""
    private let poll = Timer.publish(every: 0.15, on: .main, in: .common).autoconnect()

    var body: some View {
        ZStack {
            if scoreboardShown { scoreboardPanel }
            if chatShown { chatPanel }
        }
        .onAppear { refresh() }
        .onReceive(poll) { _ in refresh() }
    }

    // Layout aligns with aether_hud_layout_classic scoreboard rect (top-center).
    private var scoreboardPanel: some View {
        VStack(spacing: 0) {
            Text("SCOREBOARD").font(.system(size: 18, weight: .bold, design: .serif)).foregroundColor(.white)
                .frame(maxWidth: .infinity).padding(.vertical, 10).background(Color.black.opacity(0.85))
            HStack {
                Text("PLAYER").frame(maxWidth: .infinity, alignment: .leading)
                Text("SCORE").frame(width: 60)
                Text("DEATHS").frame(width: 65)
                Text("PING").frame(width: 55)
            }.font(.system(size: 10, weight: .bold, design: .serif)).foregroundColor(.white.opacity(0.65)).padding(8)
            ForEach(rows) { r in
                HStack {
                    Text(r.name).frame(maxWidth: .infinity, alignment: .leading)
                    Text("\(r.score)").frame(width: 60)
                    Text("\(r.deaths)").frame(width: 65)
                    Text("\(r.ping)").frame(width: 55)
                }.font(.system(size: 13, weight: .semibold, design: .serif)).foregroundColor(.white).padding(.horizontal, 8).padding(.vertical, 6)
            }
            if !events.isEmpty {
                VStack(alignment: .leading, spacing: 2) {
                    ForEach(events.suffix(6)) { e in
                        Text(e.text).font(.system(size: 11, design: .serif))
                            .foregroundColor(e.kind == 0 ? Color.green.opacity(0.9) : (e.kind == 2 ? Color.red.opacity(0.9) : Color.orange.opacity(0.9)))
                    }
                }.frame(maxWidth: .infinity, alignment: .leading).padding(.horizontal, 8).padding(.bottom, 4)
            }
            Text("Tap SCOREBOARD to close").font(.system(size: 10)).foregroundColor(.white.opacity(0.45)).padding(8)
        }
        .frame(maxWidth: 520).background(Color.black.opacity(0.78)).overlay(Rectangle().stroke(Color.white.opacity(0.25)))
        .padding(.horizontal, 24).allowsHitTesting(false)
    }

    // Layout aligns with aether_hud_layout_classic chat rect (bottom-center).
    private var chatPanel: some View {
        VStack {
            Spacer()
            VStack(alignment: .leading, spacing: 5) {
                ForEach(chat.suffix(8)) { line in
                    Text(line.text).font(.system(size: 13, design: .serif)).foregroundColor(.white).shadow(radius: 2)
                }
                if chatShown {
                    HStack {
                        TextField("Say...", text: $chatText, onCommit: { sendChat() })
                            .textFieldStyle(.plain).foregroundColor(.white)
                        Button("SEND") { sendChat() }.font(.system(size: 11, weight: .bold, design: .serif))
                    }.padding(7).background(Color.black.opacity(0.75)).overlay(Rectangle().stroke(Color.white.opacity(0.25)))
                }
            }.padding(12).frame(maxWidth: 520, alignment: .leading).background(Color.black.opacity(0.35))
        }.padding(.horizontal, 20).padding(.bottom, 80)
    }

    private func sendChat() {
        let t = chatText.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !t.isEmpty else { return }
        t.withCString { engine_chat_add_text($0) }
        chatText = ""
        refresh()
    }

    private func refresh() {
        var newRows: [ScoreRow] = []
        let count = Int(engine_scoreboard_count())
        for i in 0..<count {
            var name = [CChar](repeating: 0, count: 64); var s: Int32 = 0; var d: Int32 = 0; var p: Int32 = 0
            if engine_scoreboard_get_entry(Int32(i), &name, Int32(name.count), &s, &d, &p) != 0 {
                newRows.append(ScoreRow(id: i, name: String(cString: name), score: Int(s), deaths: Int(d), ping: Int(p)))
            }
        }
        rows = newRows
        var newEvents: [JoinLeaveRow] = []
        let ec = Int(engine_scoreboard_event_count())
        for i in 0..<ec {
            var kind: Int32 = 0; var pid: UInt32 = 0; var tsec: Float = 0
            var name = [CChar](repeating: 0, count: 64)
            if engine_scoreboard_get_event(Int32(i), &kind, &pid, &name, Int32(name.count), &tsec) != 0 {
                let label: String
                let colorKind: Int
                if kind == 2 {
                    label = "killed" // kill feed stub
                    colorKind = 2
                } else if kind == 0 {
                    label = "joined"; colorKind = 0
                } else {
                    label = "left"; colorKind = 1
                }
                newEvents.append(JoinLeaveRow(id: i, kind: colorKind,
                    text: kind == 2 ? "* \(String(cString: name)) \(label)" : "* \(String(cString: name)) \(label)"))
            }
        }
        events = newEvents
        var newChat: [ChatRow] = []
        let cc = Int(engine_chat_count())
        if cc > 0 {
            for i in 0..<cc {
                var text = [CChar](repeating: 0, count: 192); var pid: UInt32 = 0
                if engine_chat_get_line(Int32(i), &text, Int32(text.count), &pid) != 0 {
                    newChat.append(ChatRow(id: i, text: pid == 0 ? "* \(String(cString: text))" : "#\(pid): \(String(cString: text))"))
                }
            }
        }
        chat = newChat
    }
}

private struct ScoreRow: Identifiable { let id: Int; let name: String; let score: Int; let deaths: Int; let ping: Int }
private struct JoinLeaveRow: Identifiable { let id: Int; let kind: Int; let text: String }
private struct ChatRow: Identifiable { let id: Int; let text: String }
