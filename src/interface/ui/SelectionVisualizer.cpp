#include "SelectionVisualizer.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

SelectionVisualizer::SelectionVisualizer() {
    style_ = VisualStyle();
    animationConfig_ = AnimationConfig();
    gridLayout_ = GridLayout();
    
    // Animation state
    animationActive_ = false;
    animationStartTime_ = 0;
    lastUpdateTime_ = 0;
    fadeInProgress_ = 0.0f;
    pulsePhase_ = 0.0f;
    glowPhase_ = 0.0f;
    blinkPhase_ = 0.0f;
    
    // Performance tracking
    dirtyRegionValid_ = false;
    dirtyX_ = 0; dirtyY_ = 0; dirtyWidth_ = 0; dirtyHeight_ = 0;
    
    // Integration
    selection_ = nullptr;
}

// Configuration
void SelectionVisualizer::setVisualStyle(const VisualStyle& style) {
    style_ = style;
    
    // Clamp values to valid ranges
    style_.fillAlpha = std::min(static_cast<uint8_t>(255), style_.fillAlpha);
    style_.borderAlpha = std::min(static_cast<uint8_t>(255), style_.borderAlpha);
    style_.cornerAlpha = std::min(static_cast<uint8_t>(255), style_.cornerAlpha);
    style_.borderWidth = std::max(static_cast<uint8_t>(1), style_.borderWidth);
    style_.cornerSize = std::max(static_cast<uint8_t>(3), style_.cornerSize);
}

void SelectionVisualizer::setAnimationConfig(const AnimationConfig& config) {
    animationConfig_ = config;
    
    // Clamp animation parameters
    animationConfig_.fadeInDuration = std::max(static_cast<uint16_t>(50), animationConfig_.fadeInDuration);
    animationConfig_.pulsePeriod = std::max(static_cast<uint16_t>(100), animationConfig_.pulsePeriod);
    animationConfig_.blinkPeriod = std::max(static_cast<uint16_t>(100), animationConfig_.blinkPeriod);
    animationConfig_.glowIntensity = std::min(static_cast<uint16_t>(255), animationConfig_.glowIntensity);
}

void SelectionVisualizer::setGridLayout(const GridLayout& layout) {
    gridLayout_ = layout;
    
    // Ensure minimum cell sizes
    gridLayout_.cellWidth = std::max(static_cast<uint16_t>(8), gridLayout_.cellWidth);
    gridLayout_.cellHeight = std::max(static_cast<uint16_t>(8), gridLayout_.cellHeight);
}

// Selection visualization
void SelectionVisualizer::drawSelection(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                       PatternSelection::SelectionState state) {
    // Draw all layers in order
    drawSelectionLayer(graphics, bounds, VisualLayer::BACKGROUND, state);
    drawSelectionLayer(graphics, bounds, VisualLayer::BORDER, state);
    drawSelectionLayer(graphics, bounds, VisualLayer::CORNERS, state);
    drawSelectionLayer(graphics, bounds, VisualLayer::DIMENSIONS, state);
    
    if (animationActive_) {
        drawSelectionLayer(graphics, bounds, VisualLayer::ANIMATION, state);
    }
}

void SelectionVisualizer::drawSelectionLayer(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                            VisualLayer layer, PatternSelection::SelectionState state) {
    switch (layer) {
        case VisualLayer::BACKGROUND:
            drawSelectionBackground(graphics, bounds, state);
            break;
        case VisualLayer::BORDER:
            drawSelectionBorder(graphics, bounds, state);
            break;
        case VisualLayer::CORNERS:
            drawCornerMarkers(graphics, bounds, state);
            break;
        case VisualLayer::DIMENSIONS:
            drawDimensionText(graphics, bounds, state);
            break;
        case VisualLayer::ANIMATION:
            drawAnimationEffects(graphics, bounds, state);
            break;
    }
}

// Individual layer rendering
void SelectionVisualizer::drawSelectionBackground(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                                 PatternSelection::SelectionState state) {
    uint16_t x, y, width, height;
    getSelectionRectangle(bounds, x, y, width, height);
    
    uint32_t color = (state == PatternSelection::SelectionState::INVALID) ? 
                     style_.invalidSelectionColor : style_.selectionFillColor;
    uint8_t alpha = getAnimatedAlpha(style_.fillAlpha, state);
    
    if (style_.enableGradientFill) {
        uint32_t color2 = applyAlpha(color, alpha / 2);
        drawGradientRectangle(graphics, x, y, width, height, color, color2, alpha);
    } else {
        drawRectangle(graphics, x, y, width, height, color, alpha);
    }
}

