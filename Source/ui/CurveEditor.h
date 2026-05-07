#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>

class CurveEditor : public juce::Component
{
public:
    struct ControlPoint
    {
        float x; 
        float y; 
        
        ControlPoint(float xPos = 0.0f, float yPos = 0.5f) : x(xPos), y(yPos) {}
    };
    
    CurveEditor()
    {
        // Initialize with a simple sine-like curve
        controlPoints.clear();
        controlPoints.push_back(ControlPoint(0.0f, 0.5f));
        controlPoints.push_back(ControlPoint(0.25f, 1.0f));
        controlPoints.push_back(ControlPoint(0.5f, 0.5f));
        controlPoints.push_back(ControlPoint(0.75f, 0.0f));
        controlPoints.push_back(ControlPoint(1.0f, 0.5f));
        
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Draw background
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Draw grid lines
        g.setColour(juce::Colour(0xff2a2a2a));
        const int numVerticalLines = 8;
        const int numHorizontalLines = 4;
        
        for (int i = 1; i < numVerticalLines; ++i)
        {
            float x = bounds.getX() + (bounds.getWidth() * i / numVerticalLines);
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        }
        
        for (int i = 1; i < numHorizontalLines; ++i)
        {
            float y = bounds.getY() + (bounds.getHeight() * i / numHorizontalLines);
            g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
        }
        
        // Draw center line
        g.setColour(juce::Colour(0xff3a3a3a));
        float centerY = bounds.getCentreY();
        g.drawLine(bounds.getX(), centerY, bounds.getRight(), centerY, 1.5f);
        
        // Generate curve path
        juce::Path curvePath;
        generateCurvePath(curvePath, bounds);
        
        // Draw filled area under curve
        juce::Path filledPath = curvePath;
        filledPath.lineTo(bounds.getRight(), bounds.getBottom());
        filledPath.lineTo(bounds.getX(), bounds.getBottom());
        filledPath.closeSubPath();
        
        g.setColour(juce::Colour(0xff00ff41).withAlpha(0.2f));
        g.fillPath(filledPath);
        
        // Draw curve line
        g.setColour(juce::Colour(0xff00ff41));
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));
        
        // Draw control points
        for (size_t i = 0; i < controlPoints.size(); ++i)
        {
            auto point = controlPoints[i];
            float px = bounds.getX() + point.x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - point.y) * bounds.getHeight();
            
            // Outer circle
            g.setColour(juce::Colour(0xff00ff41));
            g.fillEllipse(px - 6, py - 6, 12, 12);
            
            // Inner circle
            bool isHovered = (hoveredPointIndex.has_value() && i == hoveredPointIndex.value());
            bool isDragged = (draggedPointIndex.has_value() && i == draggedPointIndex.value());
            
            if (isDragged)
                g.setColour(juce::Colours::white);
            else if (isHovered)
                g.setColour(juce::Colour(0xff00ff41).brighter(0.3f));
            else
                g.setColour(juce::Colour(0xff0a0a0a));
                
            g.fillEllipse(px - 4, py - 4, 8, 8);
        }
    }
    
    void mouseMove(const juce::MouseEvent& event) override
    {
        hoveredPointIndex = findNearestPoint(event.position);
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override
    {
        hoveredPointIndex.reset();
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isLeftButtonDown())
        {
            draggedPointIndex = findNearestPoint(event.position);
            
            // Double-click to add/remove points
            if (event.getNumberOfClicks() == 2)
            {
                if (draggedPointIndex.has_value())
                {
                    // Remove point (but keep first and last)
                    size_t pointIndex = draggedPointIndex.value();
                    if (pointIndex > 0 && pointIndex < controlPoints.size() - 1)
                    {
                        auto pointIterator = controlPoints.begin();
                        std::advance(pointIterator, pointIndex);
                        controlPoints.erase(pointIterator);
                        draggedPointIndex.reset();
                        notifyListeners();
                        repaint();
                    }
                }
                else
                {
                    // Add new point (if under limit)
                    const size_t maxPoints = 32;
                    if (controlPoints.size() < maxPoints)
                    {
                        auto bounds = getLocalBounds().toFloat();
                        float x = juce::jlimit(0.0f, 1.0f, (event.position.x - bounds.getX()) / bounds.getWidth());
                        float y = juce::jlimit(0.0f, 1.0f, 1.0f - (event.position.y - bounds.getY()) / bounds.getHeight());
                        
                        // Find where to insert to keep points sorted by X position
                        auto insertPosition = std::lower_bound(controlPoints.begin(), controlPoints.end(), x,
                            [](const ControlPoint& p, float val) { return p.x < val; });
                        controlPoints.insert(insertPosition, ControlPoint(x, y));
                        notifyListeners();
                        repaint();
                    }
                }
            }
        }
    }
    
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (draggedPointIndex.has_value() && draggedPointIndex.value() < controlPoints.size())
        {
            auto bounds = getLocalBounds().toFloat();
            float x = (event.position.x - bounds.getX()) / bounds.getWidth();
            float y = 1.0f - (event.position.y - bounds.getY()) / bounds.getHeight();
            
            // Clamp Y value
            y = juce::jlimit(0.0f, 1.0f, y);
            
            // Constrain X position based on point type
            if (draggedPointIndex.value() == 0)
            {
                // First point stays at x=0
                x = 0.0f;
                // Sync last point's Y value for seamless loop
                controlPoints.back().y = y;
            }
            else if (draggedPointIndex.value() == controlPoints.size() - 1)
            {
                // Last point stays at x=1
                x = 1.0f;
                // Sync first point's Y value for seamless loop
                controlPoints.front().y = y;
            }
            else
            {
                // Middle points: constrain between neighbors with small margin
                const float margin = 0.01f; // Minimum distance between points
                float minX = controlPoints[draggedPointIndex.value() - 1].x + margin;
                float maxX = controlPoints[draggedPointIndex.value() + 1].x - margin;
                x = juce::jlimit(minX, maxX, x);
            }
            
            controlPoints[draggedPointIndex.value()].x = x;
            controlPoints[draggedPointIndex.value()].y = y;
            
            notifyListeners();
            repaint();
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override
    {
        draggedPointIndex.reset();
        repaint();
    }
    
    // Get interpolated value at position x (0.0 to 1.0) using Catmull-Rom spline
    float getValueAt(float x) const
    {
        // Step 1: Validate input
        x = juce::jlimit(0.0f, 1.0f, x);
        if (controlPoints.size() < 2)
            return 0.5f;
        
        // Step 2: Find which two control points x is between
        size_t segmentIndex = findSegmentIndex(x);
        if (segmentIndex >= controlPoints.size() - 1)
            return controlPoints.back().y;
        
        // Step 3: Get the 4 points needed for smooth interpolation
        const auto& startPoint = controlPoints[segmentIndex];
        const auto& endPoint = controlPoints[segmentIndex + 1];
        const auto& beforePoint = (segmentIndex > 0) ? controlPoints[segmentIndex - 1] : startPoint;
        const auto& afterPoint = (segmentIndex + 2 < controlPoints.size()) ? controlPoints[segmentIndex + 2] : endPoint;
        
        // Step 4: Calculate how far along we are between start and end (0.0 to 1.0)
        float t = (x - startPoint.x) / (endPoint.x - startPoint.x);
        
        // Step 5: Apply smooth interpolation formula
        return catmullRom(beforePoint.y, startPoint.y, endPoint.y, afterPoint.y, t);
    }

    const std::vector<ControlPoint>& getControlPoints() const { return controlPoints; }
    
    void setControlPoints(const std::vector<ControlPoint>& points)
    {
        controlPoints = points;
        repaint();
    }
    
    // Listener interface for curve changes
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void curveChanged(CurveEditor* editor) = 0;
    };
    
    void addListener(Listener* listener) { listeners.add(listener); }
    void removeListener(Listener* listener) { listeners.remove(listener); }

