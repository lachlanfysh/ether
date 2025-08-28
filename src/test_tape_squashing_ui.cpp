#include <iostream>
#include "interface/ui/TapeSquashingUI.h"
#include "sequencer/PatternSelection.h"

int main() {
    std::cout << "EtherSynth Tape Squashing UI Test\n";
    std::cout << "==================================\n";
    
    bool allTestsPassed = true;
    
    // Test tape squashing UI creation
    std::cout << "Testing TapeSquashingUI creation... ";
    try {
        TapeSquashingUI ui;
        
        bool visible = ui.isVisible();
        bool hasSelection = ui.hasValidSelection();
        bool isActive = ui.isSquashingActive();
        
        if (!visible && !hasSelection && !isActive) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (visible:" << visible << " hasSelection:" << hasSelection << " active:" << isActive << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test settings configuration
    std::cout << "Testing settings configuration... ";
    try {
        TapeSquashingUI ui;
        
        TapeSquashingUI::SquashSettings settings;
        settings.sampleRate = 96000.0f;
        settings.bitDepth = 32;
        settings.enableAutoNormalize = false;
        settings.namePrefix = "TestCrush";
        settings.targetSlot = 5;
        
        ui.setSquashSettings(settings);
        const auto& retrievedSettings = ui.getSquashSettings();
        
        if (std::abs(retrievedSettings.sampleRate - 96000.0f) < 0.1f &&
            retrievedSettings.bitDepth == 32 &&
            !retrievedSettings.enableAutoNormalize &&
            retrievedSettings.namePrefix == "TestCrush" &&
            retrievedSettings.targetSlot == 5) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (settings not applied correctly)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test selection integration
    std::cout << "Testing selection integration... ";
    try {
        TapeSquashingUI ui;
        
        PatternSelection::SelectionBounds selection(2, 5, 4, 8);  // 4×5 selection
        ui.setCurrentSelection(selection);
        
        if (ui.hasValidSelection()) {
            const auto& overview = ui.getSelectionOverview();
            if (overview.trackCount == 4 &&
                overview.stepCount == 5 &&
                overview.totalCells == 20) {
                std::cout << "PASS (4×5 selection = 20 cells)\n";
            } else {
                std::cout << "FAIL (selection overview incorrect)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (selection not recognized as valid)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test crush button state
    std::cout << "Testing crush button state logic... ";
    try {
        TapeSquashingUI ui;
        
        // Initially should not be able to start (no selection)
        bool canStartInitially = ui.canStartSquashing();
        
        // Add valid selection
        PatternSelection::SelectionBounds selection(1, 3, 2, 6);
        ui.setCurrentSelection(selection);
        
        bool canStartWithSelection = ui.canStartSquashing();
        
        if (!canStartInitially && canStartWithSelection) {
            std::cout << "PASS (crush button logic working)\n";
        } else {
            std::cout << "FAIL (crush button state logic incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test progress tracking
    std::cout << "Testing progress tracking... ";
    try {
        TapeSquashingUI ui;
        
        // Test initial progress state
        const auto& initialProgress = ui.getProgressInfo();
        if (initialProgress.currentState != TapeSquashingUI::SquashState::IDLE ||
            initialProgress.progressPercent != 0.0f) {
            std::cout << "FAIL (initial progress state incorrect)\n";
            allTestsPassed = false;
        } else {
            // Update progress
            ui.updateProgress(TapeSquashingUI::SquashState::CAPTURING, 
                             45.5f, "Capturing audio...");
            
            const auto& updatedProgress = ui.getProgressInfo();
            if (updatedProgress.currentState == TapeSquashingUI::SquashState::CAPTURING &&
                std::abs(updatedProgress.progressPercent - 45.5f) < 0.1f &&
                updatedProgress.statusMessage == "Capturing audio...") {
                std::cout << "PASS (progress tracking working)\n";
            } else {
                std::cout << "FAIL (progress update not working)\n";
                allTestsPassed = false;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test UI visibility states
    std::cout << "Testing UI visibility states... ";
    try {
        TapeSquashingUI ui;
        
        // Test show/hide
        if (ui.isVisible()) {
            std::cout << "FAIL (initially visible)\n";
            allTestsPassed = false;
        } else {
            ui.show();
            if (!ui.isVisible()) {
                std::cout << "FAIL (show not working)\n";
                allTestsPassed = false;
            } else {
                ui.hide();
                if (ui.isVisible()) {
                    std::cout << "FAIL (hide not working)\n";
                    allTestsPassed = false;
                } else {
                    std::cout << "PASS (visibility states working)\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test error handling
    std::cout << "Testing error handling... ";
    try {
        TapeSquashingUI ui;
        bool errorHandled = false;
        
        ui.setErrorCallback([&errorHandled](const std::string& error) {
            errorHandled = true;
        });
        
        ui.handleError("Test error message");
        
        const auto& progress = ui.getProgressInfo();
        if (errorHandled && 
            progress.currentState == TapeSquashingUI::SquashState::ERROR) {
            std::cout << "PASS (error handling working)\n";
        } else {
            std::cout << "FAIL (error handling not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test callback system
    std::cout << "Testing callback system... ";
    try {
        TapeSquashingUI ui;
        
        bool tapeSquashCallbackCalled = false;
        bool progressCallbackCalled = false;
        
        ui.setTapeSquashCallback([&tapeSquashCallbackCalled](const PatternSelection::SelectionBounds&, 
                                                             const TapeSquashingUI::SquashSettings&) {
            tapeSquashCallbackCalled = true;
        });
        
        ui.setProgressUpdateCallback([&progressCallbackCalled](const TapeSquashingUI::ProgressInfo&) {
            progressCallbackCalled = true;
        });
        
        // Set up valid selection and start squashing (bypassing confirmation)
        PatternSelection::SelectionBounds selection(0, 1, 0, 3);
        ui.setCurrentSelection(selection);
        
        // Simulate confirmation dialog acceptance
        ui.onConfirmDialogYes();
        
        if (tapeSquashCallbackCalled && progressCallbackCalled) {
            std::cout << "PASS (callback system working)\n";
        } else {
            std::cout << "FAIL (callbacks not triggered)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL TAPE SQUASHING UI TESTS PASSED!\n";
        std::cout << "Tape squashing UI with 'Crush to Tape' action button is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}