#!/bin/bash

# Monitor 32B model download and transition when ready
echo "Monitoring Qwen2.5-Coder 32B download..."

# Function to restart download if needed
restart_download() {
    echo "🔄 Download appears to have stopped. Restarting..."
    echo "$(date): Restarting 32B model download" >> transition_log.txt
    
    # Kill any existing ollama processes
    pkill -f "ollama pull" || true
    sleep 5
    
    # Restart the download
    ollama pull qwen2.5-coder:32b > /dev/null 2>&1 &
    echo "✅ Download restarted in background"
}

# Check if download is already running
if ! pgrep -f "ollama pull qwen2.5-coder:32b" > /dev/null; then
    echo "📥 Starting 32B model download..."
    ollama pull qwen2.5-coder:32b > /dev/null 2>&1 &
fi

download_check_count=0

while true; do
    # Check if 32B model is available
    if ollama list | grep -q "qwen2.5-coder:32b"; then
        echo "✅ 32B model download completed!"
        echo "🔄 Initiating transition to 32B model..."
        
        # Test 32B model
        echo "Testing 32B model capabilities..."
        ./llm --model qwen2.5-coder:32b --system "You are a senior C++ audio developer" --prompt "Analyze this synthesis engine architecture pattern and confirm you understand the BaseEngine/BaseVoice structure for implementing real-time audio synthesis engines." > 32b_test_response.txt
        
        if [ $? -eq 0 ]; then
            echo "✅ 32B model is responding correctly"
            
            # Queue comprehensive review and remaining work
            echo "🚀 Queueing comprehensive review and remaining work to 32B model..."
            
            ./llm --model qwen2.5-coder:32b --system "You are implementing synthesis engines and GUI for the ether synthesizer project. You are taking over from a 7B model. You must review all previous work and complete both the remaining engines and the complete GUI revamp." --prompt "HANDOFF TO 32B MODEL - COMPREHENSIVE PROJECT COMPLETION

PHASE 1 - ENGINE COMPLETION (Priority):
Review existing engines in src/synthesis/engines/ and implement remaining 7 engines:
1) MacroChord, 2) MacroHarmonics, 3) FormantVocal, 4) NoiseParticles, 5) TidesOsc, 6) RingsVoice, 7) ElementsVoice, 8) DrumKit

PHASE 2 - GUI REVAMP (After engines complete):
Complete SwiftUI GUI implementation using resources in GUI/:
- Timeline View (8-track engine assignment)
- Engine Assignment Overlay (all 11 engines) 
- Sound Overlay (hero macros HARMONICS/TIMBRE/MORPH)
- Pattern Overlays (pitched + drum)
- Mod Page (LFOs + modulation matrix)
- Mix Page (channel strips + bus FX)
- MVVM architecture + touch optimization
- SVG icons + ColorBrewer palette integration

Follow LLM_EXTENDED_TASK_QUEUE.md for complete requirements. Use Mutable source in mutable_extracted/. Maintain real-time safety and professional quality."
            
            echo "✅ Task briefing sent to 32B model"
            echo "📝 Check the model's response and continue monitoring its progress"
            
            # Update status
            echo "$(date): Transitioned to 32B model successfully" >> transition_log.txt
            
            break
        else
            echo "❌ 32B model test failed, waiting..."
        fi
    else
        # Check if download process is still running
        if ! pgrep -f "ollama pull qwen2.5-coder:32b" > /dev/null; then
            download_check_count=$((download_check_count + 1))
            echo "⚠️  Download process not found (attempt $download_check_count/3)"
            
            if [ $download_check_count -ge 3 ]; then
                restart_download
                download_check_count=0
            fi
        else
            download_check_count=0
            echo "⏳ Still downloading... checking again in 2 minutes"
        fi
        
        sleep 120
    fi
done

echo "🎉 Transition complete! 32B model is now working on the project."