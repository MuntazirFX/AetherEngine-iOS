// DashboardView.swift — AetherEngine-iOS · Clean-room.
// STEP 13/14: Launch Game → first-person view with collision.
// STEP 15A: Texture diagnostics button.

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
                            .frame(width: 80, height: 80).foregroundColor(.blue)
                        Text("AetherEngine")
                            .font(.largeTitle).fontWeight(.bold).foregroundColor(.white)
                        Text("Classic FPS. Modern Engine.")
                            .font(.subheadline).foregroundColor(.gray)
                    }
                    .padding(.top, 20)

                    // MARK: - Launch Game
                    Button(action: startGame) {
                        HStack {
                            Image(systemName: "play.fill")
                            Text(isLoadingMap ? "Loading…" : "Launch Game")
                                .fontWeight(.semibold)
                            Spacer()
                            if isLoadingMap { ProgressView() }
                            else { Image(systemName: "chevron.right") }
                        }
                        .padding().background(Color.blue)
                        .foregroundColor(.white).cornerRadius(12)
                    }
                    .padding(.horizontal)
                    .disabled(isLoadingMap)

                    // MARK: - Load Only (debug)
                    Button(action: loadMapOnly) {
                        HStack {
                            Image(systemName: "cube.fill")
                            Text("Load c0a0.bsp (No Camera)").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.gray.opacity(0.35))
                        .foregroundColor(.white).cornerRadius(12)
                    }
                    .padding(.horizontal)

                    // MARK: - Texture Diagnostics (STEP 15A)
                    Button(action: inspectTextures) {
                        HStack {
                            Image(systemName: "photo.on.rectangle.angled")
                            Text("Inspect Textures (WAD + BSP)")
                                .fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.orange.opacity(0.7))
                        .foregroundColor(.white).cornerRadius(12)
                    }
                    .padding(.horizontal)

                    // MARK: - Installed Games
                    VStack(alignment: .leading, spacing: 10) {
                        HStack {
                            Text("Installed Games").font(.headline).foregroundColor(.white)
                            Spacer()
                            Text("View All").font(.subheadline).foregroundColor(.blue)
                        }.padding(.horizontal)

                        ForEach(games, id: \.dir) { g in
                            HStack {
                                Circle().fill(g.status == "Ready" ? Color.green : Color.red)
                                    .frame(width: 10, height: 10)
                                VStack(alignment: .leading) {
                                    Text(g.name).foregroundColor(.white)
                                    Text(g.dir).font(.caption).foregroundColor(.gray)
                                }
                                Spacer()
                                Text(g.status).font(.caption).foregroundColor(.green)
                            }
                            .padding().background(Color(UIColor.secondarySystemBackground))
                            .cornerRadius(10).padding(.horizontal)
                        }
                    }

                    // MARK: - Engine Status
                    VStack(alignment: .leading, spacing: 10) {
                        Text("Engine Status").font(.headline).foregroundColor(.white)
                        HStack {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(.blue).font(.largeTitle)
                            VStack(alignment: .leading) {
                                Text("Renderer: Metal (GPU)")
                                Text("FPS Limit: 120 FPS")
                                Text("First-Person Camera: Active")
                                Text("Collision: Enabled")
                            }.font(.subheadline).foregroundColor(.gray)
                        }
                    }
                    .padding().background(Color(UIColor.secondarySystemBackground))
                    .cornerRadius(12).padding(.horizontal)

                    Spacer()
                }
            }
            .background(Color.black.edgesIgnoringSafeArea(.all))
            .navigationBarHidden(true)
            .alert(isPresented: $showAlert) {
                Alert(title: Text(alertTitle),
                      message: Text(alertMessage),
                      dismissButton: .default(Text("OK")))
            }
            .fullScreenCover(isPresented: $showRenderer) {
                ZStack {
                    MetalView().ignoresSafeArea()
                    TouchControlsView().ignoresSafeArea()
                    VStack {
                        HStack {
                            Button(action: { showRenderer = false }) {
                                Image(systemName: "xmark.circle.fill")
                                    .font(.system(size: 34))
                                    .foregroundColor(.white)
                                    .padding(20)
                            }
                            Spacer()
                        }
                        Spacer()
                    }
                }
            }
        }
    }

    // MARK: - Actions

    private func loadMapOnly() {
        isLoadingMap = true
        "valve".withCString { engine_launch_game($0) }
        let ok = "maps/c0a0.bsp".withCString { engine_bsp_mesh_build($0) }
        isLoadingMap = false
        if ok == 1 {
            alertTitle = "Map Loaded"
            alertMessage = "Vertices: \(engine_bsp_mesh_vertex_count())\nTriangles: \(engine_bsp_mesh_triangle_count())"
            showAlert = true
        } else {
            alertTitle = "Failed"
            alertMessage = "Could not load c0a0.bsp"
            showAlert = true
        }
    }

    private func startGame() {
        isLoadingMap = true
        "valve".withCString { engine_launch_game($0) }
        let ok = "maps/c0a0.bsp".withCString { engine_bsp_mesh_build($0) }
        isLoadingMap = false
        if ok == 1 {
            showRenderer = true
        } else {
            alertTitle = "Load Failed"
            alertMessage = "c0a0.bsp could not be loaded."
            showAlert = true
        }
    }

    private func inspectTextures() {
        // 1. Ensure VFS is mounted
        "valve".withCString { engine_launch_game($0) }

        // 2. Get text summary
        var buffer = [CChar](repeating: 0, count: 8192)
        let result: Int32 = engine_texture_summary_text(&buffer, Int32(buffer.count))
        let output = String(cString: buffer)

        if result == 1 {
            alertTitle = "Texture Diagnostics"
            alertMessage = output
        } else {
            alertTitle = "Diagnostic Failed"
            alertMessage = "Could not run texture diagnostic.\n\n\(output)"
        }
        showAlert = true
        print("[Textures] result: \(result)\n\(output)")
    }
}

struct DashboardView_Previews: PreviewProvider {
    static var previews: some View { DashboardView() }
}
