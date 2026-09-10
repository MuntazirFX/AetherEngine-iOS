// DashboardView.swift
// The main UI matching the provided design (Launch Game, Recent Projects, Installed Games, Engine Status).

import SwiftUI

struct DashboardView: View {
    @State private var selectedGame: String = "Half-Life"
    @State private var engineStatus: String = "v0.20 (stable)"
    
    let games = [
        ("Half-Life", "valve", "Ready"),
        ("Blue Shift", "bshift", "Ready"),
        ("Opposing Force", "gearbox", "Ready"),
        ("Counter-Strike 1.6", "cstrike", "Not Found"),
        ("Condition Zero", "czero", "Not Found")
    ]
    
    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 20) {
                    
                    // MARK: - Header
                    VStack(spacing: 5) {
                        Image(systemName: "cube.transparent") // Placeholder for Xash3D logo
                            .resizable()
                            .scaledToFit()
                            .frame(width: 80, height: 80)
                            .foregroundColor(.blue)
                        
                        Text("AetherEngine")
                            .font(.largeTitle)
                            .fontWeight(.bold)
                            .foregroundColor(.white)
                        
                        Text("Classic FPS. Modern Engine.")
                            .font(.subheadline)
                            .foregroundColor(.gray)
                    }
                    .padding(.top, 20)
                    
                    // MARK: - Launch Button
                    Button(action: {
                        print("Launch Game Tapped")
                        // engine_launch_game() will go here
                    }) {
                        HStack {
                            Image(systemName: "play.fill")
                            Text("Launch Game")
                                .fontWeight(.semibold)
                            Spacer()
                            Image(systemName: "chevron.right")
                        }
                        .padding()
                        .background(Color.blue)
                        .foregroundColor(.white)
                        .cornerRadius(12)
                    }
                    .padding(.horizontal)
                    
                    // MARK: - Installed Games List
                    VStack(alignment: .leading, spacing: 10) {
                        HStack {
                            Text("Installed Games")
                                .font(.headline)
                                .foregroundColor(.white)
                            Spacer()
                            Text("View All")
                                .font(.subheadline)
                                .foregroundColor(.blue)
                        }
                        .padding(.horizontal)
                        
                        ForEach(games, id: \.0) { game in
                            HStack {
                                Circle()
                                    .fill(game.2 == "Ready" ? Color.green : Color.red)
                                    .frame(width: 10, height: 10)
                                
                                VStack(alignment: .leading) {
                                    Text(game.0)
                                        .foregroundColor(.white)
                                    Text(game.1)
                                        .font(.caption)
                                        .foregroundColor(.gray)
                                }
                                Spacer()
                                Text(game.2)
                                    .font(.caption)
                                    .foregroundColor(game.2 == "Ready" ? .green : .red)
                            }
                            .padding()
                            .background(Color(UIColor.secondarySystemBackground))
                            .cornerRadius(10)
                            .padding(.horizontal)
                        }
                    }
                    
                    // MARK: - Engine Status Card
                    VStack(alignment: .leading, spacing: 10) {
                        Text("Engine Status")
                            .font(.headline)
                            .foregroundColor(.white)
                        
                        HStack {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(.blue)
                                .font(.largeTitle)
                            
                            VStack(alignment: .leading) {
                                Text("Renderer: Metal (GPU)")
                                Text("FPS Limit: 120 FPS")
                                Text("Audio: Enabled")
                            }
                            .font(.subheadline)
                            .foregroundColor(.gray)
                        }
                    }
                    .padding()
                    .background(Color(UIColor.secondarySystemBackground))
                    .cornerRadius(12)
                    .padding(.horizontal)
                    
                    Spacer()
                }
            }
            .background(Color.black.edgesIgnoringSafeArea(.all))
            .navigationBarHidden(true)
        }
    }
}

struct DashboardView_Previews: PreviewProvider {
    static var previews: some View {
        DashboardView()
    }
}