void SelectionVisualizer::drawSelectionBorder(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                             PatternSelection::SelectionState state) {
    uint16_t x, y, width, height;
    getSelectionRectangle(bounds, x, y, width, height);
    
    uint32_t color = (state == PatternSelection::SelectionState::INVALID) ? 
                     style_.invalidSelectionColor : style_.selectionBorderColor;
    
    if (animationConfig_.enableBorderGlow && animationActive_) {
        color = applyGlow(color, static_cast<uint8_t>(animationConfig_.glowIntensity * glowPhase_));
    }
    
    drawBorder(graphics, x, y, width, height, color, style_.borderWidth);
}

void SelectionVisualizer::drawCornerMarkers(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                           PatternSelection::SelectionState state) {
    uint16_t x, y, width, height;
    getSelectionRectangle(bounds, x, y, width, height);
    
    uint32_t color = style_.cornerMarkerColor;
    uint8_t alpha = style_.cornerAlpha;
    
    if (animationConfig_.enableCornerBlink && animationActive_) {
        alpha = static_cast<uint8_t>(alpha * (0.5f + 0.5f * blinkPhase_));
    }
    
    uint16_t cornerSize = style_.cornerSize;
    
    // Draw corner markers at four corners
    drawCircle(graphics, x, y, cornerSize, color, alpha);                           // Top-left
    drawCircle(graphics, x + width, y, cornerSize, color, alpha);                  // Top-right
    drawCircle(graphics, x, y + height, cornerSize, color, alpha);                 // Bottom-left
    drawCircle(graphics, x + width, y + height, cornerSize, color, alpha);         // Bottom-right
}

void SelectionVisualizer::drawDimensionText(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                           PatternSelection::SelectionState state) {
    uint16_t x, y, width, height;
    getSelectionRectangle(bounds, x, y, width, height);
    
    // Format dimension text
    char dimensionText[16];
    snprintf(dimensionText, sizeof(dimensionText), "%d×%d", 
             bounds.getTrackCount(), bounds.getStepCount());
    
    // Position text at center of selection
    uint16_t textX = x + width / 2;
    uint16_t textY = y + height / 2;
    
    uint32_t color = (state == PatternSelection::SelectionState::INVALID) ? 
                     style_.invalidSelectionColor : style_.dimensionTextColor;
    
    drawText(graphics, textX, textY, dimensionText, color, style_.textSize);
}

void SelectionVisualizer::drawAnimationEffects(void* graphics, const PatternSelection::SelectionBounds& bounds, 
                                               PatternSelection::SelectionState state) {
    if (!animationActive_) {
        return;
    }
    
    uint16_t x, y, width, height;
    getSelectionRectangle(bounds, x, y, width, height);
    
    if (animationConfig_.enablePulse) {
        // Draw pulsing outline
        uint8_t pulseIntensity = static_cast<uint8_t>(128 + 127 * pulsePhase_);
        uint32_t pulseColor = applyAlpha(style_.selectionBorderColor, pulseIntensity);
        drawBorder(graphics, x - 2, y - 2, width + 4, height + 4, pulseColor, 1);
    }
}

// Animation management
void SelectionVisualizer::updateAnimations(uint32_t currentTimeMs) {
    if (!animationActive_) {
        return;
    }
    
    lastUpdateTime_ = currentTimeMs;
    calculateAnimationPhases(currentTimeMs);
    
    // Check if fade-in animation is complete
    if (animationConfig_.enableFadeIn && fadeInProgress_ >= 1.0f) {
        if (!animationConfig_.enablePulse && !animationConfig_.enableBorderGlow && 
            !animationConfig_.enableCornerBlink) {
            // No continuous animations, stop animation
            animationActive_ = false;
        }
    }
}

void SelectionVisualizer::startSelectionAnimation(const PatternSelection::SelectionBounds& bounds) {
    animationActive_ = true;
    animationStartTime_ = lastUpdateTime_;  // Would use real timestamp in actual implementation
    fadeInProgress_ = 0.0f;
    pulsePhase_ = 0.0f;
    glowPhase_ = 0.0f;
    blinkPhase_ = 0.0f;
    
    // Mark selection area as dirty for redraw
    uint16_t x, y, width, height;
    getSelectionRectangle(bounds, x, y, width, height);
    setDirtyRegion(x - 10, y - 10, width + 20, height + 20);  // Add margin for effects
}

