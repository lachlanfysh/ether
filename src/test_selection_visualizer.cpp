#include <iostream>
#include "interface/ui/SelectionVisualizer.h"
#include "sequencer/PatternSelection.h"

int main() {
    std::cout << "EtherSynth Selection Visualizer Test\n";
    std::cout << "====================================\n";
    
    bool allTestsPassed = true;
    
    // Test selection visualizer creation
    std::cout << "Testing SelectionVisualizer creation... ";
    try {
        SelectionVisualizer visualizer;
        
        const auto& style = visualizer.getVisualStyle();
        const auto& layout = visualizer.getGridLayout();
        
        if (style.borderWidth >= 1 &&
            style.cornerSize >= 3 &&
            layout.cellWidth >= 8 &&
            layout.cellHeight >= 8) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test coordinate conversion
    std::cout << "Testing coordinate conversion... ";
    try {
        SelectionVisualizer visualizer;
        
        // Set grid layout
        SelectionVisualizer::GridLayout layout;
        layout.cellWidth = 32;
        layout.cellHeight = 24;
        layout.gridStartX = 10;
        layout.gridStartY = 20;
        layout.cellSpacingX = 2;
        layout.cellSpacingY = 1;
        visualizer.setGridLayout(layout);
        
        auto [pixelX, pixelY] = visualizer.gridToPixel(2, 3);  // Track 2, Step 3
        auto [gridTrack, gridStep] = visualizer.pixelToGrid(pixelX, pixelY);
        
        if (pixelX == 78 &&    // 10 + 2*(32+2) = 78
            pixelY == 95 &&    // 20 + 3*(24+1) = 95
            gridTrack == 2 &&
            gridStep == 3) {
            std::cout << "PASS (grid↔pixel conversion working)\n";
        } else {
            std::cout << "FAIL (coordinate conversion not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test selection rectangle calculation
    std::cout << "Testing selection rectangle calculation... ";
    try {
        SelectionVisualizer visualizer;
        
        SelectionVisualizer::GridLayout layout;
        layout.cellWidth = 20;
        layout.cellHeight = 15;
        layout.gridStartX = 5;
        layout.gridStartY = 10;
        layout.cellSpacingX = 1;
        layout.cellSpacingY = 1;
        visualizer.setGridLayout(layout);
        
        PatternSelection::SelectionBounds bounds(1, 3, 2, 4);  // 3×3 selection
        uint16_t x, y, width, height;
        visualizer.getSelectionRectangle(bounds, x, y, width, height);
        
        // Start: Track 1 = 5 + 1*(20+1) = 26, Step 2 = 10 + 2*(15+1) = 42
        // End: Track 3 = 5 + 3*(20+1) = 68, Step 4 = 10 + 4*(15+1) = 74
        // Width = 68 + 20 - 26 = 62, Height = 74 + 15 - 42 = 47
        if (x == 26 && y == 42 && width == 62 && height == 47) {
            std::cout << "PASS (rectangle: " << x << "," << y << " " << width << "×" << height << ")\n";
        } else {
            std::cout << "FAIL (selection rectangle calculation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test visual style configuration
    std::cout << "Testing visual style configuration... ";
    try {
        SelectionVisualizer visualizer;
        
        SelectionVisualizer::VisualStyle style;
        style.selectionFillColor = 0xFF0000;  // Red
        style.borderWidth = 3;
        style.cornerSize = 8;
        style.fillAlpha = 128;
        visualizer.setVisualStyle(style);
        
        const auto& retrievedStyle = visualizer.getVisualStyle();
        if (retrievedStyle.selectionFillColor == 0xFF0000 &&
            retrievedStyle.borderWidth == 3 &&
            retrievedStyle.cornerSize == 8 &&
            retrievedStyle.fillAlpha == 128) {
            std::cout << "PASS (style configuration applied)\n";
        } else {
            std::cout << "FAIL (visual style configuration not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test animation configuration
    std::cout << "Testing animation configuration... ";
    try {
        SelectionVisualizer visualizer;
        
        SelectionVisualizer::AnimationConfig animConfig;
        animConfig.enableFadeIn = true;
        animConfig.enableBorderGlow = true;
        animConfig.fadeInDuration = 300;
        animConfig.glowIntensity = 200;
        visualizer.setAnimationConfig(animConfig);
        
        const auto& retrievedConfig = visualizer.getAnimationConfig();
        if (retrievedConfig.enableFadeIn &&
            retrievedConfig.enableBorderGlow &&
            retrievedConfig.fadeInDuration == 300 &&
            retrievedConfig.glowIntensity == 200) {
            std::cout << "PASS (animation configuration applied)\n";
        } else {
            std::cout << "FAIL (animation configuration not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test color blending
    std::cout << "Testing color blending... ";
    try {
        SelectionVisualizer visualizer;
        
        uint32_t color1 = 0xFFFF;  // White (RGB565)
        uint32_t color2 = 0x0000;  // Black
        uint32_t blended = visualizer.blendColors(color1, color2, 128);  // 50% blend
        
        if (blended != color1 && blended != color2) {  // Should be between the two
            std::cout << "PASS (color blending working)\n";
        } else {
            std::cout << "FAIL (color blending not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test dirty region tracking
    std::cout << "Testing dirty region tracking... ";
    try {
        SelectionVisualizer visualizer;
        
        if (!visualizer.isDirtyRegionValid()) {
            visualizer.setDirtyRegion(10, 20, 100, 150);
            
            uint16_t x, y, width, height;
            visualizer.getDirtyRegion(x, y, width, height);
            
            if (visualizer.isDirtyRegionValid() &&
                x == 10 && y == 20 && width == 100 && height == 150) {
                visualizer.clearDirtyRegion();
                if (!visualizer.isDirtyRegionValid()) {
                    std::cout << "PASS (dirty region tracking working)\n";
                } else {
                    std::cout << "FAIL (dirty region clear not working)\n";
                    allTestsPassed = false;
                }
            } else {
                std::cout << "FAIL (dirty region setting not working)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initial dirty region state)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test integration with pattern selection
    std::cout << "Testing integration with pattern selection... ";
    try {
        SelectionVisualizer visualizer;
        PatternSelection selection;
        
        visualizer.integrateWithPatternSelection(&selection);
        
        // Test that coordinate conversion callbacks are set up
        // This is primarily tested by ensuring no crashes occur during integration
        std::cout << "PASS (integration completed without errors)\n";
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test animation lifecycle
    std::cout << "Testing animation lifecycle... ";
    try {
        SelectionVisualizer visualizer;
        
        PatternSelection::SelectionBounds bounds(0, 2, 0, 3);
        
        if (!visualizer.isAnimationActive()) {
            visualizer.startSelectionAnimation(bounds);
            
            if (visualizer.isAnimationActive()) {
                visualizer.updateAnimations(100);  // Update at 100ms
                
                visualizer.stopSelectionAnimation();
                if (!visualizer.isAnimationActive()) {
                    std::cout << "PASS (animation lifecycle working)\n";
                } else {
                    std::cout << "FAIL (animation stop not working)\n";
                    allTestsPassed = false;
                }
            } else {
                std::cout << "FAIL (animation start not working)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initial animation state)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL SELECTION VISUALIZER TESTS PASSED!\n";
        std::cout << "Visual selection highlighting with clear boundaries is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}