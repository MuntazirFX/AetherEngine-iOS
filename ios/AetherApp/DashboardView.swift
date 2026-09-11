// DashboardView.swift
// The main UI — with BSP diagnostic button (STEP 11).
// AetherEngine-iOS · Clean-room.

import SwiftUI

struct DashboardView: View {
    @State private var selectedGame: String = "Half-Life"
    @State private var engineStatus: String = "Ready"

    let games: [(name: String, dir: String, status: String)] = [
        ("Half-Life",             "valve",   "Ready"),
        ("Blue Shift",            "bshift",  "Ready"),
        ("Opposing Force",        "gearbox", "Ready"),
        ("Counter-Strike 1.6",    "cstrike", "Ready"),
        ("Condition Zero",        "czero",   "Ready")
    ]

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 20) {
                    VStack(spacing: 5) {
                        Image(systemName: "cube.transparent")
                            .resizable()
                            .scaledToFit()
                            .frame(width: 80, height: 80)
                            .foregroundColor(.blue)
                        Text("AetherEngine")
                            .font(.largeTitle).fontWeight(.bold).foregroundColor(.white)
                        Text("Classic FPS. Modern Engine.")
                            .font(.subheadline).foregroundColor(.gray)
                    }
                    .padding(.top, 20)

                    Button(action: {
                        let docs = FileManager.default.urls(for: .documentDirectory,
                                                              in: .userDomainMask)[0].path
                        let gamePath = "\(docs)/valve"
                        gamePath.withCString { cstr in engine_launch_game(cstr) }
                    }) {
                        HStack {
                            Image(systemName: "play.fill")
                            Text("Launch Game").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.blue).foregroundColor(.white).cornerRadius(12)
                    }
                    .padding(.horizontal)

                    Button(action: {
                        let docs = FileManager.default.urls(for: .documentDirectory,
                                                              in: .userDomainMask)[0].path
                        let gamePath = "\(docs)/valve"
                        gamePath.withCString { cstr in engine_launch_game(cstr) }
                        let vpath = "maps/c0a0.bsp"
                        vpath.withCString { cstr in
                            let ok = engine_bsp_inspect_vfs(cstr)
                            print("[BSP] inspect result: \(ok)")
                        }
                    }) {
                        HStack {
                            Image(systemName: "doc.text.magnifyingglass")
                            Text("Inspect c0a0.bsp (VFS)").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.gray.opacity(0.35))
                        .foregroundColor(.white).cornerRadius(12)
                    }
                    .padding(.horizontal)

                    VStack(alignment: .leading, spacing: 10) {
                        HStack {
                            Text("Installed Games").font(.headline).foregroundColor(.white)
                            Spacer()
                            Text("View All").font(.subheadline).foregroundColor(.blue)
                        }
                        .padding(.horizontal)
                        ForEach(games, id: \.dir) { game in
                            HStack {
                                Circle()
                                    .fill(game.status == "Ready" ? Color.green : Color.red)
                                    .frame(width: 10, height: 10)
                                VStack(alignment: .leading) {
                                    Text(game.name).foregroundColor(.white)
                                    Text(game.dir).font(.caption).foregroundColor(.gray)
                                }
                                Spacer()
                                Text(game.status)
                                    .font(.caption)
                                    .foregroundColor(game.status == "Ready" ? .green : .red)
                            }
                            .padding().background(Color(UIColor.secondarySystemBackground))
                            .cornerRadius(10).padding(.horizontal)
                        }
                    }

                    VStack(alignment: .leading, spacing: 10) {
                        Text("Engine Status").font(.headline).foregroundColor(.white)
                        HStack {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(.blue).font(.largeTitle)
                            VStack(alignment: .leading) {
                                Text("Renderer: Metal (GPU)")
                                Text("FPS Limit: 120 FPS")
                                Text("Audio: Enabled")
                                Text("VFS: Xash3D-style")
                            }
                            .font(.subheadline).foregroundColor(.gray)
                        }
                    }
                    .padding().background(Color(UIColor.secondarySystemBackground))
                    .cornerRadius(12).padding(.horizontal)
                    Spacer()
                }
            }
            .background(Color.black.edgesIgnoringSafeArea(.all))
            .navigationBarHidden(true)
        }
    }
}

struct DashboardView_Previews: PreviewProvider {
    static var previews: some View { DashboardView() }
}
