// AetherAudioiOS.swift
// Wraps AVAudioEngine so the C engine can request playback via callback.
// AetherEngine-iOS · Clean-room.

import AVFoundation

final class AetherAudioiOS {

    static let shared = AetherAudioiOS()

    private let engine = AVAudioEngine()
    private var players: [Int: AVAudioPlayerNode] = [:]
    private var musicPlayer: AVAudioPlayerNode?
    private var masterMixer: AVAudioMixerNode { engine.mainMixerNode }

    private var initialized = false

    private init() {}

    func start() {
        guard !initialized else { return }
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .default, options: [.mixWithOthers])
            try session.setActive(true)

            try engine.start()
            initialized = true
            print("[AetherAudioiOS] Engine started")
        } catch {
            print("[AetherAudioiOS] Failed to start: \(error)")
        }
    }

    func stop() {
        guard initialized else { return }
        engine.stop()
        players.removeAll()
        musicPlayer = nil
        initialized = false
        print("[AetherAudioiOS] Engine stopped")
    }

    func setMasterVolume(_ vol: Float) {
        masterMixer.outputVolume = max(0, min(1, vol))
    }

    func setMuted(_ muted: Bool) {
        masterMixer.outputVolume = muted ? 0 : 1
    }

    /// Called from C bridge when a voice starts.
    func playVoice(id: Int, asset: String, volume: Float, loop: Bool) {
        guard initialized else { return }
        guard let url = resolve(asset) else {
            print("[AetherAudioiOS] Asset not found: \(asset)")
            return
        }
        do {
            let file = try AVAudioFile(forReading: url)
            let node = AVAudioPlayerNode()
            engine.attach(node)
            engine.connect(node, to: masterMixer, format: file.processingFormat)
            node.volume = max(0, min(1, volume))

            if loop {
                node.scheduleFile(file, at: nil, completionHandler: nil)
                // simple loop by re-scheduling after file end
                // (full looping handled by separate music path below)
            } else {
                node.scheduleFile(file, at: nil) { [weak self] in
                    DispatchQueue.main.async {
                        self?.stopVoice(id: id)
                    }
                }
            }
            node.play()
            players[id] = node
        } catch {
            print("[AetherAudioiOS] playVoice failed: \(error)")
        }
    }

    func stopVoice(id: Int) {
        if let node = players[id] {
            node.stop()
            engine.detach(node)
            players.removeValue(forKey: id)
        }
    }

    func stopAll() {
        for (_, node) in players {
            node.stop()
            engine.detach(node)
        }
        players.removeAll()
        if let m = musicPlayer {
            m.stop()
            engine.detach(m)
            musicPlayer = nil
        }
    }

    // MARK: - Asset resolution
    private func resolve(_ virtualPath: String) -> URL? {
        // Try Documents first, then bundle
        let docs = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let docURL = docs.appendingPathComponent(virtualPath)
        if FileManager.default.fileExists(atPath: docURL.path) { return docURL }

        let bundleURL = Bundle.main.url(forResource: virtualPath, withExtension: nil)
        return bundleURL
    }
}