void SelectionVisualizer::stopSelectionAnimation() {
    animationActive_ = false;
}

// Coordinate conversion
std::pair<uint16_t, uint16_t> SelectionVisualizer::gridToPixel(uint16_t track, uint16_t step) const {
    uint16_t x = gridLayout_.gridStartX + track * (gridLayout_.cellWidth + gridLayout_.cellSpacingX);
    uint16_t y = gridLayout_.gridStartY + step * (gridLayout_.cellHeight + gridLayout_.cellSpacingY);
    return {x, y};
}

std::pair<uint16_t, uint16_t> SelectionVisualizer::pixelToGrid(uint16_t x, uint16_t y) const {
    if (x < gridLayout_.gridStartX || y < gridLayout_.gridStartY) {
        return {0, 0};
    }
    
    uint16_t track = (x - gridLayout_.gridStartX) / (gridLayout_.cellWidth + gridLayout_.cellSpacingX);
    uint16_t step = (y - gridLayout_.gridStartY) / (gridLayout_.cellHeight + gridLayout_.cellSpacingY);
    
    return {track, step};
}

void SelectionVisualizer::getSelectionRectangle(const PatternSelection::SelectionBounds& bounds, 
                                                uint16_t& x, uint16_t& y, uint16_t& width, uint16_t& height) const {
    auto [startX, startY] = gridToPixel(bounds.startTrack, bounds.startStep);
    auto [endX, endY] = gridToPixel(bounds.endTrack, bounds.endStep);
    
    x = startX;
    y = startY;
    width = endX + gridLayout_.cellWidth - startX;
    height = endY + gridLayout_.cellHeight - startY;
}

// Visual effects
uint32_t SelectionVisualizer::blendColors(uint32_t color1, uint32_t color2, uint8_t blend) const {
    // Simple RGB565 color blending
    blend = std::min(static_cast<uint8_t>(255), blend);
    float blendFactor = blend / 255.0f;
    
    uint16_t r1 = (color1 >> 11) & 0x1F;
    uint16_t g1 = (color1 >> 5) & 0x3F;
    uint16_t b1 = color1 & 0x1F;
    
    uint16_t r2 = (color2 >> 11) & 0x1F;
    uint16_t g2 = (color2 >> 5) & 0x3F;
    uint16_t b2 = color2 & 0x1F;
    
    uint16_t r = static_cast<uint16_t>(r1 + blendFactor * (r2 - r1));
    uint16_t g = static_cast<uint16_t>(g1 + blendFactor * (g2 - g1));
    uint16_t b = static_cast<uint16_t>(b1 + blendFactor * (b2 - b1));
    
    return (r << 11) | (g << 5) | b;
}

uint32_t SelectionVisualizer::applyAlpha(uint32_t color, uint8_t alpha) const {
    // In real implementation, would properly blend with background
    // For now, just return the color (alpha would be handled by graphics system)
    return color;
}

uint32_t SelectionVisualizer::applyGlow(uint32_t baseColor, uint8_t glowIntensity) const {
    // Brighten the color for glow effect
    uint16_t r = (baseColor >> 11) & 0x1F;
    uint16_t g = (baseColor >> 5) & 0x3F;
    uint16_t b = baseColor & 0x1F;
    
    float glowFactor = 1.0f + (glowIntensity / 255.0f) * 0.5f;
    
    r = std::min(31, static_cast<int>(r * glowFactor));
    g = std::min(63, static_cast<int>(g * glowFactor));
    b = std::min(31, static_cast<int>(b * glowFactor));
    
    return (r << 11) | (g << 5) | b;
}

// Performance optimization
void SelectionVisualizer::setDirtyRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (dirtyRegionValid_) {
        expandDirtyRegion(x, y, width, height);
    } else {
        dirtyX_ = x;
        dirtyY_ = y;
        dirtyWidth_ = width;
        dirtyHeight_ = height;
        dirtyRegionValid_ = true;
    }
}

void SelectionVisualizer::clearDirtyRegion() {
    dirtyRegionValid_ = false;
    dirtyX_ = 0; dirtyY_ = 0; dirtyWidth_ = 0; dirtyHeight_ = 0;
}

