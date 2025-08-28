#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include "../sequencer/PatternSelection.h"

/**
 * SelectionVisualizer - Visual selection highlighting with clear start/end boundaries
 * 
 * Provides comprehensive visual feedback for pattern selection:
 * - Real-time selection highlighting during drag operations
 * - Clear visual boundaries with customizable styling
 * - Animation support for selection states and transitions
 * - Integration with TouchGFX for embedded GUI rendering
 * - Performance-optimized drawing for real-time updates
 * 
 * Features:
 * - Multi-layered selection visualization (fill, border, corners, text)
 * - Animated selection feedback with smooth transitions
 * - Customizable colors, transparency, and visual effects
 * - Support for invalid selection indication with visual cues
 * - Optimized drawing with dirty rectangle tracking
 * - Integration with sequencer grid layout and touch handling
 */
class SelectionVisualizer {
public:
    // Visual layer types for selection rendering
    enum class VisualLayer {
        BACKGROUND,     // Selection background fill
        BORDER,         // Selection boundary lines
        CORNERS,        // Corner markers and handles
        DIMENSIONS,     // Dimension text overlay
        ANIMATION       // Animation effects layer
    };
    
    // Animation configuration for selection feedback
    struct AnimationConfig {
        bool enableFadeIn;          // Fade in selection appearance
        bool enablePulse;           // Pulse effect for active selection
        bool enableBorderGlow;      // Glowing border effect
        bool enableCornerBlink;     // Blinking corner markers
        uint16_t fadeInDuration;    // Fade in time (ms)
        uint16_t pulsePeriod;       // Pulse cycle time (ms)
        uint16_t glowIntensity;     // Glow effect intensity (0-255)
        uint16_t blinkPeriod;       // Corner blink period (ms)
        
        AnimationConfig() :
            enableFadeIn(true),
            enablePulse(false),
            enableBorderGlow(true),
            enableCornerBlink(false),
            fadeInDuration(200),
            pulsePeriod(1000),
            glowIntensity(128),
            blinkPeriod(500) {}
    };
    
    // Visual styling configuration
    struct VisualStyle {
        // Colors
        uint32_t selectionFillColor;    // Selection area fill
        uint32_t selectionBorderColor;  // Selection border
        uint32_t cornerMarkerColor;     // Corner marker color
        uint32_t dimensionTextColor;    // Dimension text color
        uint32_t invalidSelectionColor; // Invalid selection warning
        
        // Transparency and effects
        uint8_t fillAlpha;              // Fill transparency (0-255)
        uint8_t borderAlpha;            // Border transparency (0-255)
        uint8_t cornerAlpha;            // Corner marker transparency
        
        // Dimensions
        uint8_t borderWidth;            // Border line width
        uint8_t cornerSize;             // Corner marker size
        uint8_t textSize;               // Dimension text size
        uint8_t glowRadius;             // Glow effect radius
        
        // Visual effects
        bool enableGradientFill;        // Gradient selection fill
        bool enableDropShadow;          // Drop shadow for selection
        bool enableAntiAliasing;        // Anti-aliased rendering
        
        VisualStyle() :
            selectionFillColor(0x3366FF),
            selectionBorderColor(0xFFFFFF),
            cornerMarkerColor(0xFFFF00),
            dimensionTextColor(0xFFFFFF),
            invalidSelectionColor(0xFF3333),
            fillAlpha(64),
            borderAlpha(200),
            cornerAlpha(255),
            borderWidth(2),
            cornerSize(6),
            textSize(12),
            glowRadius(3),
            enableGradientFill(false),
            enableDropShadow(true),
            enableAntiAliasing(true) {}
    };
    
    // Grid layout information for coordinate conversion
    struct GridLayout {
        uint16_t cellWidth;         // Width of each grid cell
        uint16_t cellHeight;        // Height of each grid cell
        uint16_t gridStartX;        // Grid origin X coordinate
        uint16_t gridStartY;        // Grid origin Y coordinate
        uint16_t cellSpacingX;      // Horizontal spacing between cells
        uint16_t cellSpacingY;      // Vertical spacing between cells
        
        GridLayout() :
            cellWidth(32), cellHeight(32),
            gridStartX(0), gridStartY(0),
            cellSpacingX(1), cellSpacingY(1) {}
    };
    
