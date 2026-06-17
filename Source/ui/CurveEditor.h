#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>
#include "BaseCurveDisplay.h"

class LFO;

class CurveEditor : public BaseCurveDisplay,
                    public juce::Timer
{
public:
    struct ControlPoint
    {
        float x;
        float y;
        
        ControlPoint(float xPos = 0.0f, float yPos = 0.5f) : x(xPos), y(yPos) {}
    };
    
    CurveEditor() : lfo(nullptr), tension(0.5f)
    {
        controlPoints = defaultControlPoints();
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
    
    ~CurveEditor() override
    {
        if (lfo != nullptr)
            stopTimer();
    }
    
    void setLFO(LFO* lfoToVisualize)
    {
        lfo = lfoToVisualize;
        updateTimerState();
    }
    
    void visibilityChanged() override
    {
        updateTimerState();
    }
    
    void updateTimerState()
    {
        if (isVisible() && lfo != nullptr)
            startTimerHz(60);
        else
            stopTimer();
    }
    
    void timerCallback() override
    {
        if (lfo != nullptr)
            repaint();
    }

protected:
    bool shouldDrawCenterLine() const override { return true; }
    
    void generateCurvePath(juce::Path& path, const juce::Rectangle<float>& bounds) const override
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
    
    void drawControlPoints(juce::Graphics& g, const juce::Rectangle<float>& bounds) const override
    {
        if (lfo != nullptr)
        {
            float currentPhase = lfo->getCurrentPhase();
            if (currentPhase >= 0.0f && currentPhase <= 1.0f)
            {
                float playheadX = bounds.getX() + currentPhase * bounds.getWidth();
                g.setColour(juce::Colour(0xff00ff41).brighter(0.5f));
                g.drawLine(playheadX, bounds.getY(), playheadX, bounds.getBottom(), 2.0f);
            }
        }
        
        for (size_t i = 0; i < controlPoints.size(); ++i)
        {
            auto point = controlPoints[i];
            float px = bounds.getX() + point.x * bounds.getWidth();
            float py = bounds.getY() + (1.0f - point.y) * bounds.getHeight();
            
            bool isHovered = (hoveredPointIndex.has_value() && i == hoveredPointIndex.value());
            bool isDragged = (draggedPointIndex.has_value() && i == draggedPointIndex.value());
            bool isHighlighted = isDragged || isHovered;
            
            drawPoint(g, px, py, isHighlighted);
        }
    }

public:
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
        return evaluate(controlPoints, tension, x);
    }

    // The editor's default sine-like curve. Shared so the processor can fall back to it when
    // an LFO has no saved curve points.
    static std::vector<ControlPoint> defaultControlPoints()
    {
        return {
            ControlPoint(0.0f,  0.5f),
            ControlPoint(0.25f, 1.0f),
            ControlPoint(0.5f,  0.5f),
            ControlPoint(0.75f, 0.0f),
            ControlPoint(1.0f,  0.5f)
        };
    }

    // UI-free evaluation of (control points + tension) at x in [0,1]. Used both for the editor's
    // own rendering and by the processor to rebuild the audio LFO lookup table from saved state.
    static float evaluate(const std::vector<ControlPoint>& points, float tension, float x)
    {
        x = juce::jlimit(0.0f, 1.0f, x);
        if (points.size() < 2)
            return 0.5f;

        size_t segmentIndex = points.size() - 1;
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            if (x <= points[i + 1].x)
            {
                segmentIndex = i;
                break;
            }
        }

        if (segmentIndex >= points.size() - 1)
            return points.back().y;

        const auto& startPoint  = points[segmentIndex];
        const auto& endPoint    = points[segmentIndex + 1];
        const auto& beforePoint = (segmentIndex > 0) ? points[segmentIndex - 1] : startPoint;
        const auto& afterPoint  = (segmentIndex + 2 < points.size()) ? points[segmentIndex + 2] : endPoint;

        const float t = (x - startPoint.x) / (endPoint.x - startPoint.x);
        return catmullRom(beforePoint.y, startPoint.y, endPoint.y, afterPoint.y, t, tension);
    }

    const std::vector<ControlPoint>& getControlPoints() const { return controlPoints; }
    
    void setControlPoints(const std::vector<ControlPoint>& points)
    {
        controlPoints = points;
        repaint();
    }
    
    void setTension(float newTension)
    {
        tension = juce::jlimit(0.0f, 1.0f, newTension);
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
    // Catmull-Rom spline with tension control (static so the processor can reuse it).
    static float catmullRom(float y0, float y1, float y2, float y3, float t, float tension)
    {
        float cardinalParam = (tension - 0.5f) * 2.0f;
        float tensionScale = (1.0f - cardinalParam) * 0.5f;
        float t2 = t * t;
        float t3 = t2 * t;

        float result = tensionScale * (
            (2.0f * y1) +
            (-y0 + y2) * t +
            (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * t2 +
            (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * t3
        ) + cardinalParam * (y1 + (y2 - y1) * t);
        
        return juce::jlimit(0.0f, 1.0f, result);
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
    LFO* lfo;
    float tension;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveEditor)
};