void SelectionVisualizer::getDirtyRegion(uint16_t& x, uint16_t& y, uint16_t& width, uint16_t& height) const {
    x = dirtyX_;
    y = dirtyY_;
    width = dirtyWidth_;
    height = dirtyHeight_;
}

// Integration helpers
void SelectionVisualizer::integrateWithPatternSelection(PatternSelection* selection) {
    selection_ = selection;
    
    if (selection_) {
        // Set up coordinate conversion callbacks
        selection_->setGridToCoordinateCallback([this](uint16_t track, uint16_t step) {
            return this->gridToPixel(track, step);
        });
        
        selection_->setCoordinateToGridCallback([this](uint16_t x, uint16_t y) {
            return this->pixelToGrid(x, y);
        });
    }
}

void SelectionVisualizer::setRedrawCallback(std::function<void(uint16_t, uint16_t, uint16_t, uint16_t)> callback) {
    redrawCallback_ = callback;
}

// Helper methods
void SelectionVisualizer::calculateAnimationPhases(uint32_t currentTimeMs) {
    uint32_t elapsed = currentTimeMs - animationStartTime_;
    
    if (animationConfig_.enableFadeIn) {
        fadeInProgress_ = std::min(1.0f, static_cast<float>(elapsed) / animationConfig_.fadeInDuration);
    } else {
        fadeInProgress_ = 1.0f;
    }
    
    if (animationConfig_.enablePulse) {
        pulsePhase_ = std::sin(2.0f * M_PI * elapsed / animationConfig_.pulsePeriod);
    }
    
    if (animationConfig_.enableBorderGlow) {
        glowPhase_ = 0.5f + 0.5f * std::sin(2.0f * M_PI * elapsed / (animationConfig_.pulsePeriod * 0.7f));
    }
    
    if (animationConfig_.enableCornerBlink) {
        blinkPhase_ = (std::sin(2.0f * M_PI * elapsed / animationConfig_.blinkPeriod) > 0.0f) ? 1.0f : 0.0f;
    }
}

uint8_t SelectionVisualizer::getAnimatedAlpha(uint8_t baseAlpha, PatternSelection::SelectionState state) const {
    float alpha = baseAlpha;
    
    if (animationConfig_.enableFadeIn) {
        alpha *= fadeInProgress_;
    }
    
    return static_cast<uint8_t>(alpha);
}

uint32_t SelectionVisualizer::getAnimatedColor(uint32_t baseColor, PatternSelection::SelectionState state) const {
    uint32_t color = baseColor;
    
    if (state == PatternSelection::SelectionState::INVALID) {
        color = style_.invalidSelectionColor;
    }
    
    return color;
}

void SelectionVisualizer::expandDirtyRegion(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint16_t x1 = std::min(dirtyX_, x);
    uint16_t y1 = std::min(dirtyY_, y);
    uint16_t x2 = std::max(dirtyX_ + dirtyWidth_, x + width);
    uint16_t y2 = std::max(dirtyY_ + dirtyHeight_, y + height);
    
    dirtyX_ = x1;
    dirtyY_ = y1;
    dirtyWidth_ = x2 - x1;
    dirtyHeight_ = y2 - y1;
}

void SelectionVisualizer::notifyRedraw(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    if (redrawCallback_) {
        redrawCallback_(x, y, width, height);
    }
}

// Drawing primitives (mock implementations for compilation)
void SelectionVisualizer::drawRectangle(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                                        uint32_t color, uint8_t alpha) const {
    // In real implementation, would use TouchGFX Graphics to draw filled rectangle
}

void SelectionVisualizer::drawBorder(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                                     uint32_t color, uint8_t lineWidth) const {
    // In real implementation, would use TouchGFX Graphics to draw rectangle outline
}

void SelectionVisualizer::drawCircle(void* graphics, uint16_t centerX, uint16_t centerY, uint16_t radius, 
                                     uint32_t color, uint8_t alpha) const {
    // In real implementation, would use TouchGFX Graphics to draw filled circle
}

void SelectionVisualizer::drawText(void* graphics, uint16_t x, uint16_t y, const char* text, 
                                   uint32_t color, uint8_t size) const {
    // In real implementation, would use TouchGFX Text to draw centered text
}

void SelectionVisualizer::drawGradientRectangle(void* graphics, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                                                uint32_t color1, uint32_t color2, uint8_t alpha) const {
    // In real implementation, would use TouchGFX Graphics to draw gradient rectangle
}