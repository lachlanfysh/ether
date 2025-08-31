import SwiftUI
import AppKit

class AppDelegate: NSObject, NSApplicationDelegate {
    func applicationDidFinishLaunching(_ notification: Notification) {
        print("🚀 App finished launching - keeping alive")
        
        // Ensure app stays active and visible
        NSApp.setActivationPolicy(.regular)
        NSApp.activate(ignoringOtherApps: true)
    }
    
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        print("⚠️ Last window closed - but keeping app alive")
        return false // Keep app alive even if window closes
    }
}

@main
struct EtherSynthApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var appDelegate
    @StateObject private var synthController = SynthController()
    
    init() {
        print("🎹 EtherSynth GUI App Starting...")
    }
    
    var body: some Scene {
        WindowGroup {
            EtherSynthMainView()
                .environmentObject(synthController)
                .preferredColorScheme(.light)
                .onAppear {
                    print("🎨 SwiftUI View Appeared!")
                    
                    // Force app activation after view appears
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                        NSApp.setActivationPolicy(.regular)
                        NSApp.activate(ignoringOtherApps: true)
                        
                        // Force window to front and keep it there
                        for window in NSApplication.shared.windows {
                            window.makeKeyAndOrderFront(nil)
                            window.orderFrontRegardless()
                            window.level = .floating
                            print("🖼️ Brought window to front: \(window)")
                        }
                        
                        print("✅ EtherSynth GUI should now be visible and stable!")
                    }
                }
        }
        .windowStyle(.titleBar)
        .windowResizability(.contentSize)
        .defaultPosition(.center)
        .commands {
            // Add menu commands to keep app alive
            CommandGroup(replacing: .appInfo) {
                Button("About EtherSynth") {
                    print("About EtherSynth")
                }
            }
        }
    }
}