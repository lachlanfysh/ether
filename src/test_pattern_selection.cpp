#include <iostream>
#include "sequencer/PatternSelection.h"

int main() {
    std::cout << "EtherSynth Pattern Selection System Test\n";
    std::cout << "========================================\n";
    
    bool allTestsPassed = true;
    
    // Test pattern selection creation
    std::cout << "Testing PatternSelection creation... ";
    try {
        PatternSelection selection;
        
        if (selection.getSelectionState() == PatternSelection::SelectionState::NONE &&
            !selection.hasSelection() &&
            selection.getSelectedCellCount() == 0) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test selection creation and bounds
    std::cout << "Testing selection creation and bounds... ";
    try {
        PatternSelection selection;
        
        PatternSelection::SelectionBounds bounds(2, 5, 4, 8);  // 4×5 selection
        selection.setSelection(bounds);
        
        if (selection.hasSelection() &&
            selection.getSelectionBounds().startTrack == 2 &&
            selection.getSelectionBounds().endTrack == 5 &&
            selection.getSelectionBounds().startStep == 4 &&
            selection.getSelectionBounds().endStep == 8 &&
            selection.getSelectedCellCount() == 20) {  // 4 tracks × 5 steps
            std::cout << "PASS (4×5 selection = 20 cells)\n";
        } else {
            std::cout << "FAIL (selection bounds not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test drag selection workflow
    std::cout << "Testing drag selection workflow... ";
    try {
        PatternSelection selection;
        
        selection.startSelection(1, 2);
        if (selection.getSelectionState() != PatternSelection::SelectionState::SELECTING) {
            std::cout << "FAIL (start selection state)\n";
            allTestsPassed = false;
        } else {
            selection.updateSelection(3, 5);
            selection.completeSelection();
            
            if (selection.hasSelection() &&
                selection.getSelectionBounds().getTrackCount() == 3 &&  // 1 to 3
                selection.getSelectionBounds().getStepCount() == 4) {    // 2 to 5
                std::cout << "PASS (drag selection 3×4)\n";
            } else {
                std::cout << "FAIL (drag selection not working)\n";
                allTestsPassed = false;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test cell membership queries
    std::cout << "Testing cell membership queries... ";
    try {
        PatternSelection selection;
        
        PatternSelection::SelectionBounds bounds(1, 3, 2, 4);  // Tracks 1-3, Steps 2-4
        selection.setSelection(bounds);
        
        bool insideCell = selection.isCellSelected(2, 3);   // Should be selected
        bool outsideCell = selection.isCellSelected(0, 1);  // Should not be selected
        bool trackSelected = selection.isTrackSelected(2);  // Track 2 is selected
        bool stepSelected = selection.isStepSelected(3);    // Step 3 is selected
        
        if (insideCell && !outsideCell && trackSelected && stepSelected) {
            std::cout << "PASS (cell membership working)\n";
        } else {
            std::cout << "FAIL (cell membership queries not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test selection validation
    std::cout << "Testing selection validation... ";
    try {
        PatternSelection selection;
        selection.setSequencerDimensions(16, 32);
        
        PatternSelection::SelectionBounds validBounds(0, 2, 0, 3);    // Valid 3×4 selection
        PatternSelection::SelectionBounds invalidBounds(0, 20, 0, 3); // Invalid (track 20 > 15)
        
        bool validCheck = selection.isValidSelection(validBounds);
        bool invalidCheck = selection.isValidSelection(invalidBounds);
        
        if (validCheck && !invalidCheck) {
            std::cout << "PASS (validation working)\n";
        } else {
            std::cout << "FAIL (selection validation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test minimum selection constraints
    std::cout << "Testing minimum selection constraints... ";
    try {
        PatternSelection selection;
        selection.setMinimumSelection(2, 3);  // Require at least 2 tracks, 3 steps
        
        PatternSelection::SelectionBounds tooSmall(0, 0, 0, 1);  // 1×2 - too small
        PatternSelection::SelectionBounds justRight(0, 1, 0, 2); // 2×3 - minimum valid
        
        bool tooSmallCheck = selection.isValidSelection(tooSmall);
        bool justRightCheck = selection.isValidSelection(justRight);
        
        if (!tooSmallCheck && justRightCheck) {
            std::cout << "PASS (minimum constraints working)\n";
        } else {
            std::cout << "FAIL (minimum selection constraints not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test selection expansion/shrinking
    std::cout << "Testing selection expansion/shrinking... ";
    try {
        PatternSelection selection;
        selection.setSequencerDimensions(16, 32);
        
        PatternSelection::SelectionBounds initialBounds(2, 4, 3, 6);  // 3×4 selection
        selection.setSelection(initialBounds);
        
        selection.expandSelection(1, 2);  // Expand by 1 track, 2 steps
        
        const auto& expandedBounds = selection.getSelectionBounds();
        if (expandedBounds.getTrackCount() == 4 &&  // Should be 4 tracks now
            expandedBounds.getStepCount() == 6) {   // Should be 6 steps now
            std::cout << "PASS (expansion from 3×4 to 4×6)\n";
        } else {
            std::cout << "FAIL (selection expansion not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test select all functionality
    std::cout << "Testing select all functionality... ";
    try {
        PatternSelection selection;
        selection.setSequencerDimensions(8, 16);
        
        selection.selectAll(8, 16);
        
        if (selection.hasSelection() &&
            selection.getSelectedCellCount() == 128 &&  // 8×16
            selection.getSelectionDensity() > 0.99f) {  // Should be 100%
            std::cout << "PASS (select all 8×16 = 128 cells)\n";
        } else {
            std::cout << "FAIL (select all not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test getting selected cells list
    std::cout << "Testing selected cells list... ";
    try {
        PatternSelection selection;
        
        PatternSelection::SelectionBounds bounds(1, 2, 1, 2);  // 2×2 selection
        selection.setSelection(bounds);
        
        auto selectedCells = selection.getSelectedCells();
        auto selectedTracks = selection.getSelectedTracks();
        auto selectedSteps = selection.getSelectedSteps();
        
        if (selectedCells.size() == 4 &&   // 2×2 = 4 cells
            selectedTracks.size() == 2 &&  // 2 tracks
            selectedSteps.size() == 2) {   // 2 steps
            std::cout << "PASS (cell lists correct: 4 cells, 2 tracks, 2 steps)\n";
        } else {
            std::cout << "FAIL (selected cell lists not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL PATTERN SELECTION TESTS PASSED!\n";
        std::cout << "Multi-track rectangular region selection system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}