#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ui/CurveEditor.h"

class LFOCurveState
{
public:
    LFOCurveState() = default;
    
    void initialise(juce::ValueTree treeState, juce::UndoManager* treeUndoManager)
    {
        state = treeState;
        undoManager = treeUndoManager;
    }
    
    std::vector<CurveEditor::ControlPoint> getCurvePoints(size_t lfoIndex) const
    {
        std::vector<CurveEditor::ControlPoint> points;
        auto curveNode = findCurve(static_cast<int>(lfoIndex));
        
        if (! curveNode.isValid())
            return points;
        
        points.reserve(static_cast<size_t>(curveNode.getNumChildren()));
        
        for (auto pointNode : curveNode)
        {
            if (pointNode.hasType(IDs::point))
            {
                points.emplace_back(
                    static_cast<float>(pointNode.getProperty(IDs::x, 0.0f)),
                    static_cast<float>(pointNode.getProperty(IDs::y, 0.5f))
                );
            }
        }
        
        return points;
    }
    
    void setCurvePoints(size_t lfoIndex, const std::vector<CurveEditor::ControlPoint>& points)
    {
        if (! state.isValid())
            return;
        
        auto curveNode = findOrCreateCurve(static_cast<int>(lfoIndex));
        
        curveNode.removeAllChildren(undoManager);
        
        for (const auto& point : points)
        {
            juce::ValueTree pointNode(IDs::point);
            pointNode.setProperty(IDs::x, point.x, undoManager);
            pointNode.setProperty(IDs::y, point.y, undoManager);
            curveNode.addChild(pointNode, -1, undoManager);
        }
    }

private:
    struct IDs
    {
        static inline const juce::Identifier curve { "LFOCurve" };
        static inline const juce::Identifier point { "Point" };
        static inline const juce::Identifier index { "index" };
        static inline const juce::Identifier x { "x" };
        static inline const juce::Identifier y { "y" };
    };
    
    juce::ValueTree findCurve(int lfoIndex) const
    {
        if (! state.isValid())
            return {};
        
        for (auto child : state)
        {
            if (child.hasType(IDs::curve)
                && static_cast<int>(child.getProperty(IDs::index, -1)) == lfoIndex)
                return child;
        }
        
        return {};
    }
    
    juce::ValueTree findOrCreateCurve(int lfoIndex)
    {
        auto curveNode = findCurve(lfoIndex);
        if (curveNode.isValid())
            return curveNode;
        
        curveNode = juce::ValueTree(IDs::curve);
        curveNode.setProperty(IDs::index, lfoIndex, undoManager);
        state.addChild(curveNode, -1, undoManager);
        return curveNode;
    }
    
    juce::ValueTree state;
    juce::UndoManager* undoManager = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOCurveState)
};
