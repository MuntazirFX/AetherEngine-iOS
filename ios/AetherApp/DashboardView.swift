// DashboardView.swift — AetherEngine-iOS · Clean-room.
// STEP 17A: Entity diagnostics.

import SwiftUI

struct DashboardView: View {
    @State private var alertTitle:   String = ""
    @State private var alertMessage: String = ""
    @State private var showAlert:    Bool   = false
    @State private var showRenderer: Bool   = false
    @State private var isLoadingMap: Bool   = false

    let games: [(name: String, dir: String, status: String)] = [
        ("Half-Life", "valve", "Ready"),
        ("Blue Shift", "bshift", "Ready"),
        ("Opposing Force", "gearbox", "Ready"),
        ("Counter-Strike 1.6", "cstrike", "Ready"),
        ("Condition Zero", "czero", "Ready")
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
                    }.padding(.top, 20)

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
                    }.padding(.horizontal).disabled(isLoadingMap)

                    // MARK: - Load map (debug)
                    Button(action: loadMapOnly) {
                        HStack {
                            Image(systemName: "cube.fill")
                            Text("Load c0a0.bsp (No Camera)").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.gray.opacity(0.35))
                        .foregroundColor(.white).cornerRadius(12)
                    }.padding(.horizontal)

                    // MARK: - Texture diagnostics
                    Button(action: inspectTextures) {
                        HStack {
                            Image(systemName: "photo.on.rectangle.angled")
                            Text("Inspect Textures (WAD + BSP)").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.orange.opacity(0.7))
                        .foregroundColor(.white).cornerRadius(12)
                    }.padding(.horizontal)

                    // MARK: - MDL diagnostics
                    Button(action: inspectMDL) {
                        HStack {
                            Image(systemName: "person.crop.square.fill")
                            Text("Inspect MDL (barney.mdl)").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.green.opacity(0.7))
                        .foregroundColor(.white).cornerRadius(12)
                    }.padding(.horizontal)

                    // MARK: - MDL Mesh extraction
                    Button(action: testMDLMesh) {
                        HStack {
                            Image(systemName: "cube.transparent.fill")
                            Text("Extract MDL Mesh").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.teal.opacity(0.7))
                        .foregroundColor(.white).cornerRadius(12)
                    }.padding(.horizontal)

                    // MARK: - Render Model in Map (STEP 16C)
                    Button(action: renderModel) {
                        HStack {
                            Image(systemName: "person.fill.viewfinder")
                            Text("Render Model in Map").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.pink.opacity(0.7))
                        .foregroundColor(.white).cornerRadius(12)
                    }.padding(.horizontal)

                    // MARK: - Entity diagnostics (STEP 17A)
                    Button(action: inspectEntities) {
                        HStack {
                            Image(systemName: "person.3.fill")
                            Text("Inspect Map Entities").fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding().background(Color.yellow.opacity(0.7))
                        .foregroundColor(.black).cornerRadius(12)
                    }.padding(.horizontal)

                    // MARK: - Games list
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

                    // MARK: - Engine status
                    VStack(alignment: .leading, spacing: 10) {
                        Text("Engine Status").font(.headline).foregroundColor(.white)
                        HStack {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(.blue).font(.largeTitle)
                            VStack(alignment: .leading) {
                                Text("Renderer: Metal (GPU)")
                                Text("FPS Limit: 120 FPS")
                                Text("Textures: Atlas 2048²")
                                Text("Entities: Parsed")
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
                Alert(title: Text(alertTitle), message: Text(alertMessage),
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
        } else {
            alertTitle = "Failed"; alertMessage = "Could not load c0a0.bsp"
        }
        showAlert = true
    }

    private func startGame() {
        isLoadingMap = true
        "valve".withCString { engine_launch_game($0) }
        let ok = "maps/c0a0.bsp".withCString { engine_bsp_mesh_build($0) }
        isLoadingMap = false
        if ok == 1 { showRenderer = true }
        else { alertTitle = "Load Failed"; alertMessage = "c0a0.bsp could not be loaded"; showAlert = true }
    }

    private func inspectTextures() {
        "valve".withCString { engine_launch_game($0) }
        var buffer = [CChar](repeating: 0, count: 8192)
        let result: Int32 = engine_texture_summary_text(&buffer, Int32(buffer.count))
        let output = String(cString: buffer)
        alertTitle = result == 1 ? "Texture Diagnostics" : "Diagnostic Failed"
        alertMessage = output
        showAlert = true
    }

    private func inspectMDL() {
        "valve".withCString { engine_launch_game($0) }
        let candidates = [
            "models/barney.mdl", "models/scientist.mdl", "models/gman.mdl",
            "models/hgrunt.mdl", "models/zombie.mdl", "models/player.mdl"
        ]
        var buffer = [CChar](repeating: 0, count: 8192)
        var chosen = ""
        for path in candidates {
            let r: Int32 = path.withCString { engine_mdl_summary_text($0, &buffer, Int32(buffer.count)) }
            if r == 1 || r == -2 { chosen = path; break }
        }
        let output = String(cString: buffer)
        alertTitle = chosen.isEmpty ? "❌ No MDL Found" : "MDL: \(chosen)"
        alertMessage = output
        showAlert = true
    }

    private func testMDLMesh() {
        "valve".withCString { engine_launch_game($0) }
        let candidates = [
            "models/barney.mdl", "models/scientist.mdl", "models/gman.mdl",
            "models/hgrunt.mdl", "models/zombie.mdl", "models/player.mdl"
        ]
        var chosen = ""
        var ok = Int32(0)
        for path in candidates {
            ok = path.withCString { engine_mdl_mesh_build($0) }
            if ok == 1 { chosen = path; break }
        }
        if ok == 1 {
            let v = engine_mdl_mesh_vertex_count()
            let t = engine_mdl_mesh_triangle_count()
            alertTitle = "✅ MDL Mesh Extracted"
            alertMessage = "File: \(chosen)\n\nVertices: \(v)\nTriangles: \(t)"
        } else {
            alertTitle = "❌ Extraction Failed"
            alertMessage = "Could not extract mesh from any known model."
        }
        showAlert = true
    }

    private func renderModel() {
        isLoadingMap = true
        "valve".withCString { engine_launch_game($0) }

        // 1. Load map
        let mapOk = "maps/c0a0.bsp".withCString { engine_bsp_mesh_build($0) }
        if mapOk != 1 {
            isLoadingMap = false
            alertTitle = "Map Load Failed"
            alertMessage = "Could not load c0a0.bsp"
            showAlert = true
            return
        }

        // 2. Extract model
        let candidates = [
            "models/barney.mdl", "models/scientist.mdl", "models/gman.mdl",
            "models/hgrunt.mdl", "models/zombie.mdl", "models/player.mdl"
        ]
        var chosen = ""
        var mdlOk = Int32(0)
        for path in candidates {
            mdlOk = path.withCString { engine_mdl_mesh_build($0) }
            if mdlOk == 1 { chosen = path; break }
        }

        isLoadingMap = false
        if mdlOk == 1 {
            alertTitle = "Rendering in Map"
            alertMessage = "Map: c0a0.bsp\nModel: \(chosen)\n\nModel is 200 units in front of you."
            showAlert = true
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                showRenderer = true
            }
        } else {
            alertTitle = "Model Load Failed"
            alertMessage = "Could not extract any MDL mesh."
            showAlert = true
        }
    }

    // MARK: - STEP 17A
    private func inspectEntities() {
        // Load a map first (this triggers entity parsing)
        "valve".withCString { engine_launch_game($0) }
        let mapOk = "maps/c0a0.bsp".withCString { engine_bsp_mesh_build($0) }

        if mapOk != 1 {
            alertTitle = "Map Load Failed"
            alertMessage = "Could not load c0a0.bsp"
            showAlert = true
            return
        }

        var buffer = [CChar](repeating: 0, count: 8192)
        let result: Int32 = engine_entity_summary_text(&buffer, Int32(buffer.count))
        let output = String(cString: buffer)
        alertTitle = result == 1 ? "Entity Diagnostics" : "No Entities"
        alertMessage = output
        showAlert = true
        print("[Entities] \(output)")
    }
}

struct DashboardView_Previews: PreviewProvider {
    static var previews: some View { DashboardView() }
}
