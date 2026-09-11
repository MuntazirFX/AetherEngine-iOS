// DashboardView.swift
// STEP 12 — iOS 14 compatible. Correct dir_name passing.
// AetherEngine-iOS · Clean-room.

import SwiftUI

struct DashboardView: View {
    @State private var alertTitle:   String = ""
    @State private var alertMessage: String = ""
    @State private var showAlert:    Bool   = false
    @State private var showRenderer: Bool   = false
    @State private var isLoadingMap: Bool   = false

    let games: [(name: String, dir: String, status: String)] = [
        ("Half-Life",          "valve",   "Ready"),
        ("Blue Shift",         "bshift",  "Ready"),
        ("Opposing Force",     "gearbox", "Ready"),
        ("Counter-Strike 1.6", "cstrike", "Ready"),
        ("Condition Zero",     "czero",   "Ready")
    ]

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 20) {

                    // MARK: - Header
                    VStack(spacing: 5) {
                        Image(systemName: "cube.transparent")
                            .resizable().scaledToFit()
                            .frame(width: 80, height: 80)
                            .foregroundColor(.blue)
                        Text("AetherEngine")
                            .font(.largeTitle).fontWeight(.bold).foregroundColor(.white)
                        Text("Classic FPS. Modern Engine.")
                            .font(.subheadline).foregroundColor(.gray)
                    }
                    .padding(.top, 20)

                    // MARK: - Launch Game
                    Button(action: launchGame) {
                        HStack {
                            Image(systemName: "play.fill")
                            Text("Launch Game").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.blue)
                        .foregroundColor(.white).cornerRadius(12)
                    }
                    .padding(.horizontal)

                    // MARK: - Render Map
                    Button(action: renderMap) {
                        HStack {
                            Image(systemName: "cube.fill")
                            Text(isLoadingMap ? "Loading…" : "Render c0a0.bsp")
                                .fontWeight(.semibold)
                            Spacer()
                            if isLoadingMap {
                                ProgressView()
                            } else {
                                Image(systemName: "chevron.right")
                            }
                        }
                        .padding().background(Color.purple.opacity(0.75))
                        .foregroundColor(.white).cornerRadius(12)
                    }
                    .padding(.horizontal)
                    .disabled(isLoadingMap)

                    // MARK: - Games List
                    VStack(alignment: .leading, spacing: 10) {
                        HStack {
                            Text("Installed Games")
                                .font(.headline).foregroundColor(.white)
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
                                Text(game.status).font(.caption)
                                    .foregroundColor(game.status == "Ready" ? .green : .red)
                            }
                            .padding()
                            .background(Color(UIColor.secondarySystemBackground))
                            .cornerRadius(10).padding(.horizontal)
                        }
                    }

                    // MARK: - Engine Status
                    VStack(alignment: .leading, spacing: 10) {
                        Text("Engine Status")
                            .font(.headline).foregroundColor(.white)
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
                    .padding()
                    .background(Color(UIColor.secondarySystemBackground))
                    .cornerRadius(12).padding(.horizontal)

                    Spacer()
                }
            }
            .background(Color.black.edgesIgnoringSafeArea(.all))
            .navigationBarHidden(true)
            .alert(isPresented: $showAlert) {
                Alert(title:    Text(alertTitle),
                      message:  Text(alertMessage),
                      dismissButton: .default(Text("OK")))
            }
            .fullScreenCover(isPresented: $showRenderer) {
                ZStack(alignment: .topLeading) {
                    MetalView().ignoresSafeArea()
                    Button(action: { showRenderer = false }) {
                        Image(systemName: "xmark.circle.fill")
                            .font(.system(size: 34))
                            .foregroundColor(.white)
                            .padding(20)
                    }
                }
            }
        }
    }

    // MARK: - Actions

    private func launchGame() {
        // FIX: pass dir_name ("valve"), NOT the full path
        "valve".withCString { engine_launch_game($0) }
        alertTitle = "Game Launched"
        alertMessage = "VFS mounted: valve\nBase: \(basePath())"
        showAlert = true
    }

    private func renderMap() {
        isLoadingMap = true

        // 1. Mount valve (dir_name only!)
        "valve".withCString { engine_launch_game($0) }

        // 2. Build the mesh from VFS
        let vpath = "maps/c0a0.bsp"
        let ok: Int32 = vpath.withCString { engine_bsp_mesh_build($0) }

        if ok == 1 {
            let vCount = engine_bsp_mesh_vertex_count()
            let tCount = engine_bsp_mesh_triangle_count()
            alertTitle = "✅ Mesh Built"
            alertMessage = "Vertices: \(vCount)\nTriangles: \(tCount)\n\nOpening 3D view…"
            showAlert = true
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) {
                isLoadingMap = false
                showRenderer = true
            }
        } else {
            isLoadingMap = false
            let base = basePath()
            alertTitle = "❌ Mesh Build Failed"
            alertMessage = """
            Could not load c0a0.bsp.

            Base: \(base)

            Expected file:
            \(base)/valve/maps/c0a0.bsp
            """
            showAlert = true
        }
    }

    // Helper — returns the engine's Documents base path
    private func basePath() -> String {
        guard let cstr = engine_base_path() else { return "?" }
        return String(cString: cstr)
    }
}

struct DashboardView_Previews: PreviewProvider {
    static var previews: some View { DashboardView() }
}
