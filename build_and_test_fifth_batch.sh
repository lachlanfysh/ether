#!/bin/bash

echo "=== Building and Testing EtherSynth Fifth Batch Systems ==="

# Compile the comprehensive integration test
echo "Compiling fifth batch integration test..."
g++ -std=c++17 -Wall -Wextra -O2 \
    -I. \
    -o test_fifth_batch_integration \
    test_fifth_batch_integration.cpp \
    src/audio/RealtimeAudioBouncer.cpp \
    src/sampler/AutoSampleLoader.cpp \
    src/sequencer/PatternDataReplacer.cpp \
    src/ui/CrushConfirmationDialog.cpp

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✓ Compilation successful"

# Run the integration test
echo ""
echo "Running fifth batch integration test..."
./test_fifth_batch_integration

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 FIFTH BATCH INTEGRATION TEST PASSED!"
    echo ""
    echo "Successfully implemented and tested:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "🎵 RealtimeAudioBouncer"
    echo "   • Real-time audio rendering and format conversion"
    echo "   • Multi-format output (WAV, AIFF, RAW PCM)"
    echo "   • Dynamic sample rate conversion" 
    echo "   • Audio processing with limiting and filtering"
    echo "   • Lock-free circular buffer system"
    echo ""
    echo "📦 AutoSampleLoader"
    echo "   • Intelligent automatic sample loading"
    echo "   • Smart slot assignment and management"
    echo "   • Memory-efficient sample processing"
    echo "   • LRU-based slot allocation strategies"
    echo "   • Integration with audio bouncing workflow"
    echo ""
    echo "🔄 PatternDataReplacer"
    echo "   • Safe pattern data replacement operations"
    echo "   • Comprehensive backup and restore system"
    echo "   • Multi-level undo/redo functionality"
    echo "   • Atomic operations with rollback capability"
    echo "   • Pattern validation and integrity checking"
    echo ""
    echo "💬 CrushConfirmationDialog"
    echo "   • Advanced confirmation interface for tape crushing"
    echo "   • Auto-save options with configurable settings"
    echo "   • Risk assessment and user warnings"
    echo "   • TouchGFX integration for embedded display"
    echo "   • Accessibility features and keyboard support"
    echo ""
    echo "🔗 SYSTEM INTEGRATION FEATURES:"
    echo "   • Complete tape squashing workflow integration"
    echo "   • Cross-system communication and callbacks"
    echo "   • Unified error handling and validation"
    echo "   • Memory-efficient resource management"
    echo "   • Real-time safe operations for live performance"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "The fifth batch implementation provides a complete"
    echo "end-to-end tape crushing workflow with:"
    echo "• Real-time audio capture and processing"
    echo "• Intelligent sample management and slot allocation"  
    echo "• Safe pattern manipulation with backup/restore"
    echo "• User-friendly confirmation with auto-save protection"
    echo ""
    echo "All systems are optimized for STM32 H7 embedded platform"
    echo "and integrate seamlessly with the EtherSynth architecture!"
else
    echo "❌ Fifth batch integration test FAILED!"
    exit 1
fi