    SelectionVisualizer();
    ~SelectionVisualizer() = default;
    
    // Configuration
    void setVisualStyle(const VisualStyle& style);
    const VisualStyle& getVisualStyle() const { return style_; }
    void setAnimationConfig(const AnimationConfig& config);
    const AnimationConfig& getAnimationConfig() const { return animationConfig_; }
    void setGridLayout(const GridLayout& layout);
    const GridLayout& getGridLayout() const { return gridLayout_; }
    
    // Selection visualization
    void drawSelection(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                      PatternSelection::SelectionState state);
    void drawSelectionLayer(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                           VisualLayer layer, PatternSelection::SelectionState state);
    
    // Individual layer rendering
    void drawSelectionBackground(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                PatternSelection::SelectionState state);
    void drawSelectionBorder(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                            PatternSelection::SelectionState state);
    void drawCornerMarkers(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                          PatternSelection::SelectionState state);
    void drawDimensionText(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                          PatternSelection::SelectionState state);
    void drawAnimationEffects(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                             PatternSelection::SelectionState state);
    
    // Animation management
    void updateAnimations(uint32_t currentTimeMs);
    void startSelectionAnimation(const PatternSelection::SelectionBounds& bounds);
    void stopSelectionAnimation();
    bool isAnimationActive() const { return animationActive_; }
    
    // Coordinate conversion
    std::pair<uint16_t, uint16_t> gridToPixel(uint16_t track, uint16_t step) const;
    std::pair<uint16_t, uint16_t> pixelToGrid(uint16_t x, uint16_t y) const;
    void getSelectionRectangle(const PatternSelection::SelectionBounds& bounds, 
                              uint16_t& x, uint16_t& y, uint16_t& width, uint16_t& height) const;
    
    // Visual effects
    uint32_t blendColors(uint32_t color1, uint32_t color2, uint8_t blend) const;
    uint32_t applyAlpha(uint32_t color, uint8_t alpha) const;
    uint32_t applyGlow(uint32_t baseColor, uint8_t glowIntensity) const;
    
    // Performance optimization
    void setDirtyRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void clearDirtyRegion();
    bool isDirtyRegionValid() const { return dirtyRegionValid_; }
    void getDirtyRegion(uint16_t& x, uint16_t& y, uint16_t& width, uint16_t& height) const;
    
    // Integration helpers
    void integrateWithPatternSelection(PatternSelection* selection);
    void setRedrawCallback(std::function<void(uint16_t, uint16_t, uint16_t, uint16_t)> callback);
    
private:
    // Visual configuration
    VisualStyle style_;
    AnimationConfig animationConfig_;
    GridLayout gridLayout_;
    
    // Animation state
    bool animationActive_;
    uint32_t animationStartTime_;
    uint32_t lastUpdateTime_;
    float fadeInProgress_;
    float pulsePhase_;
    float glowPhase_;
    float blinkPhase_;
    
    // Performance tracking
    bool dirtyRegionValid_;
    uint16_t dirtyX_, dirtyY_, dirtyWidth_, dirtyHeight_;
    
    // Integration
    PatternSelection* selection_;
    std::function<void(uint16_t, uint16_t, uint16_t, uint16_t)> redrawCallback_;
    
    // Helper methods
    void calculateAnimationPhases(uint32_t currentTimeMs);
    uint8_t getAnimatedAlpha(uint8_t baseAlpha, PatternSelection::SelectionState state) const;
    uint32_t getAnimatedColor(uint32_t baseColor, PatternSelection::SelectionState state) const;
    void expandDirtyRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void notifyRedraw(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    
    // Drawing primitives (would use TouchGFX in real implementation)
    void drawRectangle(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                       uint32_t color, uint8_t alpha) const;
    void drawBorder(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                    uint32_t color, uint8_t lineWidth) const;
    void drawCircle(void* graphics, uint16_t centerX, uint16_t centerY, uint16_t radius, 
                    uint32_t color, uint8_t alpha) const;
    void drawText(void* graphics, uint16_t x, uint16_t y, const char* text, 
                  uint32_t color, uint8_t size) const;
    void drawGradientRectangle(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                               uint32_t color1, uint32_t color2, uint8_t alpha) const;
};