// AetherAudioiOS.swift
// Safe AVAudioEngine wrapper. Never crashes if audio isn't available.
// AetherEngine-iOS · Clean-room.

import AVFoundation

final class AetherAudioiOS {

    static let shared = AetherAudioiOS()

    private var engine: AVAudioEngine?
    private var players: [Int: AVAudioPlayerNode] = [:]
    private var initialized = false
    private var masterVolume: Float = 1.0
    private var muted: Bool = false

    private init() {}

    /// Start the audio backend. Safe to call multiple times.
    /// If audio is unavailable, silently disables itself instead of crashing.
    func start() {
        guard !initialized else { return }

        // ---------- 1. Configure audio session safely ----------
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .default, options: [.mixWithOthers])
            try session.setActive(true, options: [])
        } catch {
            print("[AetherAudioiOS] Audio session setup failed (continuing): \(error)")
            // Don't return — some sessions work even if category fails.
        }

        // ---------- 2. Create + start engine safely ----------
        let eng = AVAudioEngine()

        // Ensure IO nodes are realized before calling start().
        // Accessing mainMixerNode forces lazy init of the graph.
        _ = eng.mainMixerNode
        _ = eng.inputNode
        _ = eng.outputNode

        do {
            try eng.start()
        } catch {
            print("[AetherAudioiOS] AVAudioEngine.start() failed (disabling audio): \(error)")
            // Graceful degradation: app continues without audio.
            self.engine = nil
            self.initialized = false
            return
        }

        self.engine = eng
        self.initialized = true
        print("[AetherAudioiOS] Audio engine started OK")
    }

    func stop() {
        guard initialized, let eng = engine else { return }
        eng.stop()
        for (_, node) in players {
            node.stop()
            eng.detach(node)
        }
        players.removeAll()
        engine = nil
        initialized = false
        print("[AetherAudioiOS] Audio engine stopped")
    }

    func setMasterVolume(_ vol: Float) {
        masterVolume = max(0, min(1, vol))
        if initialized, let eng = engine, !muted {
            eng.mainMixerNode.outputVolume = masterVolume
        }
    }

    func setMuted(_ m: Bool) {
        muted = m
        if initialized, let eng = engine {
            eng.mainMixerNode.outputVolume = m ? 0 : masterVolume
        }
    }

    /// Called by the C engine bridge when a voice starts.
    func playVoice(id: Int, asset: String, volume: Float, loop: Bool) {
        guard initialized, let eng = engine else {
            print("[AetherAudioiOS] playVoice ignored (audio not running): \(asset)")
            return
        }
        guard let url = resolve(asset) else {
            print("[AetherAudioiOS] Asset not found: \(asset)")
            return
        }
        do {
            let file = try AVAudioFile(forReading: url)
            let node = AVAudioPlayerNode()
            eng.attach(node)
            eng.connect(node, to: eng.mainMixerNode, format: file.processingFormat)
            node.volume = max(0, min(1, volume))

            node.scheduleFile(file, at: nil) { [weak self] in
                DispatchQueue.main.async {
                    if !loop { self?.stopVoice(id: id) }
                }
            }
            node.play()
            players[id] = node
        } catch {
            print("[AetherAudioiOS] playVoice failed: \(error)")
        }
    }

    func stopVoice(id: Int) {
        guard let eng = engine, let node = players[id] else { return }
        node.stop()
        eng.detach(node)
        players.removeValue(forKey: id)
    }

    func stopAll() {
        guard let eng = engine else { return }
        for (_, node) in players {
            node.stop()
            eng.detach(node)
        }
        players.removeAll()
    }

    // MARK: - Asset resolution
    private func resolve(_ virtualPath: String) -> URL? {
        let docs = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let docURL = docs.appendingPathComponent(virtualPath)
        if FileManager.default.fileExists(atPath: docURL.path) { return docURL }
        return Bundle.main.url(forResource: virtualPath, withExtension: nil)
    }
}
