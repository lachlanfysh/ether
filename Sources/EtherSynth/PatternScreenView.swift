import SwiftUI

// MARK: - Pattern Screen View (Clean Demo Version)
struct PatternScreenView: View {
    @EnvironmentObject var synthController: SynthController
    
    var body: some View {
        VStack(spacing: 16) {
            Text("PATTERN \(synthController.currentPatternID)")
                .font(.system(size: 16, weight: .bold))
            
            Text("I\(synthController.activeInstrument.index + 1) (\(synthController.getInstrumentName(Int(synthController.uiArmedInstrumentIndex)))) — 16 Steps")
                .foregroundColor(.secondary)
            
            // Simple 1×16 step buttons
            HStack(spacing: 4) {
                ForEach(0..<16, id: \.self) { stepIndex in
                    Button("\(stepIndex + 1)") {
                        synthController.toggleStep(stepIndex)
                    }
                    .frame(width: 50, height: 40)
                    .background(synthController.stepHasContent(stepIndex) ? 
                               synthController.activeInstrument.color : Color.gray.opacity(0.1))
                    .cornerRadius(6)
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(synthController.stepHasContent(stepIndex) ? .white : .primary)
                }
            }
            
            Text("Click buttons 1-16 to toggle steps")
                .font(.system(size: 9))
                .foregroundColor(.secondary)
            
            Spacer()
        }
        .padding()
    }
}