private:
    // Helper: Find which segment contains the given x position
    size_t findSegmentIndex(float x) const
    {
        for (size_t i = 0; i < controlPoints.size() - 1; ++i)
        {
            if (x <= controlPoints[i + 1].x)
                return i;
        }
        return controlPoints.size() - 1;
    }
    
    // Catmull-Rom spline: smooth curve through y1 and y2, using y0 and y3 for direction
    float catmullRom(float y0, float y1, float y2, float y3, float t) const
    {
        float t2 = t * t;
        float t3 = t2 * t;
        
        float result = 0.5f * (
            (2.0f * y1) +
            (-y0 + y2) * t +
            (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * t2 +
            (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * t3
        );
        
        return juce::jlimit(0.0f, 1.0f, result);
    }
    
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const
    {
        if (controlPoints.empty())
            return;
        
        // Generate smooth curve through control points
        const int resolution = 200;
        bool first = true;
        
        for (int i = 0; i <= resolution; ++i)
        {
            float x = i / (float)resolution;
            float value = getValueAt(x);
            
            float px = bounds.getX() + x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - value) * bounds.getHeight();
            
            if (first)
            {
                path.startNewSubPath(px, py);
                first = false;
            }
            else
            {
                path.lineTo(px, py);
            }
        }
    }
    
    std::optional<size_t> findNearestPoint(juce::Point<float> pos) const
    {
        auto bounds = getLocalBounds().toFloat();
        const float threshold = 10.0f;
        std::optional<size_t> nearest;
        float minDist = threshold;
        
        for (size_t i = 0; i < controlPoints.size(); ++i)
        {
            float px = bounds.getX() + controlPoints[i].x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - controlPoints[i].y) * bounds.getHeight();
            
            float dist = pos.getDistanceFrom(juce::Point<float>(px, py));
            if (dist < minDist)
            {
                minDist = dist;
                nearest = i;
            }
        }
        
        return nearest;
    }
    
    void notifyListeners()
    {
        listeners.call([this](Listener& l) { l.curveChanged(this); });
    }
    
    std::vector<ControlPoint> controlPoints;
    std::optional<size_t> hoveredPointIndex;
    std::optional<size_t> draggedPointIndex;
    juce::ListenerList<Listener> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveEditor)
};
