import SwiftUI

struct ClassicConsoleOverlay: View {
    @Binding var isPresented: Bool
    @State private var input = ""
    @State private var lines: [String] = []
    private let poll = Timer.publish(every: 0.15, on: .main, in: .common).autoconnect()

    var body: some View {
        GeometryReader { geo in
            if isPresented {
                VStack(spacing: 0) {
                    HStack {
                        Text("CONSOLE").font(.system(size: 18, weight: .bold, design: .monospaced)).foregroundColor(.white)
                        Spacer()
                        Button("X") { close() }.foregroundColor(.white)
                    }.padding(12)
                    ScrollViewReader { proxy in
                        ScrollView {
                            LazyVStack(alignment: .leading, spacing: 4) {
                                ForEach(Array(lines.enumerated()), id: \.offset) { i, line in
                                    Text(line).font(.system(size: 13, design: .monospaced)).foregroundColor(.white.opacity(0.92)).frame(maxWidth: .infinity, alignment: .leading).id(i)
                                }
                            }.padding(12)
                        }
                        .onChange(of: lines.count) { _ in if let last = lines.indices.last { proxy.scrollTo(last, anchor: .bottom) } }
                    }
                    HStack(spacing: 8) {
                        Text(">_").font(.system(size: 13, weight: .bold, design: .monospaced)).foregroundColor(.white)
                        TextField("enter command", text: $input, onCommit: { execute() })
                            .textFieldStyle(.plain).font(.system(size: 14, design: .monospaced)).foregroundColor(.white)
                        Button("EXEC") { execute() }.font(.system(size: 12, weight: .bold, design: .monospaced)).foregroundColor(.white)
                    }.padding(10).background(Color.white.opacity(0.08))
                }
                .frame(width: min(900, geo.size.width * 0.94), height: min(620, geo.size.height * 0.82))
                .background(Color.black.opacity(0.94))
                .overlay(Rectangle().stroke(Color.white.opacity(0.25), lineWidth: 1))
                .onAppear { refresh() }
                .onReceive(poll) { _ in refresh() }
            }
        }
    }

    private func refresh() {
        var out: [String] = []
        let count = Int(engine_console_count())
        if count > 0 {
            for i in 0..<count {
                var buffer = [CChar](repeating: 0, count: 1024); var level: Int32 = 0
                if engine_console_get_line(Int32(i), &buffer, Int32(buffer.count), &level) != 0 { out.append(String(cString: buffer)) }
            }
        }
        lines = out
    }
    private func execute() {
        let command = input.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !command.isEmpty else { return }
        command.withCString { _ = engine_console_execute($0) }
        input = ""; refresh()
    }
    private func close() { engine_console_set_visible(false); isPresented = false }
}
